/*
Translator plugin for Miranda IM

Copyright © 2006-2007 Cristian Libotean

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "dlg_handlers.h"
#include "commctrl.h"

#define UPDATE_TIMER 100

#define FILTER_TIMER 101

int filterTimeout = DEFAULT_FILTER_TIMEOUT;
int bAutomaticFilter = TRUE;

HWND hWndTranslations = NULL;

WNDPROC oldTranslationsListDlgProc = NULL;

#define MIN_TRANSLATIONS_WIDTH  400
#define MIN_TRANSLATIONS_HEIGHT 400

const char *szTranslateFilter[] = {"All strings", "Only untranslated strings", "Only translated strings"};
const int cTranslateFilter = sizeof(szTranslateFilter) / sizeof(szTranslateFilter[0]);

const WCHAR *szTranslationsColumns[] = {L"Hash", L"English", L"Translation", L"Module", L"Flags"};
const int cxTranslationsColumns[] = {70, 220, 220, 80, 80};
const int cTranslationsColumns = sizeof(szTranslationsColumns) / sizeof(szTranslationsColumns[0]);

__inline void ConvertCase(WCHAR *dest, const WCHAR *source, size_t size)
{
	wcsncpy(dest, source, size);
	dest[size - 1] = 0;
	_wcslwr(dest);
}

int MatchesFilterCI(const WCHAR *filterS, const PTranslation translation)
{
	if (wcslen(filterS) <= 0)	{ return 1;	} //if no filter is set then the popup item matches the filter
	int match = 0;
	const int BUFFER_SIZE = 8192;
	WCHAR buffer[BUFFER_SIZE];
	WCHAR filterI[BUFFER_SIZE];
	
	ConvertCase(filterI, filterS, BUFFER_SIZE);
	
	ConvertCase(buffer, translation->english, BUFFER_SIZE); //check english part
	match = (wcsstr(buffer, filterI)) ? 1 : match;
	
	if (!match) // check transltion part of no match has been found
	{
		ConvertCase(buffer, translation->translation, BUFFER_SIZE);
		match = (wcsstr(buffer, filterI)) ? 1 : match;
	}
	
	return match;
}

typedef int (*SIG_MATCHESFILTER)(const WCHAR *filter, const PTranslation translation);

int AddInfoToCombobox(HWND hWnd, int nTranslateFilterComboBox)
{
	int i;
	for (i = 0; i < cTranslateFilter; i++)
		{
			SendDlgItemMessage(hWnd, nTranslateFilterComboBox, CB_ADDSTRING, 0, (LPARAM) Translate(szTranslateFilter[i]));
		}
		
	return 0;
}

SIZE GetControlTextSize(HWND hCtrl)
{
	HDC hDC = GetDC(hCtrl);
	HFONT font = (HFONT) SendMessage(hCtrl, WM_GETFONT, 0, 0);
	HFONT oldFont = (HFONT) SelectObject(hDC, font);
	const int maxSize = 2048;
	TCHAR buffer[maxSize];
	SIZE size;
	GetWindowText(hCtrl, buffer, maxSize);
	GetTextExtentPoint32(hDC, buffer, (int) _tcslen(buffer), &size);
	SelectObject(hDC, oldFont);
	ReleaseDC(hCtrl, hDC);
	return size;
}

int EnlargeControl(HWND hCtrl, HWND hGroup, SIZE oldSize)
{
	SIZE size = GetControlTextSize(hCtrl);
	int offset = 0;
	RECT rect;
	GetWindowRect(hCtrl, &rect);
 	offset = (rect.right - rect.left) - oldSize.cx;
	SetWindowPos(hCtrl, HWND_TOP, 0, 0, size.cx + offset, oldSize.cy, SWP_NOMOVE);
	SetWindowPos(hCtrl, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	//RedrawWindow(hCtrl, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASENOW);
	
	return 0;
}

struct SortParams{
	HWND hList;
	int column;
};

static int lastColumn = -1;

int CALLBACK TranslationsCompare(LPARAM lParam1, LPARAM lParam2, LPARAM myParam)
{
	SortParams params = *(SortParams *) myParam;
	int res;
	const int MAX_SIZE = 1024;
	
	wchar_t text1[MAX_SIZE];
	wchar_t text2[MAX_SIZE];
	
	ListView_GetItemTextW(params.hList, (int) lParam1, params.column, text1, MAX_SIZE);
	ListView_GetItemTextW(params.hList, (int) lParam2, params.column, text2, MAX_SIZE);
	
	res = _wcsicmp(text1, text2);
	
	res = (params.column == lastColumn) ? -res : res;
	
	return res;
}

TTranslation *GetSelectedTranslation(HWND hWnd, int *index = NULL)
{
	int i;
	HWND hList = GetDlgItem(hWnd, IDC_LIST_TRANSLATIONS);
	int count = ListView_GetItemCount(hList);
	for (i = 0; i < count; i++)
	{
		if (ListView_GetItemState(hList, i, LVIS_SELECTED) == LVIS_SELECTED)
		{
			LVITEM item = {0};
			item.iItem = i;
			item.mask = LVIF_PARAM;
			
			if (index)
			{
				*index = i;
			}
			
			ListView_GetItem(hList, &item);
			return (TTranslation *) item.lParam;
		}
	}
	
	if (index)
	{
		*index = -1;
	}
	return NULL;
}

void LoadTranslationInfo(HWND hWnd, TTranslation *translation)
{
	const int MAX_SIZE = 8192;
	WCHAR buffer[MAX_SIZE];
	if (translation)
	{
		wcsncpy(buffer, translation->english, MAX_SIZE);
		ParseSpecialChars(buffer);
		buffer[MAX_SIZE - 1] = 0;
		SetWindowTextW(GetDlgItem(hWnd, IDC_ENGLISH), buffer);

		wcsncpy(buffer, translation->translation, MAX_SIZE);
		ParseSpecialChars(buffer);
		buffer[MAX_SIZE - 1] = 0;
		SetWindowTextW(GetDlgItem(hWnd, IDC_TRANSLATION), buffer);
	}
}

void LoadTranslationsColumns(HWND hList)
{
	int i;
	LVCOLUMNW col = {0};
	col.mask = LVCF_TEXT | LVCF_WIDTH;
	
	for (i = 0; i < cTranslationsColumns; i++)
	{
		col.pszText = (WCHAR *) realCallServiceFunction(MS_LANGPACK_TRANSLATESTRING, LANG_UNICODE, (LPARAM) szTranslationsColumns[i]);
		col.cx = cxTranslationsColumns[i];
	
		ListView_InsertColumnW(hList, i, &col);
	}
}

void UpdateTranslation(HWND hList, int index, TTranslation *translation)
{
	const int MAX_SIZE = 1024;
	WCHAR buffer[MAX_SIZE];
	
	ListView_SetItemTextW(hList, index, 1, translation->english);
	ListView_SetItemTextW(hList, index, 2, translation->translation);
	ListView_SetItemTextW(hList, index, 3, translation->module);
	
	buffer[0] = L'\0';
	int ok = 0;
	if (translation->flags & TF_LANGPACK)
	{
		ok = 1;
		wcsncat(buffer, L"Lang pack", MAX_SIZE);
	}
	
	if (translation->flags & TF_DIALOG)
	{
		if (ok) wcsncat(buffer, L", ", MAX_SIZE);
		ok = 1;
		wcsncat(buffer, L"Dialog", MAX_SIZE);
	}
	
	if (translation->flags & TF_MENU)
	{
		if (ok) wcsncat(buffer, L", ", MAX_SIZE);
		ok = 1;
		wcsncat(buffer, L"Menu", MAX_SIZE);
	}
	
	if  (translation->flags & TF_WIDE)
	{
		if (ok) wcsncat(buffer, L", ", MAX_SIZE);
		ok = 1;
		wcsncat(buffer, L"Wide", MAX_SIZE);
	}
	
	ListView_SetItemTextW(hList, index, 4, buffer);

}

void LoadTranslations(HWND hWnd, WCHAR *filter)
{
	HWND hList = GetDlgItem(hWnd, IDC_LIST_TRANSLATIONS);
	ListView_DeleteAllItems(hList);
	
	int i;
	LVITEMW item = {0};
	item.mask = LVIF_TEXT | LVIF_PARAM;
	const int MAX_SIZE = 128;
	WCHAR buffer[MAX_SIZE];
	TTranslation *translation;
	
	int index = 0;
	for (i = 0; i < lstTranslations.Count(); i++)
	{
		translation = lstTranslations[i];
		if (MatchesFilterCI(filter, translation))
		{
			_snwprintf(buffer, MAX_SIZE, L"%08X", translation->hEnglish);
			
			item.iItem = index;
			item.pszText = buffer;
			item.lParam = (LPARAM) translation;
			ListView_InsertItemW(hList, &item);
			
			UpdateTranslation(hList, index++, translation);
		}
	}
}

void SaveSelectedTranslation(HWND hWnd)
{
	int index;
	TTranslation *translation = GetSelectedTranslation(hWnd, &index);
	if (translation)
	{
		static const int MAX_SIZE = 8192;
		WCHAR buffer[MAX_SIZE];
		GetWindowTextW(GetDlgItem(hWnd, IDC_TRANSLATION), buffer, MAX_SIZE);
		ShowSpecialChars(buffer);
		
		//free(translation->translation);
		//translation->translation = _wcsdup(buffer);
		ReplaceTranslationString(translation->translation, buffer);

		if (!translation->hModule)
		{
			ReplaceTranslationString(translation->module, LANGUAGEPACK_MODIFIED);
		}	
		
		if (lstTranslations.DynamicTranslations())
		{
			ReplaceDynamicTranslation(translation, buffer);
		}
		
		lstTranslations.InvalidateTranslation(translation);
		
		UpdateTranslation(GetDlgItem(hWnd, IDC_LIST_TRANSLATIONS), index, translation);
	}
}

void EnableTranslationControls(HWND hWnd, int bEnable)
{
	EnableWindow(GetDlgItem(hWnd, IDC_DYNAMIC_TRANSLATION), bEnable);
	EnableWindow(GetDlgItem(hWnd, IDC_IGNORE_DOUBLE_TRANSLATIONS), bEnable);
	EnableWindow(GetDlgItem(hWnd, IDC_TRANSLATIONS), bEnable);
	EnableWindow(GetDlgItem(hWnd, IDC_LANGUAGEPACK_IGNOREUNTRANSLATED), bEnable);
	
	EnableWindow(GetDlgItem(hWnd, IDC_FILTER_MODE), !bEnable);
	//EnableWindow(GetDlgItem(hWnd, IDC_FLUSH), bEnable);
	EnableWindow(GetDlgItem(hWnd, IDC_FLUSHAMOUNT), !bEnable);
	
	EnableWindow(GetDlgItem(hWnd, IDC_AUTOMATIC_FILTER), bEnable);
	EnableWindow(GetDlgItem(hWnd, IDC_FILTER_TIMEOUT), bEnable && IsDlgButtonChecked(hWnd, IDC_AUTOMATIC_FILTER) ? 1 : 0);
}

INT_PTR CALLBACK DlgProcOptions(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static int bInitializing = 0;
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			bInitializing = 1;
		
			char buffer[256];
			int flush = DBGetContactSettingWord(NULL, ModuleName, "FlushLimit", 200);
			int translationMode = DBGetContactSettingByte(NULL, ModuleName, "TranslationMode", 0);
			int bCustomCP = DBGetContactSettingByte(NULL, ModuleName, "UseCustomCodePage", 0);
			int bLPIgnoreUntranslated = DBGetContactSettingByte(NULL, ModuleName, "LPIgnoreUntranslated", 0);
			filterTimeout = DBGetContactSettingWord(NULL, ModuleName, "FilterTimeout", DEFAULT_FILTER_TIMEOUT);
			bAutomaticFilter = DBGetContactSettingByte(NULL, ModuleName, "AutomaticFilter", TRUE);

			CheckDlgButton(hWnd, IDC_DYNAMIC_TRANSLATION, lstTranslations.DynamicTranslations());
			CheckDlgButton(hWnd, IDC_IGNORE_DOUBLE_TRANSLATIONS, lstTranslations.IgnoreDoubleTranslations());
			CheckDlgButton(hWnd, IDC_LANGUAGEPACK_IGNOREUNTRANSLATED, bLPIgnoreUntranslated);
			
			CheckDlgButton(hWnd, IDC_AUTOMATIC_FILTER, bAutomaticFilter);
			
			DWORD customCP = (bCustomCP) ? DBGetContactSettingDword(NULL, ModuleName, "CustomCodePage", ACTIVE_CODEPAGE) : CallService(MS_LANGPACK_GETCODEPAGE, 0, 0);
			_itoa(customCP, buffer, 10);
			SetWindowText(GetDlgItem(hWnd, IDC_CUSTOM_CODEPAGE), buffer);
			CheckDlgButton(hWnd, IDC_OVERRIDE_CODEPAGE, bCustomCP);
			EnableWindow(GetDlgItem(hWnd, IDC_CUSTOM_CODEPAGE), bCustomCP);
			
			_itoa(flush, buffer, 10);
			SetWindowText(GetDlgItem(hWnd, IDC_FLUSHAMOUNT), buffer);
			
			_itoa(filterTimeout, buffer, 10);
			SetWindowText(GetDlgItem(hWnd, IDC_FILTER_TIMEOUT), buffer);
			
			AddInfoToCombobox(hWnd, IDC_FILTER_MODE);

			SIZE oldSize = GetControlTextSize(GetDlgItem(hWnd, IDC_TRANSLATION_MODE));
			
			TranslateDialogDefault(hWnd);
			
			EnlargeControl(GetDlgItem(hWnd, IDC_TRANSLATION_MODE), GetDlgItem(hWnd, IDC_TRANSLATIONS_GROUPBOX), oldSize);
		
			nCurrentFilterMode = DBGetContactSettingByte(NULL, ModuleName, "Filter", 0);
			
			SendDlgItemMessage(hWnd, IDC_FILTER_MODE, CB_SETCURSEL, nCurrentFilterMode, 0);
			
			SetTimer(hWnd, UPDATE_TIMER, 500, NULL);
			PostMessage(hWnd, WM_TIMER, UPDATE_TIMER, 0);
			
			CheckDlgButton(hWnd, IDC_TRANSLATION_MODE, translationMode);
			CheckDlgButton(hWnd, IDC_FLUSHONEXIT, DBGetContactSettingByte(NULL, ModuleName, "FlushOnExit", 1));
			
			EnableTranslationControls(hWnd, translationMode);
			
			bInitializing = 0;
			return TRUE;
			break;
		}
		
		case WM_TIMER:
		{
			switch (wParam)
			{
				case UPDATE_TIMER:
				{
					char buffer[1024];
					sprintf(buffer, Translate("Strings in memory: %d / %d"), lstTranslations.Count(), lstTranslations.Capacity());
					SetWindowText(GetDlgItem(hWnd, IDC_STATISTICS), buffer);
				
					break;
				}
			}
			
			break;
		}
		
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDC_FLUSHAMOUNT:
				case IDC_CUSTOM_CODEPAGE:
				case IDC_FILTER_TIMEOUT:
				{
					if ((HIWORD(wParam) == EN_CHANGE))
					{
						if (!bInitializing)
						{
							SendMessage(GetParent(hWnd), PSM_CHANGED, 0, 0);
						}
					}
					
					break;
				}
				
				case IDC_TRANSLATION_MODE:
				case IDC_FILTER_MODE:
				case IDC_FLUSHONEXIT:
				case IDC_OVERRIDE_CODEPAGE:
				case IDC_DYNAMIC_TRANSLATION:
				case IDC_IGNORE_DOUBLE_TRANSLATIONS:
				case IDC_LANGUAGEPACK_IGNOREUNTRANSLATED:
				case IDC_AUTOMATIC_FILTER:
				{
					EnableTranslationControls(hWnd, IsDlgButtonChecked(hWnd, IDC_TRANSLATION_MODE));
					EnableWindow(GetDlgItem(hWnd, IDC_CUSTOM_CODEPAGE), IsDlgButtonChecked(hWnd, IDC_OVERRIDE_CODEPAGE)); 
					SendMessage(GetParent(hWnd), PSM_CHANGED, 0, 0);
					
					break;
				}
				
				case IDC_FLUSH:
				{
					lstTranslations.Flush(!lstTranslations.TranslationsMode());
				
					break;
				}
				
				case IDC_TRANSLATIONS:
				{
					CallService(MS_TRANSLATOR_SHOWMANAGER, 0, 0);
					
					break;
				}
			}
			
			break;
		}
		
		case WM_NOTIFY:
		{
			switch(((LPNMHDR)lParam)->idFrom)
			{
				case 0:
				{
					switch (((LPNMHDR)lParam)->code)
					{
						case PSN_APPLY:
						{
							char buffer[256];
							int mode;
							nCurrentFilterMode = SendDlgItemMessage(hWnd, IDC_FILTER_MODE, CB_GETCURSEL, 0, 0);
							DBWriteContactSettingByte(NULL, ModuleName, "Filter", nCurrentFilterMode);
							
							DBWriteContactSettingByte(NULL, ModuleName, "TranslationMode", IsDlgButtonChecked(hWnd, IDC_TRANSLATION_MODE));
							
							
							GetWindowText(GetDlgItem(hWnd, IDC_FLUSHAMOUNT), buffer, sizeof(buffer));
							mode = atoi(buffer);
							DBWriteContactSettingWord(NULL, ModuleName, "FlushLimit", mode);
							
							GetWindowText(GetDlgItem(hWnd, IDC_FILTER_TIMEOUT), buffer, sizeof(buffer));
							filterTimeout = atoi(buffer);
							if (filterTimeout <= 0) { filterTimeout = DEFAULT_FILTER_TIMEOUT; }
							
							DBWriteContactSettingWord(NULL, ModuleName, "FilterTimeout", filterTimeout);
							
							bAutomaticFilter = IsDlgButtonChecked(hWnd, IDC_AUTOMATIC_FILTER) ? 1 : 0;
							DBWriteContactSettingByte(NULL, ModuleName, "AutomaticFilter", bAutomaticFilter);
							
							DBWriteContactSettingByte(NULL, ModuleName, "FlushOnExit", IsDlgButtonChecked(hWnd, IDC_FLUSHONEXIT) ? 1 : 0);
							
							if (IsDlgButtonChecked(hWnd, IDC_OVERRIDE_CODEPAGE))
							{
								GetWindowText(GetDlgItem(hWnd, IDC_CUSTOM_CODEPAGE), buffer, sizeof(buffer));
								ACTIVE_CODEPAGE = atoi(buffer);
							}
							else{
								ACTIVE_CODEPAGE = CallService(MS_LANGPACK_GETCODEPAGE, 0, 0);
							}
							
							DBWriteContactSettingByte(NULL, ModuleName, "UseCustomCodePage", IsDlgButtonChecked(hWnd, IDC_OVERRIDE_CODEPAGE));
							DBWriteContactSettingDword(NULL, ModuleName, "CustomCodePage", ACTIVE_CODEPAGE);
							
							//lstTranslations.SetIgnoreDoubleTranslations(IsDlgButtonChecked(hWnd, IDC_IGNORE_DOUBLE_TRANSLATIONS));
							//lstTranslations.SetDynamicTranslations(IsDlgButtonChecked(hWnd, IDC_DYNAMIC_TRANSLATION));
							
							DBWriteContactSettingByte(NULL, ModuleName, "DynamicTranslations", IsDlgButtonChecked(hWnd, IDC_DYNAMIC_TRANSLATION));
							DBWriteContactSettingByte(NULL, ModuleName, "IgnoreDoubleTranslations", IsDlgButtonChecked(hWnd, IDC_IGNORE_DOUBLE_TRANSLATIONS));

							DBWriteContactSettingByte(NULL, ModuleName, "LPIgnoreUntranslated", IsDlgButtonChecked(hWnd, IDC_LANGUAGEPACK_IGNOREUNTRANSLATED));
						
							break;
						}
					}
					
					break;
				}
			}
			
			break;
		}
	}
	
	return 0;
}

void AddAnchorWindowToDeferList(HDWP &hdWnds, HWND window, RECT *rParent, WINDOWPOS *wndPos, int anchors)
{
	RECT rChild = AnchorCalcPos(window, rParent, wndPos, anchors);
	hdWnds = DeferWindowPos(hdWnds, window, HWND_NOTOPMOST, rChild.left, rChild.top, rChild.right - rChild.left, rChild.bottom - rChild.top, SWP_NOZORDER);
}

void CopySelectedTranslationsToClipboard(HWND hWnd)
{
	if (!GetOpenClipboardWindow())
	{
		size_t size = 0;
		wchar_t *buffer = NULL;
		TTranslation *item = NULL;
		
		HWND hList = GetDlgItem(hWnd, IDC_LIST_TRANSLATIONS);
		int count = ListView_GetItemCount(hList);
		for (int i = 0; i < count; i++)
		{
			if (ListView_GetItemState(hList, i, LVIS_SELECTED) == LVIS_SELECTED)
			{
				LVITEM lvItem = {0};
				lvItem.iItem = i;
				lvItem.mask = LVIF_PARAM;
				
				ListView_GetItem(hList, &lvItem);
				item = (TTranslation *) lvItem.lParam;
			
				size_t count = 2 + wcslen(item->english) + 2 + wcslen(item->translation) + 2 + 2; // [english] + \r\n + translation + \r\n + \0
				
				size += count * sizeof(wchar_t);
				
				wchar_t *tmp = (wchar_t *) malloc(count * sizeof(wchar_t));
				mir_snwprintf(tmp, count, L"[%s]\r\n%s\r\n", item->english, item->translation); //create entry with translation format
				
				if (buffer == NULL)
				{
					buffer = _wcsdup(tmp);
				}
				else{
					buffer = (wchar_t *) realloc(buffer, size);
					wcsncat(buffer, tmp, count);
				}
				
				free(tmp);
			}
		}

		if (buffer)
		{
			INT_PTR res = OpenClipboard(hWnd);
			res = GetLastError();
			res = EmptyClipboard();
			
			HANDLE hData = GlobalAlloc(GMEM_MOVEABLE, size);
			wchar_t *data = (wchar_t *) GlobalLock(hData); //lock memory
			wcscpy(data, buffer);
			res = GlobalUnlock(hData);
			res = (INT_PTR) SetClipboardData(CF_UNICODETEXT, hData);
			res = GetLastError();
			res = CloseClipboard();
		}
		
		free(buffer);
	}
	else{
		MyPUShowMessage(Translate("The clipboard is not available, retry."), SM_WARNING);
	}
}

int CALLBACK TranslationsListSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_KEYUP:
		{
			if ((wParam == 'C') && ((GetKeyState(VK_CONTROL)) & (1 << 8))) //ctrl + c pressed
			{
				CopySelectedTranslationsToClipboard(hWndTranslations);
			}
			
			break;
		}
	}
	
	return CallWindowProc(oldTranslationsListDlgProc, hWnd, msg, wParam, lParam);
}

INT_PTR CALLBACK DlgProcTranslations(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static int bInitializing = 0;
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			bInitializing = 1;
			HWND hList = GetDlgItem(hWnd, IDC_LIST_TRANSLATIONS);
			
			SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM) hiTranslator);
		
			oldTranslationsListDlgProc = (WNDPROC) SetWindowLongPtr(GetDlgItem(hWnd, IDC_LIST_TRANSLATIONS), GWLP_WNDPROC, (LONG_PTR) TranslationsListSubclassProc);
		
			ListView_SetExtendedListViewStyleEx(hList, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
			
			CheckDlgButton(hWnd, IDC_AUTOSAVE, DBGetContactSettingByte(NULL, ModuleName, "AutoSave", 0));
			
			TranslateDialogDefault(hWnd);
			LoadTranslationsColumns(hList);
			
			LoadTranslations(hWnd, L"");
			
			bInitializing = 0;
		
			return TRUE;
			break;
		}
		
		case WM_DESTROY:
		{
			hWndTranslations = NULL;
			oldTranslationsListDlgProc = NULL;
		}
		
		case WM_CLOSE:
		{
			if (hWndTranslations)
			{
				DBWriteContactSettingByte(NULL, ModuleName, "AutoSave", IsDlgButtonChecked(hWnd, IDC_AUTOSAVE));
				DestroyWindow(hWnd);
			}
		
			break;
		}
		
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDC_RESET:
				{
					int index;
					TTranslation *translation = GetSelectedTranslation(hWnd, &index);
					if (translation)
					{
						SetWindowTextW(GetDlgItem(hWnd, IDC_TRANSLATION), translation->translation);
					}
				
					break;
				}
				
				case IDC_SAVE:
				{
					SaveSelectedTranslation(hWnd);
				
					break;
				}
				
				case IDC_EXPORT:
				{
					HWND hExportWnd = NULL;
					DialogBox(hInstance, MAKEINTRESOURCE(IDD_EXPORT), hWnd, DlgProcExport);
				
					break;
				}
				
				case IDC_FILTER_EDIT:
				{
					if (HIWORD(wParam) == EN_CHANGE)
					{
						if ((!bInitializing) && (bAutomaticFilter))
						{
							SetTimer(hWnd, FILTER_TIMER, filterTimeout, NULL);
						}
					}
					break;
				}
				
				case IDC_FILTER:
				{
					static const int MAX_SIZE = 1024;
					WCHAR filter[MAX_SIZE];
					GetWindowTextW(GetDlgItem(hWnd, IDC_FILTER_EDIT), filter, MAX_SIZE);
					
					LoadTranslations(hWnd, filter);
					
					KillTimer(hWnd, FILTER_TIMER);
				
					break;
				}
			}
			
			break;
		}
		
		case WM_TIMER:
		{
			switch (wParam)
			{
				case FILTER_TIMER:
				{
					SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDC_FILTER, 0), 0);
					
					break;
				}
			}
		
			break;
		}
		
		case WM_WINDOWPOSCHANGING:
		{
			HDWP hdWnds = BeginDeferWindowPos(2);
			if (hdWnds == NULL)
			{
				int err = GetLastError();
				char buffer[256];
				sprintf(buffer, "Could not move controls ... error %d", err);
				MessageBox(0, buffer, "Error", MB_OK);
			}
			RECT rParent;
			WINDOWPOS *wndPos = (WINDOWPOS *) lParam;
			GetWindowRect(hWnd, &rParent);

			if (wndPos->cx < MIN_TRANSLATIONS_WIDTH)
			{
				wndPos->cx = MIN_TRANSLATIONS_WIDTH;
			}
			if (wndPos->cy < MIN_TRANSLATIONS_HEIGHT)
			{
				wndPos->cy = MIN_TRANSLATIONS_HEIGHT;
			}
			
			AddAnchorWindowToDeferList(hdWnds, GetDlgItem(hWnd, IDC_LIST_TRANSLATIONS), &rParent, wndPos, ANCHOR_ALL);
			AddAnchorWindowToDeferList(hdWnds, GetDlgItem(hWnd, IDC_RESET), &rParent, wndPos, ANCHOR_RIGHT | ANCHOR_BOTTOM);
			AddAnchorWindowToDeferList(hdWnds, GetDlgItem(hWnd, IDC_SAVE), &rParent, wndPos, ANCHOR_RIGHT | ANCHOR_BOTTOM);
			AddAnchorWindowToDeferList(hdWnds, GetDlgItem(hWnd, IDC_EXPORT), &rParent, wndPos, ANCHOR_RIGHT | ANCHOR_BOTTOM);
			AddAnchorWindowToDeferList(hdWnds, GetDlgItem(hWnd, IDC_AUTOSAVE), &rParent, wndPos, ANCHOR_LEFT | ANCHOR_RIGHT | ANCHOR_BOTTOM);
			AddAnchorWindowToDeferList(hdWnds, GetDlgItem(hWnd, IDC_ENGLISH), &rParent, wndPos, ANCHOR_LEFT | ANCHOR_RIGHT | ANCHOR_BOTTOM);
			AddAnchorWindowToDeferList(hdWnds, GetDlgItem(hWnd, IDC_LABEL_ENGLISH), &rParent, wndPos, ANCHOR_LEFT | ANCHOR_RIGHT | ANCHOR_BOTTOM);
			AddAnchorWindowToDeferList(hdWnds, GetDlgItem(hWnd, IDC_TRANSLATION), &rParent, wndPos, ANCHOR_LEFT | ANCHOR_RIGHT | ANCHOR_BOTTOM);
			AddAnchorWindowToDeferList(hdWnds, GetDlgItem(hWnd, IDC_LABEL_TRANSLATION), &rParent, wndPos, ANCHOR_LEFT | ANCHOR_RIGHT | ANCHOR_BOTTOM);
			AddAnchorWindowToDeferList(hdWnds, GetDlgItem(hWnd, IDC_FILTER_EDIT), &rParent, wndPos, ANCHOR_LEFT | ANCHOR_RIGHT | ANCHOR_TOP);
			AddAnchorWindowToDeferList(hdWnds, GetDlgItem(hWnd, IDC_FILTER), &rParent, wndPos, ANCHOR_RIGHT | ANCHOR_TOP);
			
			EndDeferWindowPos(hdWnds);
			
			break;
		}		

		case WM_NOTIFY:
		{
			switch(((LPNMHDR)lParam)->idFrom)
			{
				case IDC_LIST_TRANSLATIONS:
				{
					switch (((LPNMHDR)lParam)->code)
					{
						case LVN_COLUMNCLICK:
						{
							LPNMLISTVIEW lv = (LPNMLISTVIEW) lParam;
							int column = lv->iSubItem;
							SortParams params = {0};
							params.hList = GetDlgItem(hWnd, IDC_LIST_TRANSLATIONS);
							params.column = column;
							
							ListView_SortItemsEx(params.hList, TranslationsCompare, (LPARAM) &params);
							lastColumn = (params.column == lastColumn) ? -1 : params.column;

							break;
						}
						
						case LVN_ITEMCHANGED:
						{
							static int questionAsked = 0;
							
							NMLISTVIEW *data = (NMLISTVIEW *) lParam;
							if (data->uNewState & LVIS_SELECTED) //new item
							{
								LoadTranslationInfo(hWnd, GetSelectedTranslation(hWnd));
								questionAsked = 0;
							}
							else{ //selected item
								if ((data->uOldState & LVIS_SELECTED) && (!questionAsked))
								{
									const int MAX_SIZE = 8192;
									TTranslation *translation = (TTranslation *) data->lParam;
									if (translation)
									{
										WCHAR buffer[MAX_SIZE];
										GetWindowTextW(GetDlgItem(hWnd, IDC_ENGLISH), buffer, MAX_SIZE);
										ShowSpecialChars(buffer);
										
										if (wcscmp(translation->english, buffer) == 0)
										{
											questionAsked = 1;
										
											WCHAR newTranslation[MAX_SIZE];
											
											GetWindowTextW(GetDlgItem(hWnd, IDC_TRANSLATION), newTranslation, MAX_SIZE);
											ShowSpecialChars(newTranslation);
											
											if (wcscmp(translation->translation, newTranslation))
											{
												int autosave = IsDlgButtonChecked(hWnd, IDC_AUTOSAVE);

												if ((autosave) || (MessageBox(hWnd, Translate("Do you want to save the changes now ?"), Translate("Changes not saved"), MB_YESNO | MB_ICONINFORMATION) == IDYES))
												{
													//free(translation->translation);
													//translation->translation = _wcsdup(newTranslation);
													ReplaceTranslationString(translation->translation, newTranslation);
													if (!translation->hModule)
													{
														ReplaceTranslationString(translation->module, LANGUAGEPACK_MODIFIED);
													}
													
													if (lstTranslations.DynamicTranslations())
													{
														ReplaceDynamicTranslation(translation, newTranslation);
													}
													
													lstTranslations.InvalidateTranslation(translation);
													
													UpdateTranslation(GetDlgItem(hWnd, IDC_LIST_TRANSLATIONS), data->iItem, translation);
												}
											}
										}
									}
								}
							}

							break;
						}
					}
					
					break;
				}
			}
		
			break;
		}
	}
	
	return 0;
}

int HashExists(DWORD hash, DWORD *vector, int count)
{
	int i;
	for (i = 0; i < count; i++)
	{
		if (vector[i] == hash)
		{
			return 1;
		}
	}
	
	return 0;
}

INT_PTR CALLBACK DlgProcExport(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM) hiTranslator);
		
			TranslateDialogDefault(hWnd);
			static const int MAX_SIZE = 32768;
			char buffer[MAX_SIZE];
			char module[128];
			
			SetWindowText(GetDlgItem(hWnd, IDC_LANGUAGE), languagePack.language);
			SetWindowText(GetDlgItem(hWnd, IDC_LOCALE), languagePack.locale);
			SetWindowText(GetDlgItem(hWnd, IDC_AUTHORS), languagePack.authors);
			SetWindowText(GetDlgItem(hWnd, IDC_EMAILS), languagePack.authorEmail);
			SetWindowText(GetDlgItem(hWnd, IDC_FLID), languagePack.FLID);
			
			int count = 0;
			DWORD modules[512]; //are 512 plugins enough ?
			int i;
			if ((languagePack.plugins) &&  (strlen(languagePack.plugins) > 0))
			{
				char *plugin = strtok(languagePack.plugins, ", ");
				DWORD hash;
				while ((plugin) && (*plugin))
				{
					hash = LangPackHash(plugin);
					if (!HashExists(hash, modules, count))
					{
						if (!count)
						{
							strncpy(buffer, plugin, sizeof(buffer));
						}
						else{
							strncat(buffer, (count % 4 == 0) ? ",\r\n" : ", ", sizeof(buffer));
							strncat(buffer, plugin, sizeof(buffer));
						}
						
						modules[count++] = hash;
					}
				
					plugin = strtok(NULL, ", ");
				}
			}
			
			TTranslation *translation;
			for (i = 0; i < lstTranslations.Count(); i++)
			{
				translation = lstTranslations[i];
				if (!HashExists(translation->hModule, modules, count))
				{
					if (wcslen(translation->module) > 0)
					{
						WideCharToMultiByte(CP_ACP, 0, translation->module, -1, module, sizeof(module), NULL, NULL);
						if (!count)
						{
							strncpy(buffer, module, sizeof(buffer));
						}
						else{
							strncat(buffer, (count % 4 == 0) ? ",\r\n" : ", ", sizeof(buffer));
							strncat(buffer, module, sizeof(buffer));
						}
						modules[count++] = translation->hModule;
					}
				}
			}
			SetWindowText(GetDlgItem(hWnd, IDC_PLUGINSINCLUDED), buffer);
		
			return TRUE;
			break;
		}
	
		case WM_DESTROY:
		{
			EndDialog(hWnd, NULL);
		
			break;
		}
		
		case WM_CLOSE:
		{
			DestroyWindow(hWnd);
		
			break;
		}
		
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDC_EXPORT:
				{
					if (GetWindowTextLength(GetDlgItem(hWnd, IDC_OUTPUT)) <= 0)
					{
						MessageBoxW(0, TranslateW(L"Please select an output file."), TranslateW(L"Error"), MB_OK | MB_ICONERROR);
						
						break;
					}
					else if (GetWindowTextLength(GetDlgItem(hWnd, IDC_LOCALE)) <= 0)
					{
						MessageBoxW(0, TranslateW(L"Please specify a locale."), TranslateW(L"Error"), MB_OK | MB_ICONERROR);
						
						break;
					}
					else{
						ExportLanguagePack(hWnd);
					}
				}//fallthrough
			
				case IDC_CANCEL:
				{
					SendMessage(hWnd, WM_CLOSE, 0, 0);
				
					break;
				}
			
				case IDC_BROWSE:
				{
					char folder[MAX_PATH];
					GetModuleFileName(GetModuleHandle(NULL), folder, MAX_PATH);
					char *pos = strrchr(folder, '\\');
					if (!pos) { break; }
					*pos = '\0';
					OPENFILENAME file = {0};
					char fileName[512];
					strncpy(fileName, "langpack_", sizeof(fileName));
					file.lStructSize = sizeof(OPENFILENAME);
					file.hwndOwner = hWnd;
					file.lpstrFilter = "Language pack files\0langpack_*.txt\0All files\0*.*\0\0\0";
					file.lpstrDefExt = "txt";
					file.nFilterIndex = 1;
					file.lpstrFile = fileName;
					file.nMaxFile = sizeof(fileName);
					file.lpstrInitialDir = folder;
					file.lpstrTitle = Translate("Save language pack as");
					file.Flags = OFN_CREATEPROMPT | OFN_ENABLESIZING | OFN_OVERWRITEPROMPT;
					if (GetSaveFileName(&file))
					{
						SetWindowText(GetDlgItem(hWnd, IDC_OUTPUT), file.lpstrFile);
					}
				
					break;
				}
				
				break;
			}
		
			break;
		}
	}
	
	return 0;
}

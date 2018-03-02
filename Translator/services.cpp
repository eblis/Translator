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

#include "services.h"

#define MAX_SIZE 8192

wchar_t TRANSLATIONS_FOLDER[512] = {0};

wchar_t *szEnglish;
wchar_t *szTranslation;

//DWORD ACTIVE_CODEPAGE;

static HANDLE hsShowManager = NULL;

static CRITICAL_SECTION csServices;
DWORD TRANSLATESTRING_HASH = NameHashFunction(MS_LANGPACK_TRANSLATESTRING);
DWORD TRANSLATEMENU_HASH = NameHashFunction(MS_LANGPACK_TRANSLATEMENU);
DWORD TRANSLATEDIALOG_HASH = NameHashFunction(MS_LANGPACK_TRANSLATEDIALOG);

CALLSERVICEFUNCTION realCallServiceFunction = NULL;

int InitServices()
{
	wcscpy(TRANSLATIONS_FOLDER, L"Translations");
	nCurrentFilterMode = DBGetContactSettingByte(NULL, ModuleName, "Filter", 0);
	filterTimeout = DBGetContactSettingWord(NULL, ModuleName, "FilterTimeout", DEFAULT_FILTER_TIMEOUT);
	bAutomaticFilter = DBGetContactSettingByte(NULL, ModuleName, "AutomaticFilter", TRUE);
	
	szEnglish = (wchar_t *) malloc(MAX_SIZE * sizeof(wchar_t));
	szTranslation = (wchar_t *) malloc(MAX_SIZE * sizeof(wchar_t));
	
	InitializeCriticalSection(&csServices);
	
	if ((szEnglish) && (szTranslation))
	{
		ACTIVE_CODEPAGE = (DBGetContactSettingByte(NULL, ModuleName, "UseCustomCodePage", 0)) ? DBGetContactSettingDword(NULL, ModuleName, "CustomCodePage", CP_ACP) : CallService(MS_LANGPACK_GETCODEPAGE, 0, 0);
		lstTranslations.SetTranslationsMode(DBGetContactSettingByte(NULL, ModuleName, "TranslationMode", 0));
		lstTranslations.SetDynamicTranslations(DBGetContactSettingByte(NULL, ModuleName, "DynamicTranslations", 0));
		lstTranslations.SetIgnoreDoubleTranslations(DBGetContactSettingByte(NULL, ModuleName, "IgnoreDoubleTranslations", 0));
		
		if (lstTranslations.TranslationsMode())
		{
			hsShowManager = CreateServiceFunction(MS_TRANSLATOR_SHOWMANAGER, TranslatorShowManagerService);
		}
		
		EnterCriticalSection(&csServices);
		HookRealServices();
		LeaveCriticalSection(&csServices);
	}
	
	return 0;
}

int DestroyServices()
{
	UnhookRealServices();
	DeleteCriticalSection(&csServices);
	
	if (hsShowManager)
	{
		DestroyServiceFunction(hsShowManager);
	}
	
	if (szEnglish)
	{
		free(szEnglish);
	}
	if (szTranslation)
	{
		free(szTranslation);
	}
	
	return 0;
}

void HookRealServices()
{
	Log("Hooking real CallService(). Replacing 0x%p with 0x%p", pluginLink->CallService, TranslatorCallServiceFunction);
	
	realCallServiceFunction = pluginLink->CallService;
	
	pluginLink->CallService = TranslatorCallServiceFunction;
}

void UnhookRealServices()
{
	Log("Unhooking real CallService(). Replacing 0x%p with 0x%p", pluginLink->CallService, realCallServiceFunction);
	EnterCriticalSection(&csServices);
	if (realCallServiceFunction)
	{
		pluginLink->CallService = realCallServiceFunction;
	}
	
	LeaveCriticalSection(&csServices);
}

static void TranslateWindow(const char *data, HWND hWnd, int flags)
{
	const int BUFF_SIZE = MAX_SIZE;
	WCHAR title[BUFF_SIZE];
	WCHAR *translation;
	GetWindowTextW(hWnd, title, BUFF_SIZE);
	{
		translation = (WCHAR *) realCallServiceFunction(MS_LANGPACK_TRANSLATESTRING, LANG_UNICODE, (LPARAM) title);
		int index = AddTranslatableStringToList(data, (char *) title, (char *) translation, flags, TF_DIALOG);
		if ((lstTranslations.DynamicTranslations()) && (index >= 0))
		{
			//SetWindowTextW(hWnd, lstTranslations[index]->translation);
			SetWindowTextW(hWnd, lstTranslations[index]->dynamic->wszDynamic);
		}
	}
}

void HandleTranslateMenu(const char *data, WPARAM wParam, LPARAM lParam)
{
	HMENU        hMenu = ( HMENU )wParam;
	int          i;
	MENUITEMINFOW mii;
	WCHAR        str[256];
	WCHAR        *translation;

	mii.cbSize = MENUITEMINFO_V4_SIZE;
	for ( i = GetMenuItemCount( hMenu )-1; i >= 0; i--)
	{
		mii.fMask = MIIM_TYPE | MIIM_SUBMENU;
		mii.dwTypeData = ( WCHAR *) str;
		mii.cch = SIZEOF(str);
		GetMenuItemInfoW(hMenu, i, TRUE, &mii);

		if ( mii.cch && mii.dwTypeData )
		{
			translation = (WCHAR *) realCallServiceFunction(MS_LANGPACK_TRANSLATESTRING, LANG_UNICODE, (LPARAM) str);
			int index = AddTranslatableStringToList(data, (const char *) mii.dwTypeData, (char *) translation, LANG_UNICODE, TF_MENU);
			if ((lstTranslations.DynamicTranslations()) && (index >= 0)) 
			{
				//mii.dwTypeData = lstTranslations[index]->translation;
				mii.dwTypeData = lstTranslations[index]->dynamic->wszDynamic;
				mii.fMask = MIIM_TYPE;
				SetMenuItemInfoW(hMenu, i, TRUE, &mii);
			}
		}
	}

	if ( mii.hSubMenu != NULL ) HandleTranslateMenu(data, ( WPARAM )mii.hSubMenu, lParam);
	
	return;
}

static BOOL CALLBACK TranslateDialogEnumProc(HWND hwnd,LPARAM lParam)
{
	LANGPACKTRANSLATEDIALOG *lptd=(LANGPACKTRANSLATEDIALOG*)lParam;
	WCHAR szClass[32];
	int id = GetDlgCtrlID( hwnd );

	GetClassNameW(hwnd,szClass,SIZEOF(szClass));
	if(!lstrcmpiW(szClass, L"static") || !lstrcmpiW(szClass, L"hyperlink") || !lstrcmpiW(szClass, L"button") || !lstrcmpiW(szClass, L"MButtonClass"))
		TranslateWindow((char *) lParam, hwnd, LANG_UNICODE);
	else if(!lstrcmpiW(szClass, L"edit")) {
		if(lptd->flags&LPTDF_NOIGNOREEDIT || GetWindowLong(hwnd,GWL_STYLE)&ES_READONLY)
			TranslateWindow((char *) lParam, hwnd, LANG_UNICODE);
	}
	return TRUE;
}

void HandleTranslateDialog(const char *data, WPARAM wParam, LPARAM lParam)
{//taken from lpservices.c
	LANGPACKTRANSLATEDIALOG *lptd=(LANGPACKTRANSLATEDIALOG*)lParam;
	if(lptd==NULL||lptd->cbSize!=sizeof(LANGPACKTRANSLATEDIALOG)) return;
	if(!(lptd->flags&LPTDF_NOTITLE))
		TranslateWindow(data, lptd->hwndDlg, LANG_UNICODE );

	EnumChildWindows(lptd->hwndDlg,TranslateDialogEnumProc, (LPARAM) data);
}

INT_PTR TranslatorCallServiceFunction(const char *name, WPARAM wParam, LPARAM lParam)
{
	DWORD hash = NameHashFunction(name);

	if (hash == TRANSLATEDIALOG_HASH)
	{
		HandleTranslateDialog(name, wParam, lParam);
	}
	else if (hash == TRANSLATEMENU_HASH)
	{
		HandleTranslateMenu(name, wParam, lParam);
	}
	
	INT_PTR res = realCallServiceFunction(name, wParam, lParam);
	
	if (hash == TRANSLATESTRING_HASH)
	{
		int index = AddTranslatableStringToList(name, (char *) lParam, (char *) res, wParam, ((wParam & LANG_UNICODE) != 0) ? TF_WIDE : TF_DEFAULT);
		if ((lstTranslations.DynamicTranslations()) && (index >= 0))
		{
			//res = (DWORD) lstTranslations[index]->translation;
			if ((wParam & LANG_UNICODE) != 0)
			{
				res = (INT_PTR) lstTranslations[index]->dynamic->wszDynamic;
			}
			else{
				res = (INT_PTR) lstTranslations[index]->dynamic->szDynamic;
			}
		}
	}

	return res;
}


INT_PTR TranslatorShowManagerService(WPARAM wParam, LPARAM lParam)
{
	if (lstTranslations.TranslationsMode())
	{
		if (hWndTranslations == NULL)
		{
			hWndTranslations = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_TRANSLATIONS), NULL, DlgProcTranslations);
		}
		ShowWindow(hWndTranslations, SW_SHOW);
	}
	
	return 0;
}

#include "DbgHelp.h"

//only gets called if we hacked CallService, so szEnglish and szTranslation are valid.
int AddTranslatableStringToList(const char *data, const char *english, const char *translation, int type, DWORD flags)
{
	EnterCriticalSection(&csServices);
	MEMORY_BASIC_INFORMATION mbi;
	HINSTANCE hInst;
	WCHAR section[256];
	
	char mod[256];
	
	strcpy(mod, "<Unknown>");
	if (VirtualQuery(data, &mbi, sizeof(mbi)))
	{
		hInst = (HINSTANCE) mbi.AllocationBase;
		char mod_path[MAX_PATH];
		GetModuleFileName(hInst, mod_path, sizeof(mod_path));
		char *p = strrchr(mod_path, '\\');
		if (p)
		{
			strncpy(mod, ++p, sizeof(mod));
		}
	}
		
	{
		MultiByteToWideChar(CP_ACP, 0, mod, -1, section, 256);
		WCHAR *p = wcsrchr(section, L'.');
		if (p)
		{
			p[0] = L'\0';
		}
	}
	
	//wcscpy(szEnglish, L"\"");
	//wcscpy(szTranslation, L"\"");
	szEnglish[0] = szTranslation[0] = L'\0';
	
	if ((type & LANG_UNICODE) != 0)
	{
		if (english) { wcscat(szEnglish, (WCHAR *) english); }
		if (translation) { wcscat(szTranslation, (WCHAR *) translation); }
	}
	else{
		WCHAR temp[MAX_SIZE];
		MultiByteToWideChar(CP_ACP, 0, (english) ? english : "", -1, temp, MAX_SIZE);
		temp[MAX_SIZE - 1] = L'\0';
		wcscat(szEnglish, temp);
		
		MultiByteToWideChar(ACTIVE_CODEPAGE, 0, (char *) (translation) ? translation : "", -1, temp, MAX_SIZE);
		temp[MAX_SIZE - 1] = L'\0';
		wcscat(szTranslation, temp);
	}
	//wcscat(szEnglish, L"\"");
	//wcscat(szTranslation, L"\"");
	
	ShowSpecialChars(szEnglish);
	ShowSpecialChars(szTranslation);
	
	INT_PTR index = lstTranslations.Add(szEnglish, szTranslation, section, flags);
	
	LeaveCriticalSection(&csServices);

	return index;
}

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

#include "translation_utils.h"

LanguagePackStruct languagePack; // = {0};

DWORD LangPackHashW(const wchar_t *szStr)
{
#if defined _M_IX86 && !defined _NUMEGA_BC_FINALCHECK && !defined __GNUC__
	__asm {				//this is mediocrely optimised, but I'm sure it's good enough
		xor  edx,edx
		mov  esi,szStr
		xor  cl,cl
lph_top:
		xor  eax,eax
		and  cl,31
		mov  al,[esi]
		inc  esi
		inc  esi
		test al,al
		jz   lph_end
		rol  eax,cl
		add  cl,5
		xor  edx,eax
		jmp  lph_top
lph_end:
		mov  eax,edx
	}
#else
	DWORD hash=0;
	int i;
	int shift=0;
	for(i=0;szStr[i];i+=2) {
		hash^=szStr[i]<<shift;
		if(shift>24) hash^=(szStr[i]>>(32-shift))&0x7F;
		shift=(shift+5)&0x1F;
	}
	return hash;
#endif
}

DWORD LangPackHash(const char *szStr)
{
#if defined _M_IX86 && !defined _NUMEGA_BC_FINALCHECK && !defined __GNUC__
	__asm {				//this is mediocrely optimised, but I'm sure it's good enough
		xor  edx,edx
		mov  esi,szStr
		xor  cl,cl
lph_top:
		xor  eax,eax
		and  cl,31
		mov  al,[esi]
		inc  esi
		test al,al
		jz   lph_end
		rol  eax,cl
		add  cl,5
		xor  edx,eax
		jmp  lph_top
lph_end:
		mov  eax,edx
	}
#else
	DWORD hash=0;
	int i;
	int shift=0;
	for(i=0;szStr[i];i++) {
		hash^=szStr[i]<<shift;
		if(shift>24) hash^=(szStr[i]>>(32-shift))&0x7F;
		shift=(shift+5)&0x1F;
	}
	return hash;
#endif
}

int SkipBOM(FILE *fin)
{
	int res = -1;
	int BOM = fgetwc(fin);
	if (BOM != EOF)
	{
		res = 1;
		if ((BOM != 0xFFFE) && (BOM != 0xFEFF))
		{
			fseek(fin, 0, SEEK_SET);
			res = 0;
		}
	}
	
	return res;
}

int WriteTranslation(wchar_t *english, wchar_t *translation, wchar_t *fileName)
{
	FILE *fin = _wfopen(fileName, L"a+, ccs=UTF-16LE");
	const int MAX_SIZE = 4096;
	int found = 0;
	size_t len;
	wchar_t buffer[MAX_SIZE];
	wchar_t *p;
	
	if (fin)
	{
		fseek(fin, 0, SEEK_SET); //move to the start of the file
		SkipBOM(fin);
		while (!feof(fin)) //check to see if the string is already present
		{
			p = fgetws(buffer, MAX_SIZE, fin);
			if (p)
			{
				len = wcslen(buffer);
				if (buffer[len - 1] == L'\n') buffer[len-- - 1] = L'\0';
				if ((buffer[0] == L'[') && (buffer[len - 1] == L']'))
				{
					buffer[len - 1] = L'\0';
					p++;
					if (_wcsicmp(p, english) == 0)
					{
						found = 1;
						break;
					}
				}
			}
		}
		
		if (!found)
		{
			fwprintf(fin, L"[%s]\n%s\n", english, translation);
		}
		
		fclose(fin);
	}
	
	return 0;
}

void ShowSpecialChars(wchar_t *string)
{
	StrReplace(string, L"\r\n", L"\\n");
	StrReplace(string, L"\n", L"\\n");
	StrReplace(string, L"\r", L"\\r");
	StrReplace(string, L"\t", L"\\t");
}

void ParseSpecialChars(WCHAR *string)
{
	StrReplace(string, L"\\n", L"\r\n");
	StrReplace(string, L"\\r", L"\r\n");
	StrReplace(string, L"\\t", L"\t");
}

static void TrimStringSimple(char *str) 
{
	if (str[lstrlenA(str)-1] == '\n') str[lstrlenA(str)-1] = '\0';
	if (str[lstrlenA(str)-1] == '\r') str[lstrlenA(str)-1] = '\0';
}

static void TrimStringSimple(wchar_t *str)
{
	if (str[lstrlenW(str)-1] == L'\n') str[lstrlenW(str)-1] = L'\0';
	if (str[lstrlenW(str)-1] == L'\r') str[lstrlenW(str)-1] = L'\0';
}

static int IsEmpty(char *str)
{
	int i = 0;

	while (str[i])
	{
		if (str[i]!=' '&&str[i]!='\r'&&str[i]!='\n')
			return 0;
		i++;
	}
	return 1;
}

static int IsEmpty(wchar_t *str)
{
	int i = 0;
	while (str[i])
	{
		if (str[i]!=' '&&str[i]!='\r'&&str[i]!='\n')
			return 0;
		i++;
	}
	return 1;
}

static void TrimString(char *str)
{
	int len,start;
	len=lstrlenA(str);
	while(str[0] && (unsigned char)str[len-1]<=' ') str[--len]=0;
	for(start=0;str[start] && (unsigned char)str[start]<=' ';start++);
	MoveMemory(str,str+start,len-start+1);
}

DWORD WINAPI ReadCurrentTranslations(void *param)
{
	WIN32_FIND_DATA findData;
	HANDLE hFindData;
	char langpackPath[MAX_PATH], *pos;
	POPUPDATA pu = {0};
	pu.lchIcon = hiTranslator;
	mir_snprintf(pu.lptzContactName, MAX_CONTACTNAME, "%s", Translate("Translator"));
	mir_snprintf(pu.lptzText, MAX_SECONDLINE, "%s", Translate("Starting to read language pack file and previous translations."));
	
	PUAddPopUp(&pu);
	
	GetModuleFileName(GetModuleHandle(NULL), langpackPath, MAX_PATH);
	pos = strrchr(langpackPath, '\\');
	if (pos)
	{
		strcpy(++pos, "langpack_*.txt");
		
		if ((hFindData = FindFirstFile(langpackPath, &findData)) != INVALID_HANDLE_VALUE)
		{
			strcpy(pos, findData.cFileName);
			ReadTranslationsFile(langpackPath);
			FindClose(hFindData);
		}
	}
	
	ReadSavedTranslations();
	
	mir_snprintf(pu.lptzText, MAX_SECONDLINE, "%s", Translate("Finished reading language pack file and previous translations."));
	
	PUAddPopUp(&pu);
	
	return 0;
}


DWORD GetCP(char *locale)
{
	char szBuf[20], *stopped;
	
	USHORT langID = (USHORT) strtol(locale, &stopped, 16);
	LCID localeID = MAKELCID(langID, 0);
	GetLocaleInfoA(localeID, LOCALE_IDEFAULTANSICODEPAGE, szBuf, 10);
	szBuf[5] = 0;                       // codepages have max. 5 digits
	return atoi(szBuf);
}

int ReadTranslationsFile(char *transFileName)
{
	FILE *fin;
	fin = fopen(transFileName, "rt");
	const int MAX_LINE_SIZE = 10240;
	char line[MAX_LINE_SIZE], *szColon;
	
	int bEngTextAvailable = 0;
	int len;
	static wchar_t english[MAX_LINE_SIZE], translation[MAX_LINE_SIZE];
	
	int startOfLine = 0;
	
	if (fin)
	{
		fgets(line, MAX_LINE_SIZE, fin);
		TrimString(line);
		if (lstrcmp(line, "Miranda Language Pack Version 1"))
		{
			fclose(fin);
			
			return -1;
		}
		
		//header
		while(!feof(fin))
		{
			startOfLine = ftell(fin);
			if (fgets(line, MAX_LINE_SIZE, fin) == NULL) { break; }
			TrimString(line);
			if(IsEmpty(line) || line[0] == 0)  { continue; }
			if (line[0] == ';')
			{
				if (strstr(line, "; FLID:") == line) //updater FLID string ?
				{
					//mir_snprintf(languagePack.FLID, sizeof(languagePack.FLID), "%s", line + 7);
					languagePack.FLID = _strdup(line + 7);
					TrimString(languagePack.FLID);
				}
				
				continue; //just a regular comment
			}
			if(line[0] == '[') { break; }
			szColon = strchr(line,':');
			if(szColon == NULL)
			{
				fclose(fin);
				
				return -2;
			}
			
			*szColon = 0;
			if(!lstrcmpA(line,"Language"))
			{
				//mir_snprintf(languagePack.language, sizeof(languagePack.language),"%s", szColon + 1);
				languagePack.language = _strdup(szColon + 1);
				TrimString(languagePack.language);
			}
			//else if(!lstrcmpA(line,"Last-Modified-Using"))
			//{
			//	mir_snprintf(languagePack.lastModifiedUsing, sizeof(languagePack.lastModifiedUsing), "%s", szColon + 1);
			//	TrimString(languagePack.lastModifiedUsing);
			//}
			else if(!lstrcmpA(line,"Authors"))
			{
				size_t len = (!languagePack.authors) ? 0 : strlen(languagePack.authors);
				size_t size;
				char *format = ((!len) || (languagePack.authors[len - 1] == ',') || (szColon[0] == ',')) ? "%s" : ", %s";
				
				size = len + strlen(szColon + 1) + 1;
				languagePack.authors = (char *) realloc(languagePack.authors, size);
				mir_snprintf(languagePack.authors + len, size - len, format, szColon + 1);
				TrimString(languagePack.authors);
			}
			else if(!lstrcmpA(line,"Author-email"))
			{
				size_t len = (!languagePack.authorEmail) ? 0 : strlen(languagePack.authorEmail);
				size_t size;
				char *format = ((!len) || (languagePack.authorEmail[len - 1] == ',') || (szColon[0] == ',')) ? "%s" : ", %s";
				size = (len) ? (strlen(languagePack.authorEmail) + len + 1) : (strlen(szColon + 1) + 1);
				languagePack.authorEmail = (char *) realloc(languagePack.authorEmail, size);
				mir_snprintf(languagePack.authorEmail + len, size - len, format, szColon + 1);
				TrimString(languagePack.authorEmail);
			}
			else if (!lstrcmpA(line, "Plugins-included"))
			{
				languagePack.plugins = _strdup(szColon + 1);
				_strlwr(languagePack.plugins);
				TrimString(languagePack.plugins);
			}
			else if(!lstrcmpA(line, "Locale"))
			{
				//char szBuf[20], *stopped;
				
				TrimString(szColon + 1);
				//mir_snprintf(languagePack.locale, sizeof(languagePack.locale), "%s", szColon + 1); //copy the locale, we'll need it in the export dialog.
				languagePack.locale = _strdup(szColon + 1);
			//	langID = (USHORT) strtol(szColon + 1, &stopped, 16);
			//	languagePack.localeID = MAKELCID(langID, 0);
			//	GetLocaleInfoA(languagePack.localeID, LOCALE_IDEFAULTANSICODEPAGE, szBuf, 10);
			//	szBuf[5] = 0;                       // codepages have max. 5 digits
			//	languagePack.cpANSI = atoi(szBuf);
			}
		}
		
		fseek(fin, startOfLine, SEEK_SET);
		while (!feof(fin))
		{
			if (fgets(line, MAX_LINE_SIZE, fin) == NULL) { break; }
			if(IsEmpty(line) || line[0] == ';' || line[0] == 0)  { continue; }
			TrimStringSimple(line);
			len = lstrlen(line);
			if ((line[0] == '[') && (line[len - 1] == ']'))
			{
				line[len - 1] = '\0';
				bEngTextAvailable = 1; //skip the ]
				MultiByteToWideChar(CP_ACP, 0, line + 1, -1, english, MAX_LINE_SIZE); //skip the [
				StrReplace(english, L"\t", L"\\t");
			}
			else{
				if (bEngTextAvailable)
				{
					MultiByteToWideChar(ACTIVE_CODEPAGE, 0, line, -1, translation, MAX_LINE_SIZE);
					StrReplace(english, L"\t", L"\\t");
					lstTranslations.Add(english, translation, L"", TF_LANGPACK, 1);
				}
				bEngTextAvailable = 0;
			}
		}
		fclose(fin);
	}
	
	return 0;
}

int ReadPreviousTranslations(wchar_t *fileName)
{
	FILE *fin = _wfopen(fileName, L"rt, ccs=UTF-16LE");
	const int MAX_LINE_SIZE = 4096;
	int len = GetLastError();
	
	if (fin)
	{
		int bEngTextAvailable = 0;
		wchar_t line[MAX_LINE_SIZE];
		wchar_t english[MAX_LINE_SIZE];
		wchar_t module[512], *pos;
		
		pos = wcsrchr(fileName, L'\\');
		if ((pos) && (wcsstr(fileName, LANGUAGEPACK_MODIFIED) == 0))
		{
			wcscpy(module, pos + 1);
			pos = wcsrchr(module, L'.');
			if (pos) { *pos = L'\0'; }
		}
		else{
			wcscpy(module, L"");
		}
	
		fseek(fin, 0, SEEK_SET);
		SkipBOM(fin);
		while (!feof(fin))
		{
			if (fgetws(line, MAX_LINE_SIZE, fin) == NULL) { break; }
			if(IsEmpty(line) || line[0] == ';' || line[0] == 0)  { continue; }
			TrimStringSimple(line);
			len = lstrlenW(line);
			if ((line[0] == L'[') && (line[len - 1] == L']'))
			{
				line[len - 1] = L'\0';
				bEngTextAvailable = 1; //skip the ]
				wcscpy(english, line + 1); //both strings have the same size, so it should fit
			}
			else{
				if (bEngTextAvailable)
				{
					lstTranslations.Add(english, line, module, 0, 1);
				}
				bEngTextAvailable = 0;
			}
		}
		
		fclose(fin);
	} 
	
	return 0;
}

int ReadSavedTranslations()
{
	const int MAX_SIZE = 1024;
	wchar_t searchFolder[MAX_SIZE];
	wchar_t *pos;
	if (!wcschr(TRANSLATIONS_FOLDER, ':'))
	{
		GetModuleFileNameW(GetModuleHandle(NULL), searchFolder, MAX_SIZE);
		pos = wcsrchr(searchFolder, L'\\') + 1;
		wcscpy(pos, TRANSLATIONS_FOLDER);
		wcscat(searchFolder, L"\\");
		pos = searchFolder + wcslen(searchFolder);
	}
	else{
		wcscpy(searchFolder, TRANSLATIONS_FOLDER);
		wcscat(searchFolder, L"\\");
		pos = searchFolder + wcslen(searchFolder);
	}
	wcscat(searchFolder, L"*.txt");
	WIN32_FIND_DATAW findData;
	HANDLE hFindData;
	
	if ((hFindData = FindFirstFileW(searchFolder, &findData)) != INVALID_HANDLE_VALUE)
	{
		do
		{
			wcscpy(pos, findData.cFileName);
			ReadPreviousTranslations(searchFolder);
		}
		while (FindNextFileW(hFindData, &findData));
		FindClose(hFindData);
	}
	
	return 0;
}


int fputsnl(const char *buffer, FILE *fout)
{
	int res = -1;
	if (buffer)
	{
		size_t len = strlen(buffer);
		if (len)
		{
			if (buffer[len - 1] == '\n')
			{
				fputs(buffer, fout);
				
				res = 0;
			}
			else{
				fputs(buffer, fout);
				fputc('\n', fout);
				
				res = 1;
			}
		}
		else{
			fputc('\n', fout);
		}
	}
	
	return res;
}

int ExportLanguagePack(HWND hExportWnd)
{
	OSVERSIONINFO vi = {0};
	vi.dwOSVersionInfoSize = sizeof(vi);
	GetVersionEx(&vi);
	int BUFFER_SIZE;
	int MAX_SIZE; //BUFFER_SIZE / 2 - 1
	if (vi.dwMajorVersion == 4)
	{
		BUFFER_SIZE = 20000;
	}
	else{
		BUFFER_SIZE = 1024 * 1024; //this should be enough :)
	}
	MAX_SIZE = (BUFFER_SIZE - 1) / 2 - 1;
	
	char *buffer = (char *) malloc(BUFFER_SIZE);
	if (!buffer) { return - 1; }
	DWORD CP = CP_ACP;
	GetWindowText(GetDlgItem(hExportWnd, IDC_OUTPUT), buffer, BUFFER_SIZE);
	if (strlen(buffer) > 0)
	{
		FILE *fout = fopen(buffer, "w");
		if (fout)
		{
			fputsnl("Miranda Language Pack Version 1", fout);
			
			GetWindowText(GetDlgItem(hExportWnd, IDC_LANGUAGE), buffer, BUFFER_SIZE);
			fputs("Language: ", fout);
			fputsnl(buffer, fout);
			
			GetWindowText(GetDlgItem(hExportWnd, IDC_LOCALE), buffer, BUFFER_SIZE);
			fputs("Locale: ", fout);
			if (strlen(buffer) > 0)
			{
				CP = GetCP(buffer);
			}
			else{
				CP = ACTIVE_CODEPAGE;
			}
			fputsnl(buffer, fout);
			
			CallService(MS_SYSTEM_GETVERSIONTEXT, (WPARAM) BUFFER_SIZE, (LPARAM) buffer);
			fputs("Last-Modified-Using: Miranda IM ", fout);
			fputs(buffer, fout);
			fputsnl(" (Translator plugin " __VERSION_STRING ")", fout);
			
			GetWindowText(GetDlgItem(hExportWnd, IDC_AUTHORS), buffer, BUFFER_SIZE);
			fputs("Authors: ", fout);
			fputsnl(buffer, fout);
			
			GetWindowText(GetDlgItem(hExportWnd, IDC_EMAILS), buffer, BUFFER_SIZE);
			fputs("Author-email: ", fout);
			fputsnl(buffer, fout);
			
			GetWindowText(GetDlgItem(hExportWnd, IDC_PLUGINSINCLUDED), buffer, BUFFER_SIZE);
			fputs("Plugins-included: ", fout);
			StrReplace(buffer, "\r\n", " ");
			StrReplace(buffer, "\n", " ");
			StrReplace(buffer, "\r", " ");
			fputsnl(buffer, fout);
			
			GetWindowText(GetDlgItem(hExportWnd, IDC_FLID), buffer, BUFFER_SIZE);
			fputs("; FLID: ", fout);
			fputsnl(buffer, fout);
			
			GetWindowText(GetDlgItem(hExportWnd, IDC_COMMENTS), buffer, BUFFER_SIZE);
			fputs("\n;", fout);
			StrReplace(buffer, "\r\n", "\n;");
			fputsnl(buffer, fout);
			fputs("\n", fout);
			
			int bLPIgnoreUntranslated = DBGetContactSettingByte(NULL, ModuleName, "LPIgnoreUntranslated", 0);
			
			TTranslation *translation;
			TTranslation *prev = NULL;
			int i;
			char *eng = (char *) malloc(MAX_SIZE * sizeof(char));
			char *trans = (char *) malloc(MAX_SIZE * sizeof(char));
			
			if ((eng) && (trans))
			{
				for (i = 0; i < lstTranslations.Count(); i++)
				{
					translation = lstTranslations[i];
					
					if (bLPIgnoreUntranslated)
					{
						DWORD hEng = LangPackHashW(translation->english);
						DWORD hTrn = LangPackHashW(translation->translation);
						
						if (hEng == hTrn) { continue; } //jump to the next entry
					}

					if ((!prev) || ((prev->hEnglish != translation->hEnglish) || (prev->hTranslation != translation->hTranslation))) //ignore duplicates
					{
						WideCharToMultiByte(CP, 0, translation->english, -1, eng, MAX_SIZE, NULL, NULL);
						WideCharToMultiByte(CP, 0, translation->translation, -1, trans, MAX_SIZE, NULL, NULL);
						
						mir_snprintf(buffer, BUFFER_SIZE, "[%s]\n%s", eng, trans);
						fputsnl(buffer, fout);
					}
					
					prev = translation;
				}
			}
			else{
				MessageBox(0, Translate("Not enough memory to create export buffers."), Translate("Error"), MB_OK | MB_ICONERROR);
			}
			
			if (eng) { free(eng); }
			if (trans) { free(trans); }
			
			fclose(fout);
		}
	}	
	
	free(buffer);

	return 0;
}
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

#ifndef M_TRANSLATOR_TRANSLATION_UTILS_H
#define M_TRANSLATOR_TRANSLATION_UTILS_H

#define LANGUAGEPACK_MODIFIED L"language pack - modified"

#include "commonheaders.h"

struct LanguagePackStruct{
	char *language;
	//char lastModifiedUsing[LP_SIZE];
	char *authors;
	char *authorEmail;
	char *locale;
	LCID localeID;
	DWORD cpANSI;
	char *FLID;
	char *plugins;
	
	public:
	LanguagePackStruct()
	{
		plugins = language = authors = authorEmail = locale = FLID = NULL;
		localeID = cpANSI = 0;
	}
	
	~LanguagePackStruct()
	{
		if (language)
		{
			free(language);
		}
		
		if (authors)
		{
			free(authors);
		}
		
		if (authorEmail)
		{
			free(authorEmail);
		}
		
		if (locale)
		{
			free(locale);
		}
		
		if (FLID)
		{
			free(FLID);
		}
		
		if (plugins)
		{
			free(plugins);
		}
		
	}
	
};

extern LanguagePackStruct languagePack;

DWORD LangPackHashW(const wchar_t *szStr);
DWORD LangPackHash(const char *szStr);

int SkipBOM(FILE *fin);

int WriteTranslation(wchar_t *english, wchar_t *translation, wchar_t *fileName);

void ShowSpecialChars(wchar_t *string);
void ParseSpecialChars(WCHAR *string);

DWORD WINAPI ReadCurrentTranslations(void *param);
int ReadTranslationsFile(char *transFileName);
int ReadSavedTranslations();

int ExportLanguagePack(HWND hExportWnd);

#endif //M_TRANSLATOR_TRANSLATION_UTILS_H
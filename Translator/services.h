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

#ifndef M_TRANSLATOR_SERVICES_H
#define M_TRANSLATOR_SERVICES_H

#include "commonheaders.h"

#define MS_TRANSLATOR_SHOWMANAGER "Translator/ShowManager"

#define SIZEOF(X) (sizeof(X)/sizeof(X[0]))
#define MENUITEMINFO_V4_SIZE (offsetof(MENUITEMINFO,cch)+sizeof((*((MENUITEMINFO*)0)).cch))

extern wchar_t TRANSLATIONS_FOLDER[];
#define ACTIVE_CODEPAGE languagePack.cpANSI

extern DWORD TRANSLATESTRING_HASH;
extern DWORD TRANSLATEMENU_HASH;
extern DWORD TRANSLATEDIALOG_HASH;

typedef INT_PTR (*CALLSERVICEFUNCTION) (const char *, WPARAM wParam, LPARAM lParam);

extern CALLSERVICEFUNCTION realCallServiceFunction;

int InitServices();
int DestroyServices();

void HookRealServices();
void UnhookRealServices();

INT_PTR TranslatorCallServiceFunction(const char *name, WPARAM wParam, LPARAM lParam);
INT_PTR TranslatorShowManagerService(WPARAM wParam, LPARAM lParam);

int AddTranslatableStringToList(const char *data, const char *szEnglish, const char *szTranslation, int type, DWORD flags);

#endif //M_SERVICESLIST_SERVICES_H

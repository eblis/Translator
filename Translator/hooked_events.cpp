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

#include "hooked_events.h"

HANDLE hModulesLoaded;
HANDLE hFolderChanged;
HANDLE hOptionsInitialize;
//HANDLE hPreShutDown;

HANDLE hTranslationsFolder = NULL;
int bModulesLoaded = 0;

#define HOST "http://eblis.tla.ro/projects"

#if defined(WIN64) || defined(_WIN64)
#define TRANSLATOR_VERSION_URL HOST "/miranda/Translator/updater/x64/Translator.html"
#define TRANSLATOR_UPDATE_URL HOST "/miranda/Translator/updater/x64/Translator.zip"
#else
#define TRANSLATOR_VERSION_URL HOST "/miranda/Translator/updater/Translator.html"
#define TRANSLATOR_UPDATE_URL HOST "/miranda/Translator/updater/Translator.zip"
#endif
#define TRANSLATOR_VERSION_PREFIX "Translator version "

int HookEvents()
{
	Log("%s", "Entering function " __FUNCTION__);
	hModulesLoaded = HookEvent(ME_SYSTEM_MODULESLOADED, OnModulesLoaded);
	hOptionsInitialize = HookEvent(ME_OPT_INITIALISE, OnOptionsInitialise);
	//hPreShutDown = HookEvent(ME_SYSTEM_PRESHUTDOWN, OnPreShutDown);
	//hPreShutdown = HookEvent(ME_SYSTEM_PRESHUTDOWN, OnPreShutdown);
	Log("%s", "Leaving function " __FUNCTION__);
	
	return 0;
}

int UnhookEvents()
{
	Log("%s", "Entering function " __FUNCTION__);
	UnhookEvent(hModulesLoaded);
	UnhookEvent(hFolderChanged);
	UnhookEvent(hOptionsInitialize);
	//UnhookEvent(hPreShutDown);
	Log("%s", "Leaving function " __FUNCTION__);
	
	return 0;
}

int OnModulesLoaded(WPARAM wParam, LPARAM lParam)
{
	char mod[MAX_PATH];
	GetModuleFileName(NULL, mod, sizeof(mod));
	char *p = strrchr(mod, '\\');
	p[0] = '\0';
	hTranslationsFolder = (HANDLE) FoldersRegisterCustomPathW(ModuleName, "Translations folder", L"%miranda_path%\\Translations");
	FoldersGetCustomPathW(hTranslationsFolder, TRANSLATIONS_FOLDER, 512, L"Translations");
	bModulesLoaded = 1;

	hFolderChanged = HookEvent(ME_FOLDERS_PATH_CHANGED, OnFolderChanged);
	
	if (lstTranslations.TranslationsMode())
	{
		DWORD threadID;
		HANDLE thread;
		thread = CreateThread(NULL, NULL, ReadCurrentTranslations, NULL, 0, &threadID);
		if ((thread != NULL) && (thread != INVALID_HANDLE_VALUE))
		{
			CloseHandle(thread);
		}
		
		CLISTMENUITEM cl = {0};
		cl.cbSize = sizeof(CLISTMENUITEM);
		cl.hIcon = hiTranslator;
		cl.position = 20000000;
		cl.pszService = MS_TRANSLATOR_SHOWMANAGER;
		cl.pszName = Translate("Translations manager");
		CallService(MS_CLIST_ADDMAINMENUITEM, 0, (LPARAM) &cl);
	}
	
	char buffer[1024];
	Update update = {0};
	update.cbSize = sizeof(Update);
	update.szComponentName = __PLUGIN_DISPLAY_NAME;
	update.pbVersion = (BYTE *) CreateVersionString(VERSION, buffer);
	update.cpbVersion = (int) strlen((char *) update.pbVersion);
	update.szUpdateURL = UPDATER_AUTOREGISTER;
	update.szBetaVersionURL = TRANSLATOR_VERSION_URL;
	update.szBetaUpdateURL = TRANSLATOR_UPDATE_URL;
	update.pbBetaVersionPrefix = (BYTE *) TRANSLATOR_VERSION_PREFIX;
	update.cpbBetaVersionPrefix = (int) strlen(TRANSLATOR_VERSION_PREFIX);
	CallService(MS_UPDATE_REGISTER, 0, (LPARAM) &update);
	
	return 0;
}

int OnFolderChanged(WPARAM wParam, LPARAM lParam)
{
	FoldersGetCustomPathW(hTranslationsFolder, TRANSLATIONS_FOLDER, 512, L"Translations");
	
	return 0;
}

int OnOptionsInitialise(WPARAM wParam, LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp = {0};
	
	odp.cbSize = sizeof(odp);
	odp.position = 100000000;
	odp.hInstance = hInstance;
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_TRANSLATOR);
	odp.ptszTitle = TranslateT("Translator");
	odp.ptszGroup = TranslateT("Services");
	odp.groupPosition = 810000000;
	odp.flags=ODPF_BOLDGROUPS;
	odp.pfnDlgProc = DlgProcOptions;
	CallService(MS_OPT_ADDPAGE, wParam, (LPARAM)&odp);
	
	return 0;
}

//int OnPreShutDown(WPARAM wParam, LPARAM lParam)
//{
//	UnhookRealServices();
//	
//	return 0;
//}
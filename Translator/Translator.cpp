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

#include "commonheaders.h"

char ModuleName[] = "Translator";
HINSTANCE hInstance;

HICON hiTranslator;

PLUGINLINK *pluginLink;

PLUGININFOEX pluginInfo = {
	sizeof(PLUGININFOEX),
	__PLUGIN_DISPLAY_NAME,
	VERSION,
	__DESC,
	__AUTHOR,
	__AUTHOREMAIL,
	__COPYRIGHT,
	__AUTHORWEB,
	1,
	0,
	{0x2fb94425, 0x14a7, 0x4af7, {0xb3, 0xd9, 0x43, 0xfd, 0xf6, 0xcf, 0x28, 0x7d}} //{2fb94425-14a7-4af7-b3d9-43fdf6cf287d}
}; //not used
	
OLD_MIRANDAPLUGININFO_SUPPORT;
	
extern "C" __declspec(dllexport) PLUGININFOEX *MirandaPluginInfoEx(DWORD mirandaVersion) 
{
	if (mirandaVersion < PLUGIN_MAKE_VERSION(0, 6, 0, 0))
	{
		return NULL;
	}
	
	return &pluginInfo;
}

static const MUUID interfaces[] = {MIID_TRANSLATOR, MIID_LAST};

extern "C" __declspec(dllexport) const MUUID *MirandaPluginInterfaces()
{
	return interfaces;
}

extern "C" int __declspec(dllexport) Load(PLUGINLINK *link)
{
	LogInit();
	
	hiTranslator = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TRANSLATOR));
	
	pluginLink = link;
	
	InitServices();
	
	HookEvents();
	
	InitializeMirandaMemFunctions();
	
	return 0;
}

extern "C" int __declspec(dllexport) Unload()
{
	int flush = DBGetContactSettingByte(NULL, ModuleName, "FlushOnExit", 1);
	if (flush)
	{
		lstTranslations.Flush();
	}

	DestroyServices();

	UnhookEvents();
	
	if (hWndTranslations)
	{
		DestroyWindow(hWndTranslations);
	}
	
	return 0;
}

bool WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	hInstance = hinstDLL;
	if (fdwReason == DLL_PROCESS_ATTACH)
		{
			DisableThreadLibraryCalls(hinstDLL);
		}
		
	return TRUE;
}
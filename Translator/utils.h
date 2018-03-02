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

#ifndef M_TRANSLATOR_UTILS_H
#define M_TRANSLATOR_UTILS_H

#include <stdarg.h>
#include "commonheaders.h"

#define LOG_FILE "translator.log"

#define ANCHOR_LEFT     0x000001
#define ANCHOR_RIGHT		0x000002
#define ANCHOR_TOP      0x000004
#define ANCHOR_BOTTOM   0x000008
#define ANCHOR_ALL      ANCHOR_LEFT | ANCHOR_RIGHT | ANCHOR_TOP | ANCHOR_BOTTOM

int LogInit();
int Log(char *format, ...);
int Info(char *title, char *format, ...);

char *BinToHex(int size, PBYTE data);
void HexToBin(char *inData, ULONG &size, PBYTE &outData);

void ScreenToClient(HWND hWnd, LPRECT rect);
void AnchorMoveWindow(HWND window, const WINDOWPOS *parentPos, int anchors);
RECT AnchorCalcPos(HWND window, const RECT *rParent, const WINDOWPOS *parentPos, int anchors);

int GetStringFromDatabase(char *szSettingName, char *szError, char *szResult, size_t size);

TCHAR *GetContactName(HANDLE hContact, char *szProto);
TCHAR *GetContactID(HANDLE hContact);
TCHAR *GetContactID(HANDLE hContact, char *szProto);
HANDLE GetContactFromID(TCHAR *szID, char *szProto);
HANDLE GetContactFromID(TCHAR *szID, wchar_t *szProto);
void GetContactProtocol(HANDLE hContact, char *szProto, size_t size);

DWORD NameHashFunction(const char *szStr);

char *StrReplace(char *source, const char *what, const char *withWhat);
char *StrCopy(char *source, size_t index, const char *what, size_t count);
char *StrDelete(char *source, size_t index, size_t count);
char *StrInsert(char *source, size_t index, const char *what);

wchar_t *StrReplace(wchar_t *source, const wchar_t *what, const wchar_t *withWhat);
wchar_t *StrCopy(wchar_t *source, size_t index, const wchar_t *what, size_t count);
wchar_t *StrDelete(wchar_t *source, size_t index, size_t count);
wchar_t *StrInsert(wchar_t *source, size_t index, const wchar_t *what);

int MyPUShowMessage(char *lpzText, BYTE kind);

static __inline size_t mir_snwprintf(wchar_t *buffer, size_t count, const wchar_t *fmt, ...) {
	va_list va;
	size_t len;

	va_start(va, fmt);
	len = _vsnwprintf(buffer, count - 1, fmt, va);
	va_end(va);
	buffer[count - 1] = 0;
	return len;
}

#endif
/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/
 */

#ifndef GME_H_INCLUDED
#define GME_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <tchar.h>
#include <string.h>
#include <algorithm>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <process.h>

#include <winsock2.h>
#include <ws2tcpip.h>

#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <htmlhelp.h>
#include <stdint.h>
#include <inttypes.h>

/*
  Mini lib for Zip implementation, found here:
    https://code.google.com/archive/p/miniz/
*/
#include "miniz.h"

/*
  xxHash - Extremely Fast Hash algorithm:
    https://github.com/Cyan4973/xxHash
*/
#include "xxhash.h"

typedef unsigned char ubyte;

#include "../resource.h"

#define GPL_HEADER L"\
This program is free software: you canredistribute it and/or modify \
it under the terms of the GNU General Public License as published \
by the Free Software Foundation, either version 3 of the License, or \
(at your option) any later version.\
\n\nThis program is distributed in the hope that it will be useful, \
but WITHOUT ANY WARRANTY; without even the implied warranty of \
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  \
See the GNU General Public License for more details.\
\n\nYou should have received a copy of the GNU General Public License \
along with this program. If not, see http://www.gnu.org/licenses/"

// global defines
#define GME_APP_NAME      L"OvGME"
#define GME_APP_MAJOR     1
#define GME_APP_MINOR     7
#define GME_APP_REVIS     0
#define GME_APP_DATE      L"February 2017"

/* handle for folder changes tracking */
extern HANDLE      g_hChange;

/* global handles to GUI elements */
extern HINSTANCE   g_hInst;
extern HICON       g_hicnMain;
extern HWND        g_hwndMain;
extern HMENU       g_hmnuMain;
extern HMENU       g_hmnuSubProf;
extern HWND        g_hwndAddGame;
extern HWND        g_hwndEdiGame;
extern HWND        g_hwndNewAMod;
extern HWND        g_hwndSnapNew;
extern HWND        g_hwndSnapCmp;
extern HWND        g_hwndUninst;
extern HWND        g_hwndRepUpd;
extern HWND        g_hwndRepXml;
extern HWND        g_hwndRepXts;
extern HWND        g_hwndRepConf;
extern HWND        g_hwndProfNew;
extern HWND        g_hwndProfDel;
extern HWND        g_hwndDebug;

wchar_t* GME_GetVersionString();

#endif // GME_H_INCLUDED

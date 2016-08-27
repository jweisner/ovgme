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

#include "gme.h"

/* handle for folder changes tracking */
HANDLE      g_hChange = 0;

/* global handles to GUI elements */
HINSTANCE   g_hInst = NULL;
HICON       g_hicnMain = NULL;
HWND        g_hwndMain = NULL;
HMENU       g_hmnuMain = NULL;
HWND        g_hwndAddGame = NULL;
HWND        g_hwndEdiGame = NULL;
HWND        g_hwndNewAMod = NULL;
HWND        g_hwndNewAModQ = NULL;
HWND        g_hwndSnapNew = NULL;
HWND        g_hwndSnapCmp = NULL;
HWND        g_hwndRepConf = NULL;
HWND        g_hwndRepUpd = NULL;
HWND        g_hwndRepXml = NULL;

HWND        g_hwndDebug = NULL;
HWND        g_hwndUninst = NULL;

wchar_t version[128];

wchar_t* GME_GetVersionString()
{
  swprintf(version, L"%d.%d.%d - %s", GME_APP_MAJOR, GME_APP_MINOR, GME_APP_REVIS, GME_APP_DATE);
  return version;
}

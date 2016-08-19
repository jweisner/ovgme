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

#include "gme_dlg_debug.h"

HFONT dbg_hFont;
std::string g_DebugLog = "";

 /*
  message callback for snapshot creation
*/
BOOL CALLBACK GME_DlgDebug(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
  case WM_INITDIALOG:
    g_hwndDebug = hwndDlg;
    SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)g_hicnMain);
    dbg_hFont = CreateFont(14,0,0,0,FW_DONTCARE,FALSE,TRUE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
                            CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,VARIABLE_PITCH,TEXT("Courier New"));
    SendMessage(GetDlgItem(hwndDlg, ENT_OUTPUT), WM_SETFONT, (WPARAM)dbg_hFont, 1);
    SetDlgItemTextA(g_hwndDebug, ENT_OUTPUT, g_DebugLog.c_str());
    return true;

  case WM_CLOSE:
    EndDialog(hwndDlg, 0);
    return false;

  case WM_COMMAND:
    switch(LOWORD(wParam))
    {
    case BTN_CLOSE:
      EndDialog(hwndDlg, 0);
      return false;
    }
  }

  return false;
}

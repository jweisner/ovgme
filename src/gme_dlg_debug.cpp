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
#include "gme_logs.h"


/*
  initialization routine for debug logs dialog
*/
void GME_DlgDebugInit()
{
    HFONT courier = CreateFont(14,0,0,0,FW_DONTCARE,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
                            CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,VARIABLE_PITCH,TEXT("Courier New"));

    SendMessage(GetDlgItem(g_hwndRepXts, ENT_OUTPUT), WM_SETFONT, (WPARAM)courier, 1);

    SetDlgItemTextA(g_hwndDebug, ENT_OUTPUT, GME_LogsHistory().c_str());
}

/*
  controls resize & placement routine for debug logs dialog
*/
void GME_DlgDebugResize()
{
  HWND hwd = g_hwndDebug;
  RECT cli;
  GetClientRect(hwd, &cli);

  SetWindowPos(GetDlgItem(hwd, BTN_CLOSE), NULL, cli.right-90, cli.bottom-30, 80, 24, SWP_NOZORDER);

  SetWindowPos(GetDlgItem(hwd, TXT_TITLE), NULL, 10, 10, 300, 15, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, BOX_MAIN), NULL, 10, 30, cli.right-20, cli.bottom-80, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, LAB_RESULT), NULL, 20, 50, 120, 15, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, ENT_OUTPUT), NULL, 20, 65, cli.right-40, cli.bottom-130, SWP_NOZORDER);
}

 /*
  message callback for debug logs dialog
*/
BOOL CALLBACK GME_DlgDebug(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
  case WM_INITDIALOG:
    g_hwndDebug = hwndDlg;
    SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)g_hicnMain);
    GME_DlgDebugInit();
    GME_DlgDebugResize();
    return true;

  case WM_SIZE:
    GME_DlgDebugResize();
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

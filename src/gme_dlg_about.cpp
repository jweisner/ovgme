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

/*
  message callback for About dialog window
*/
BOOL CALLBACK GME_DlgAbout(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
  case WM_INITDIALOG:
    SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)g_hicnMain);
    SetDlgItemTextW(hwndDlg, TXT_VERSION, GME_GetVersionString());
    SetDlgItemTextW(hwndDlg, TXT_GPL, GPL_HEADER);
    return true;

  case WM_CLOSE:
    EndDialog(hwndDlg, 0);
    return true;

  case WM_COMMAND:
    switch(LOWORD(wParam))
    {
    case IDOK:
      EndDialog(hwndDlg, 0);
      return true;
    }
    return true;
  }
  return false;
}

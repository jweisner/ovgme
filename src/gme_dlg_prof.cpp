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
#include "gme_game.h"
#include "gme_prof.h"
#include "gme_tools.h"

/* temporary buffer */
wchar_t tmp_pname[64];


/*
  message callback for New Profile dialog window
*/
BOOL CALLBACK GME_DlgProfNew(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
  case WM_INITDIALOG:
    SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)g_hicnMain);
    g_hwndProfNew = hwndDlg;
    // set defaut entries
    SetDlgItemTextW(hwndDlg, ENT_NAME, L"New Profile");
    // disable Add button
    EnableWindow(GetDlgItem(hwndDlg, BTN_ADD), false);
    return true;

  case WM_CLOSE:
    EndDialog(hwndDlg, 0);
    return true;

  case WM_COMMAND:
    switch(LOWORD(wParam))
    {
    case BTN_ADD:
      GetDlgItemTextW(hwndDlg, ENT_NAME, tmp_pname, 64);
      if(GME_ProfCreate(tmp_pname)) {
        EndDialog(hwndDlg, 0);
      }
      return true;

    case IDCANCEL:
      EndDialog(hwndDlg, 0);
      return true;

    case ENT_NAME:
      // tbool or disable add button
      GetDlgItemTextW(hwndDlg, ENT_NAME, tmp_pname, 64);
      if(wcslen(tmp_pname) > 0) {
        EnableWindow(GetDlgItem(hwndDlg, BTN_ADD), true);
      } else {
        EnableWindow(GetDlgItem(hwndDlg, BTN_ADD), false);
      }
      return true;
    }
    return true;

  }
  return false;
}


/*
  initialization routine for Delete Profile dialog
*/
void GME_DlgProfDelInit()
{
  HWND hcb = GetDlgItem(g_hwndProfDel, CMB_PROFLIST);

  for(unsigned i = 0; i < GME_ProfGetCount(); i++) {
    /* fill combo box */
    SendMessageW(hcb, CB_ADDSTRING, i, (LPARAM)GME_ProfGetName(i).c_str());
  }

  /* select first profile in combo box */
  SendMessageW(hcb, CB_SETCURSEL, 0, 0);
}



/*
  message callback for Delete Profile dialog window
*/
BOOL CALLBACK GME_DlgProfDel(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
  case WM_INITDIALOG:
    SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)g_hicnMain);
    g_hwndProfDel = hwndDlg;
    GME_DlgProfDelInit();
    return true;

  case WM_CLOSE:
    EndDialog(hwndDlg, 0);
    return true;

  case WM_COMMAND:
    switch(LOWORD(wParam))
    {
    case BTN_REM:
      if(GME_ProfDelete()) {
        GME_GameUpdMenu();
        EndDialog(hwndDlg, 0);
      }
      return true;

    case IDCANCEL:
      EndDialog(hwndDlg, 0);
      return true;
    }
  }
  return false;
}

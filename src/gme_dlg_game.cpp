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
#include "gme_tools.h"

bool tmp_b[6];
wchar_t tmp_sname[64];
wchar_t tmp_sroot[260];
wchar_t tmp_sback[260];
wchar_t tmp_smods[260];

/*
  message callback for Edit Game dialog window
*/
BOOL CALLBACK GME_DlgGameEdit(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
  case WM_INITDIALOG:
    g_hwndEdiGame = hwndDlg;
    SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)g_hicnMain);
    if(GME_GameGetCurUseCustBack()) {
      SendMessage(GetDlgItem(hwndDlg, CHK_CUSTBACKUP), BM_SETCHECK, BST_CHECKED, 0);
      SetDlgItemTextW(hwndDlg, ENT_BACKPATH, GME_GameGetCurBackPath().c_str());
      EnableWindow(GetDlgItem(hwndDlg, ENT_BACKPATH), true);
      EnableWindow(GetDlgItem(hwndDlg, BTN_BROWSEBCK), true);
    } else {
      SendMessage(GetDlgItem(hwndDlg, CHK_CUSTBACKUP), BM_SETCHECK, BST_UNCHECKED, 0);
      EnableWindow(GetDlgItem(hwndDlg, ENT_BACKPATH), false);
      EnableWindow(GetDlgItem(hwndDlg, BTN_BROWSEBCK), false);
    }
    // set defaut entries
    SetDlgItemTextW(hwndDlg, ENT_NAME, GME_GameGetCurTitle().c_str());
    SetDlgItemTextW(hwndDlg, ENT_ROOTPATH, GME_GameGetCurRoot().c_str());
    SetDlgItemTextW(hwndDlg, ENT_MODSPATH, GME_GameGetCurModsPath().c_str());
    EnableWindow(GetDlgItem(hwndDlg, BTN_APPLY), true);
    return true;

  case WM_CLOSE:
    EndDialog(hwndDlg, 0);
    return true;

  case WM_COMMAND:
    switch(LOWORD(wParam))
    {
    case CHK_CUSTBACKUP:
      if(SendMessage(GetDlgItem(hwndDlg, CHK_CUSTBACKUP), BM_GETCHECK, 0, 0)) {
        EnableWindow(GetDlgItem(hwndDlg, ENT_BACKPATH), true);
        EnableWindow(GetDlgItem(hwndDlg, BTN_BROWSEBCK), true);
      } else {
        EnableWindow(GetDlgItem(hwndDlg, ENT_BACKPATH), false);
        EnableWindow(GetDlgItem(hwndDlg, BTN_BROWSEBCK), false);
      }
      // tbool or disable edit button
      tmp_b[0] = false; tmp_b[1] = false; tmp_b[2] = false;
      if(SendMessage(GetDlgItem(hwndDlg, CHK_CUSTBACKUP), BM_GETCHECK, 0, 0)) {
        GetDlgItemTextW(hwndDlg, ENT_NAME, tmp_sname, 64);
        if(wcslen(tmp_sname) > 0) tmp_b[0] = true;
        GetDlgItemTextW(hwndDlg, ENT_MODSPATH, tmp_smods, 260);
        if(wcslen(tmp_smods) > 2) tmp_b[1] = true;
        GetDlgItemTextW(hwndDlg, ENT_BACKPATH, tmp_sback, 260);
        if(wcslen(tmp_sback) > 2) tmp_b[2] = true;
        if(tmp_b[0] && tmp_b[1] && tmp_b[2]) {
          EnableWindow(GetDlgItem(hwndDlg, BTN_APPLY), true);
        } else {
          EnableWindow(GetDlgItem(hwndDlg, BTN_APPLY), false);
        }
      } else {
        GetDlgItemTextW(hwndDlg, ENT_NAME, tmp_sname, 64);
        if(wcslen(tmp_sname) > 0) tmp_b[1] = true;
        GetDlgItemTextW(hwndDlg, ENT_MODSPATH, tmp_smods, 260);
        if(wcslen(tmp_smods) > 2) tmp_b[0] = true;
        if(tmp_b[0] && tmp_b[1]) {
          EnableWindow(GetDlgItem(hwndDlg, BTN_APPLY), true);
        } else {
          EnableWindow(GetDlgItem(hwndDlg, BTN_APPLY), false);
        }
      }

      return true;

    case BTN_BROWSEBCK:
      GME_DialogDirChooser(hwndDlg, tmp_sback, 260);
      SetDlgItemTextW(hwndDlg, ENT_BACKPATH, tmp_sback);
      return true;

    case BTN_BROWSEMOD:
      GME_DialogDirChooser(hwndDlg, tmp_smods, 260);
      SetDlgItemTextW(hwndDlg, ENT_MODSPATH, tmp_smods);
      return true;

    case BTN_APPLY:
      GetDlgItemTextW(hwndDlg, ENT_NAME, tmp_sname, 260);
      GetDlgItemTextW(hwndDlg, ENT_MODSPATH, tmp_smods, 260);
      GetDlgItemTextW(hwndDlg, ENT_BACKPATH, tmp_sback, 260);
      if(SendMessage(GetDlgItem(hwndDlg, CHK_CUSTBACKUP), BM_GETCHECK, 0, 0)) {
        if(GME_GameEditCurCfg(tmp_sname, tmp_smods, true, tmp_sback)) {
          EndDialog(hwndDlg, 0);
        }
      } else {
        if(GME_GameEditCurCfg(tmp_sname, tmp_smods, false, tmp_sback)) {
          EndDialog(hwndDlg, 0);
        }
      }
      return true;

    case IDCANCEL:
      EndDialog(hwndDlg, 0);
      return true;

    case ENT_NAME:
    case ENT_MODSPATH:
    case ENT_BACKPATH:
      // tbool or disable edit button
      tmp_b[0] = false; tmp_b[1] = false; tmp_b[2] = false;
      if(SendMessage(GetDlgItem(hwndDlg, CHK_CUSTBACKUP), BM_GETCHECK, 0, 0)) {
        GetDlgItemTextW(hwndDlg, ENT_NAME, tmp_sname, 64);
        if(wcslen(tmp_sname) > 0) tmp_b[0] = true;
        GetDlgItemTextW(hwndDlg, ENT_MODSPATH, tmp_smods, 260);
        if(wcslen(tmp_smods) > 2) tmp_b[1] = true;
        GetDlgItemTextW(hwndDlg, ENT_BACKPATH, tmp_sback, 260);
        if(wcslen(tmp_sback) > 2) tmp_b[2] = true;
        if(tmp_b[0] && tmp_b[1] && tmp_b[2]) {
          EnableWindow(GetDlgItem(hwndDlg, BTN_APPLY), true);
        } else {
          EnableWindow(GetDlgItem(hwndDlg, BTN_APPLY), false);
        }
      } else {
        GetDlgItemTextW(hwndDlg, ENT_NAME, tmp_sname, 64);
        if(wcslen(tmp_sname) > 0) tmp_b[1] = true;
        GetDlgItemTextW(hwndDlg, ENT_MODSPATH, tmp_smods, 260);
        if(wcslen(tmp_smods) > 2) tmp_b[0] = true;
        if(tmp_b[0] && tmp_b[1]) {
          EnableWindow(GetDlgItem(hwndDlg, BTN_APPLY), true);
        } else {
          EnableWindow(GetDlgItem(hwndDlg, BTN_APPLY), false);
        }
      }
      return true;
    }

    return true;

  }
  return false;
}


/*
  message callback for Add Game dialog window
*/
BOOL CALLBACK GME_DlgGameAdd(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
  case WM_INITDIALOG:
    SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)g_hicnMain);
    g_hwndAddGame = hwndDlg;
    // set defaut entries
    SetDlgItemTextW(hwndDlg, ENT_NAME, L"New Config");
    // disable Add button
    EnableWindow(GetDlgItem(hwndDlg, BTN_ADD), false);
    SendMessage(GetDlgItem(hwndDlg, CHK_CUSTBACKUP), BM_SETCHECK, BST_UNCHECKED, 0);
    EnableWindow(GetDlgItem(hwndDlg, ENT_BACKPATH), false);
    EnableWindow(GetDlgItem(hwndDlg, BTN_BROWSEBCK), false);
    return true;

  case WM_CLOSE:
    EndDialog(hwndDlg, 0);
    return true;

  case WM_COMMAND:
    switch(LOWORD(wParam))
    {
    case CHK_CUSTBACKUP:
      if(SendMessage(GetDlgItem(hwndDlg, CHK_CUSTBACKUP), BM_GETCHECK, 0, 0)) {
        EnableWindow(GetDlgItem(hwndDlg, ENT_BACKPATH), true);
        EnableWindow(GetDlgItem(hwndDlg, BTN_BROWSEBCK), true);
      } else {
        EnableWindow(GetDlgItem(hwndDlg, ENT_BACKPATH), false);
        EnableWindow(GetDlgItem(hwndDlg, BTN_BROWSEBCK), false);
      }
      // tbool or disable add button
      tmp_b[0] = false; tmp_b[1] = false; tmp_b[2] = false; tmp_b[3] = false;
      if(SendMessage(GetDlgItem(hwndDlg, CHK_CUSTBACKUP), BM_GETCHECK, 0, 0)) {
        GetDlgItemTextW(hwndDlg, ENT_NAME, tmp_sname, 64);
        if(wcslen(tmp_sname) > 0) tmp_b[0] = true;
        GetDlgItemTextW(hwndDlg, ENT_ROOTPATH, tmp_sroot, 260);
        if(wcslen(tmp_sroot) > 3) tmp_b[1] = true;
        GetDlgItemTextW(hwndDlg, ENT_MODSPATH, tmp_smods, 260);
        if(wcslen(tmp_smods) > 3) tmp_b[2] = true;
        GetDlgItemTextW(hwndDlg, ENT_BACKPATH, tmp_sback, 260);
        if(wcslen(tmp_sback) > 3) tmp_b[3] = true;
        if(tmp_b[0] && tmp_b[1] && tmp_b[2] && tmp_b[3]) {
          EnableWindow(GetDlgItem(hwndDlg, BTN_ADD), true);
        } else {
          EnableWindow(GetDlgItem(hwndDlg, BTN_ADD), false);
        }
      } else {
        GetDlgItemTextW(hwndDlg, ENT_NAME, tmp_sname, 64);
        if(wcslen(tmp_sname) > 0) tmp_b[0] = true;
        GetDlgItemTextW(hwndDlg, ENT_ROOTPATH, tmp_sroot, 260);
        if(wcslen(tmp_sroot) > 3) tmp_b[1] = true;
        GetDlgItemTextW(hwndDlg, ENT_MODSPATH, tmp_smods, 260);
        if(wcslen(tmp_smods) > 3) tmp_b[2] = true;
        if(tmp_b[0] && tmp_b[1] && tmp_b[2]) {
          EnableWindow(GetDlgItem(hwndDlg, BTN_ADD), true);
        } else {
          EnableWindow(GetDlgItem(hwndDlg, BTN_ADD), false);
        }
      }
      return true;

    case BTN_BROWSEBCK:
      GME_DialogDirChooser(hwndDlg, tmp_sback, 260);
      SetDlgItemTextW(hwndDlg, ENT_BACKPATH, tmp_sback);
      return true;

    case BTN_BROWSEROOT:
      GME_DialogDirChooser(hwndDlg, tmp_sroot, 260);
      SetDlgItemTextW(hwndDlg, ENT_ROOTPATH, tmp_sroot);
      return true;

    case BTN_BROWSEMOD:
      GME_DialogDirChooser(hwndDlg, tmp_smods, 260);
      SetDlgItemTextW(hwndDlg, ENT_MODSPATH, tmp_smods);
      return true;

    case BTN_ADD:
      GetDlgItemTextW(hwndDlg, ENT_NAME, tmp_sname, 64);
      GetDlgItemTextW(hwndDlg, ENT_ROOTPATH, tmp_sroot, 260);
      GetDlgItemTextW(hwndDlg, ENT_MODSPATH, tmp_smods, 260);
      GetDlgItemTextW(hwndDlg, ENT_BACKPATH, tmp_sback, 260);
      if(SendMessage(GetDlgItem(hwndDlg, CHK_CUSTBACKUP), BM_GETCHECK, 0, 0)) {
        if(GME_GameNewCfg(tmp_sname, tmp_sroot, tmp_smods, true, tmp_sback)) {
          EndDialog(hwndDlg, 0);
        }
      } else {
        if(GME_GameNewCfg(tmp_sname, tmp_sroot, tmp_smods, false, tmp_sback)) {
          EndDialog(hwndDlg, 0);
        }
      }
      return true;

    case IDCANCEL:
      EndDialog(hwndDlg, 0);
      return true;

    case ENT_NAME:
    case ENT_MODSPATH:
    case ENT_ROOTPATH:
    case ENT_BACKPATH:
      // tbool or disable add button
      tmp_b[0] = false; tmp_b[1] = false; tmp_b[2] = false; tmp_b[3] = false;
      if(SendMessage(GetDlgItem(hwndDlg, CHK_CUSTBACKUP), BM_GETCHECK, 0, 0)) {
        GetDlgItemTextW(hwndDlg, ENT_NAME, tmp_sname, 64);
        if(wcslen(tmp_sname) > 0) tmp_b[0] = true;
        GetDlgItemTextW(hwndDlg, ENT_ROOTPATH, tmp_sroot, 260);
        if(wcslen(tmp_sroot) > 3) tmp_b[1] = true;
        GetDlgItemTextW(hwndDlg, ENT_MODSPATH, tmp_smods, 260);
        if(wcslen(tmp_smods) > 3) tmp_b[2] = true;
        GetDlgItemTextW(hwndDlg, ENT_BACKPATH, tmp_sback, 260);
        if(wcslen(tmp_sback) > 3) tmp_b[3] = true;
        if(tmp_b[0] && tmp_b[1] && tmp_b[2] && tmp_b[3]) {
          EnableWindow(GetDlgItem(hwndDlg, BTN_ADD), true);
        } else {
          EnableWindow(GetDlgItem(hwndDlg, BTN_ADD), false);
        }
      } else {
        GetDlgItemTextW(hwndDlg, ENT_NAME, tmp_sname, 64);
        if(wcslen(tmp_sname) > 0) tmp_b[0] = true;
        GetDlgItemTextW(hwndDlg, ENT_ROOTPATH, tmp_sroot, 260);
        if(wcslen(tmp_sroot) > 3) tmp_b[1] = true;
        GetDlgItemTextW(hwndDlg, ENT_MODSPATH, tmp_smods, 260);
        if(wcslen(tmp_smods) > 3) tmp_b[2] = true;
        if(tmp_b[0] && tmp_b[1] && tmp_b[2]) {
          EnableWindow(GetDlgItem(hwndDlg, BTN_ADD), true);
        } else {
          EnableWindow(GetDlgItem(hwndDlg, BTN_ADD), false);
        }
      }
      return true;
    }
    return true;

  }
  return false;
}

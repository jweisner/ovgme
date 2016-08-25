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
#include "gme_mods.h"
#include "gme_tools.h"
#include "gme_logs.h"

wchar_t tmp_ssrc[260];
wchar_t tmp_sdst[260];
wchar_t* tmp_sdesc;
size_t tmp_desc_size;
int tmp_vmaj;
int tmp_vmin;
int tmp_vrev;
int tmp_zlvl;
int tmp_res;
int tmp_idx;

void GME_DlgModsMakeInit()
{
  HFONT Lucida = CreateFont(12,0,0,0,FW_DONTCARE,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
                          CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,VARIABLE_PITCH,TEXT("Lucida Console"));

  SendMessage(GetDlgItem(g_hwndNewAMod, ENT_MODDESC), WM_SETFONT, (WPARAM)Lucida, 1);
}

/*
  message callback for Create Mod dialog window
*/
BOOL CALLBACK GME_DlgModsMake(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
  case WM_INITDIALOG:
    SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)g_hicnMain);
    g_hwndNewAMod = hwndDlg;
    GME_DlgModsMakeInit();
    EnableWindow(GetDlgItem(g_hwndNewAMod, ENT_SRC), true);
    SetDlgItemText(hwndDlg, ENT_SRC, "");
    EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_BROWSESRC), true);
    SetDlgItemText(hwndDlg, ENT_DST, "");
    EnableWindow(GetDlgItem(g_hwndNewAMod, ENT_DST), true);
    EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_BROWSEDST), true);
    SetDlgItemText(hwndDlg, ENT_VERSMAJOR, "0");
    EnableWindow(GetDlgItem(g_hwndNewAMod, ENT_VERSMAJOR), true);
    SetDlgItemText(hwndDlg, ENT_VERSMINOR, "0");
    EnableWindow(GetDlgItem(g_hwndNewAMod, ENT_VERSMINOR), true);
    SetDlgItemText(hwndDlg, ENT_VERSREVIS, "0");
    EnableWindow(GetDlgItem(g_hwndNewAMod, ENT_VERSREVIS), true);
    SetDlgItemText(hwndDlg, ENT_MODDESC, "");
    EnableWindow(GetDlgItem(g_hwndNewAMod, ENT_MODDESC), true);
    EnableWindow(GetDlgItem(g_hwndNewAMod, IDCANCEL), false);
    // disable Add button
    EnableWindow(GetDlgItem(hwndDlg, BTN_CREATE), false);
    // add compression levels
    SendMessage(GetDlgItem(g_hwndNewAMod, CB_ZIPLEVEL), CB_ADDSTRING, 0, (LPARAM)"Best Compression");
    SendMessage(GetDlgItem(g_hwndNewAMod, CB_ZIPLEVEL), CB_SETITEMDATA, 0, (LPARAM)MZ_BEST_COMPRESSION);
    SendMessage(GetDlgItem(g_hwndNewAMod, CB_ZIPLEVEL), CB_ADDSTRING, 1, (LPARAM)"Default Level");
    SendMessage(GetDlgItem(g_hwndNewAMod, CB_ZIPLEVEL), CB_SETITEMDATA, 1, (LPARAM)MZ_DEFAULT_LEVEL);
    SendMessage(GetDlgItem(g_hwndNewAMod, CB_ZIPLEVEL), CB_ADDSTRING, 2, (LPARAM)"Best Speed");
    SendMessage(GetDlgItem(g_hwndNewAMod, CB_ZIPLEVEL), CB_SETITEMDATA, 2, (LPARAM)MZ_BEST_SPEED);
    SendMessage(GetDlgItem(g_hwndNewAMod, CB_ZIPLEVEL), CB_ADDSTRING, 3, (LPARAM)"No Compression");
    SendMessage(GetDlgItem(g_hwndNewAMod, CB_ZIPLEVEL), CB_SETITEMDATA, 3, (LPARAM)MZ_NO_COMPRESSION);
    SendMessage(GetDlgItem(g_hwndNewAMod, CB_ZIPLEVEL), CB_SETCURSEL, 1, 0);
    return true;

  case WM_CLOSE:
    GME_ModsMakeCancel();
    EndDialog(hwndDlg, 0);
    return true;

  case WM_COMMAND:
    switch(LOWORD(wParam))
    {
    case BTN_BROWSESRC:
      GME_DialogDirChooser(hwndDlg, tmp_ssrc, 260);
      SetDlgItemTextW(hwndDlg, ENT_SRC, tmp_ssrc);
      return true;

    case BTN_BROWSEDST:
      GME_DialogDirChooser(hwndDlg, tmp_sdst, 260);
      SetDlgItemTextW(hwndDlg, ENT_DST, tmp_sdst);
      return true;

    case BTN_CREATE:
      GetDlgItemTextW(hwndDlg, ENT_DST, tmp_sdst, 260);
      GetDlgItemTextW(hwndDlg, ENT_SRC, tmp_ssrc, 260);
      tmp_desc_size = GetWindowTextLength(GetDlgItem(hwndDlg, ENT_MODDESC))+1;
      try {
        tmp_sdesc = new wchar_t[tmp_desc_size+1];
      } catch (const std::bad_alloc&) {
        GME_Logs(GME_LOG_ERROR, "GME_DlgModsMake", "Bad alloc", std::to_string(tmp_desc_size+1).c_str());
        return true;
      }
      GetDlgItemTextW(hwndDlg, ENT_MODDESC, tmp_sdesc, tmp_desc_size);

      tmp_vmaj = GetDlgItemInt(hwndDlg, ENT_VERSMAJOR, &tmp_res, true);
      tmp_vmin = GetDlgItemInt(hwndDlg, ENT_VERSMINOR, &tmp_res, true);
      tmp_vrev = GetDlgItemInt(hwndDlg, ENT_VERSREVIS, &tmp_res, true);

      tmp_idx = SendMessage(GetDlgItem(g_hwndNewAMod, CB_ZIPLEVEL), CB_GETCURSEL, 0, 0);
      tmp_zlvl = SendMessage(GetDlgItem(g_hwndNewAMod, CB_ZIPLEVEL), CB_GETITEMDATA, tmp_idx, 0);
      std::cout << "tmp_zlvl = " << tmp_zlvl << "\n";
      GME_ModsMakeArchive(tmp_ssrc, tmp_sdst, tmp_sdesc, tmp_vmaj, tmp_vmin, tmp_vrev, tmp_zlvl);
      delete[] tmp_sdesc;
      return true;

    case IDCANCEL:
      GME_ModsMakeCancel();
      return true;

    case BTN_CLOSE:
      GME_ModsMakeCancel();
      EndDialog(hwndDlg, 0);
      return true;

    case ENT_DST:
    case ENT_SRC:
      // tbool or disable add button
      GetDlgItemTextW(hwndDlg, ENT_DST, tmp_sdst, 260);
      if(wcslen(tmp_sdst) > 2) {
        GetDlgItemTextW(hwndDlg, ENT_SRC, tmp_ssrc, 32);
        if(wcslen(tmp_ssrc) > 0) {
          EnableWindow(GetDlgItem(hwndDlg, BTN_CREATE), true);
        } else {
          EnableWindow(GetDlgItem(hwndDlg, BTN_CREATE), false);
        }
      } else {
        EnableWindow(GetDlgItem(hwndDlg, BTN_CREATE), false);
      }
      return true;
    }
  }
  return false;
}

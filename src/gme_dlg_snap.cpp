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
#include "gme_snap.h"

/*
  message callback for snapshot creation
*/
BOOL CALLBACK GME_DlgSnapNew(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
  case WM_INITDIALOG:
    SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)g_hicnMain);
    g_hwndSnapNew = hwndDlg;
    // disable Add button
    ShowWindow(GetDlgItem(hwndDlg, TXT_STATUS), false);
    ShowWindow(GetDlgItem(hwndDlg, TXT_COMMENT), false);
    ShowWindow(GetDlgItem(hwndDlg, PBM_SNAP_NEW), false);
    EnableWindow(GetDlgItem(hwndDlg, IDOK), true);
    SetDlgItemTextW(hwndDlg, TXT_TITLE, L"You are about to create a snapshot of the current config root file tree, this can take a long time. Are you ready ?");
    return true;

  case WM_CLOSE:
    GME_SnapCancel();
    EndDialog(hwndDlg, 0);
    return true;

  case WM_COMMAND:
    switch(LOWORD(wParam))
    {
    case IDCANCEL:
      GME_SnapCancel();
      EndDialog(hwndDlg, 0);
      return true;

    case BTN_CLOSE:
      GME_SnapCancel();
      EndDialog(hwndDlg, 0);
      return true;

    case IDOK:
      SetDlgItemTextW(hwndDlg, TXT_TITLE, L"Creating file tree snapshot, please wait...");
      ShowWindow(GetDlgItem(hwndDlg, TXT_STATUS), true);
      ShowWindow(GetDlgItem(hwndDlg, TXT_COMMENT), true);
      ShowWindow(GetDlgItem(hwndDlg, PBM_SNAP_NEW), true);
      EnableWindow(GetDlgItem(hwndDlg, IDOK), false);
      GME_SnapCreate();
      return true;
    }
  }

  return false;
}


void GME_DlgSnapCompInit()
{
    HFONT courier = CreateFont(14,0,0,0,FW_DONTCARE,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
                            CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,VARIABLE_PITCH,TEXT("Courier New"));

    SendMessage(GetDlgItem(g_hwndSnapCmp, ENT_OUTPUT), WM_SETFONT, (WPARAM)courier, 1);
}

/*
  message callback for snapshot creation
*/
BOOL CALLBACK GME_DlgSnapComp(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
  case WM_INITDIALOG:
    SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)g_hicnMain);
    g_hwndSnapCmp = hwndDlg;
    GME_DlgSnapCompInit();
    // disable Add button
    ShowWindow(GetDlgItem(hwndDlg, TXT_STATUS), false);
    ShowWindow(GetDlgItem(hwndDlg, TXT_COMMENT), false);
    ShowWindow(GetDlgItem(hwndDlg, PBM_SNAP_CMP), false);
    EnableWindow(GetDlgItem(hwndDlg, IDOK), true);
    SetDlgItemTextW(hwndDlg, TXT_TITLE, L"You are about to compare the snapshot with the current config root file tree, this can take a long time. Are you ready ?");
    return true;

  case WM_CLOSE:
    GME_SnapCancel();
    EndDialog(hwndDlg, 0);
    return true;

  case WM_COMMAND:
    switch(LOWORD(wParam))
    {
    case IDCANCEL:
      GME_SnapCancel();
      EndDialog(hwndDlg, 0);
      return true;

    case BTN_CLOSE:
      GME_SnapCancel();
      EndDialog(hwndDlg, 0);
      return true;

    case IDOK:
      SetDlgItemTextW(hwndDlg, TXT_TITLE, L"Comparing file tree with snapshot, please wait...");
      ShowWindow(GetDlgItem(hwndDlg, TXT_STATUS), true);
      ShowWindow(GetDlgItem(hwndDlg, TXT_COMMENT), true);
      ShowWindow(GetDlgItem(hwndDlg, PBM_SNAP_CMP), true);
      EnableWindow(GetDlgItem(hwndDlg, IDOK), false);
      GME_SnapCompare();
      return true;
    }
  }

  return false;
}

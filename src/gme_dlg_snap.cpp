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

RECT tmp_cli;

/*
  controls resize & placement routine for snapshot creation dialog
*/
void GME_DlgSnapNewResize()
{
  HWND hwd = g_hwndSnapNew;
  RECT cli;
  GetClientRect(hwd, &cli);

  SetWindowPos(GetDlgItem(hwd, TXT_TITLE), NULL, 10, 10, cli.right-20, 30, SWP_NOZORDER);
  InvalidateRect(GetDlgItem(hwd, TXT_TITLE), NULL, true);
  SetWindowPos(GetDlgItem(hwd, PBM_SNAP_NEW), NULL, 10, 40, cli.right-20, 20, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, TXT_STATUS), NULL, 10, 70, cli.right-20, 15, SWP_NOZORDER);
  InvalidateRect(GetDlgItem(hwd, TXT_STATUS), NULL, true);
  SetWindowPos(GetDlgItem(hwd, TXT_COMMENT), NULL, 10, 90, cli.right-20, 15, SWP_NOZORDER);
  InvalidateRect(GetDlgItem(hwd, TXT_COMMENT), NULL, true);
  SetWindowPos(GetDlgItem(hwd, IDOK), NULL, 10, cli.bottom-34, 80, 24, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, BTN_CLOSE), NULL, cli.right-90, cli.bottom-34, 80, 24, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, IDCANCEL), NULL, 90, cli.bottom-34, 80, 24, SWP_NOZORDER);
}

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
    ShowWindow(GetDlgItem(hwndDlg, BTN_CLOSE), false);
    EnableWindow(GetDlgItem(hwndDlg, IDOK), true);
    SetDlgItemTextW(hwndDlg, TXT_TITLE, L"You are about to create a snapshot of the current config's root file tree, this can take a long time. Are you ready ?");
    GME_DlgSnapNewResize();
    return true;

  case WM_SIZE:
    GME_DlgSnapNewResize();
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
      SetDlgItemTextW(hwndDlg, TXT_TITLE, L"Creating current file tree snapshot, please wait...");
      /* resize dialog box */
      GetWindowRect(hwndDlg, &tmp_cli);
      SetWindowPos(hwndDlg, NULL, tmp_cli.left, tmp_cli.top, tmp_cli.right-tmp_cli.left, (tmp_cli.bottom-tmp_cli.top)+30, SWP_NOZORDER);
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

/*
  initialization routine for snapshot compare dialog
*/
void GME_DlgSnapCompInit()
{
    HFONT courier = CreateFont(14,0,0,0,FW_DONTCARE,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
                            CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,VARIABLE_PITCH,TEXT("Courier New"));

    SendMessage(GetDlgItem(g_hwndSnapCmp, ENT_OUTPUT), WM_SETFONT, (WPARAM)courier, 1);
}

/*
  controls resize & placement routine for snapshot compare dialog
*/
void GME_DlgSnapCompResize()
{
  HWND hwd = g_hwndSnapCmp;
  RECT cli;
  GetClientRect(hwd, &cli);

  SetWindowPos(GetDlgItem(hwd, TXT_TITLE), NULL, 10, 10, cli.right-20, 30, SWP_NOZORDER);
  InvalidateRect(GetDlgItem(hwd, TXT_TITLE), NULL, true);
  SetWindowPos(GetDlgItem(hwd, PBM_SNAP_NEW), NULL, 10, 40, cli.right-20, 20, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, TXT_STATUS), NULL, 10, 70, cli.right-20, 15, SWP_NOZORDER);
  InvalidateRect(GetDlgItem(hwd, TXT_STATUS), NULL, true);
  SetWindowPos(GetDlgItem(hwd, TXT_COMMENT), NULL, 10, 90, cli.right-20, 15, SWP_NOZORDER);
  InvalidateRect(GetDlgItem(hwd, TXT_COMMENT), NULL, true);

  SetWindowPos(GetDlgItem(hwd, LAB_OUTPUT), NULL, 10, 120, cli.right-20, 15, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, ENT_OUTPUT), NULL, 10, 135, cli.right-20, cli.bottom-179, SWP_NOZORDER);

  SetWindowPos(GetDlgItem(hwd, IDOK), NULL, 10, cli.bottom-34, 80, 24, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, BTN_CLOSE), NULL, cli.right-90, cli.bottom-34, 80, 24, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, IDCANCEL), NULL, 90, cli.bottom-34, 80, 24, SWP_NOZORDER);
}

/*
  message callback for snapshot compare
*/
BOOL CALLBACK GME_DlgSnapComp(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
  case WM_INITDIALOG:
    SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)g_hicnMain);
    g_hwndSnapCmp = hwndDlg;
    GME_DlgSnapCompInit();
    GME_DlgSnapCompResize();
    // disable Add button
    ShowWindow(GetDlgItem(hwndDlg, TXT_STATUS), false);
    ShowWindow(GetDlgItem(hwndDlg, TXT_COMMENT), false);
    ShowWindow(GetDlgItem(hwndDlg, PBM_SNAP_CMP), false);
    ShowWindow(GetDlgItem(hwndDlg, LAB_OUTPUT), false);
    ShowWindow(GetDlgItem(hwndDlg, ENT_OUTPUT), false);

    EnableWindow(GetDlgItem(hwndDlg, IDOK), true);
    SetDlgItemTextW(hwndDlg, TXT_TITLE, L"You are about to compare snapshot with the current config's root file tree, this can take a long time. Are you ready ?");
    return true;

  case WM_SIZE:
    GME_DlgSnapCompResize();
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
      SetDlgItemTextW(hwndDlg, TXT_TITLE, L"Comparing current file tree with snapshot, please wait...");
      /* resize dialog box */
      GetWindowRect(hwndDlg, &tmp_cli);
      SetWindowPos(hwndDlg, NULL, tmp_cli.left, tmp_cli.top, tmp_cli.right-tmp_cli.left, (tmp_cli.bottom-tmp_cli.top)+150, SWP_NOZORDER);
      ShowWindow(GetDlgItem(hwndDlg, TXT_STATUS), true);
      ShowWindow(GetDlgItem(hwndDlg, TXT_COMMENT), true);
      ShowWindow(GetDlgItem(hwndDlg, PBM_SNAP_CMP), true);
      ShowWindow(GetDlgItem(hwndDlg, LAB_OUTPUT), true);
      ShowWindow(GetDlgItem(hwndDlg, ENT_OUTPUT), true);
      EnableWindow(GetDlgItem(hwndDlg, IDOK), false);
      GME_SnapCompare();
      return true;
    }
  }

  return false;
}

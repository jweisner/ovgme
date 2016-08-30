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
#include "gme_repo.h"
#include "gme_tools.h"

/* temporary buffer */
char tmp_srurl[256];
wchar_t tmp_spath[260];
unsigned tmp_idummy;

/* Ok guys, let me tell you a story... One day, Microsoft implemented
 a treeview control with checkboxes. But Microsoft implemented this
 thing so badly that, if we set the checkbox status to Enabled BEFORE
 the treeview was painted for the first time, the checkboxes appear
 as unchecked even if the check-status is ENABLED...

 After a pair of hours of research in all MSND and google, i deduct
 Microsoft never thought anyone would initialize a treeview with some
 items already checked, even if the feature exists...

 Yeah... So, to workaround this wonderful "non-feature", i force
 the treeview to be updated just ONE time after the first WM_PAINT
 message... The flag below is used to explain all my love to Microsoft and
 technicaly, to prevent the treeview to be updated each WM_PAINT...
 */
bool FUCK_MICROSOFT;

/*
  message callback for repositories configure
*/
BOOL CALLBACK GME_DlgRepConf(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
  case WM_INITDIALOG:
    SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)g_hicnMain);
    g_hwndRepConf = hwndDlg;
    EnableWindow(GetDlgItem(hwndDlg, BTN_REPOADD), false);
    FUCK_MICROSOFT = true;
    return true;

  case WM_PAINT:
    if(FUCK_MICROSOFT) {
      FUCK_MICROSOFT = false;
      UpdateWindow(hwndDlg);
      /* Do you see this ? yes, update window just after a WM_PAINT
        with flag toggle in between... Yes, this is what you must do to have a
        correct treeview initialization with correct checked checkboxes... */
      GME_RepoUpdList();
    }
    return false;

  case WM_CLOSE:
    GME_RepoWritList();
    EndDialog(hwndDlg, 0);
    return false;

  case WM_COMMAND:
    switch(LOWORD(wParam))
    {
    case BTN_REPOADD:
      GetDlgItemText(hwndDlg, ENT_REPOURL, tmp_srurl, 256);
      GME_RepoAddUrl(tmp_srurl);
      SetDlgItemText(hwndDlg, ENT_REPOURL, "");
      return true;

    case BTN_REPOREM:
      GME_RepoRemUrl();
      return true;

    case BTN_CLOSE:
      GME_RepoWritList();
      EndDialog(hwndDlg, 0);
      return false;

    case ENT_REPOURL:
      // tbool or disable add button
      GetDlgItemText(hwndDlg, ENT_REPOURL, tmp_srurl, 256);
      if(strlen(tmp_srurl) > 4) {
        EnableWindow(GetDlgItem(hwndDlg, BTN_REPOADD), true);
      } else {
        EnableWindow(GetDlgItem(hwndDlg, BTN_REPOADD), false);
      }
      return true;

    }

  case WM_NOTIFY:
    if(((LPNMHDR)lParam)->code == 4294966877) { /* 4294966877 seem to be item change notify... optained by checking raw messages */
      GME_RepoChkList();
    }
    return true;
  }

  return false;
}

void GME_DlgRepUpdInit()
{
  /* list view image list */
  HIMAGELIST hImgLst;
  HBITMAP hBitmap;
  hImgLst = ImageList_Create( 16, 16, ILC_COLOR32, 3, 0 );
  hBitmap = (HBITMAP)LoadImage(g_hInst, MAKEINTRESOURCE(IDB_DNGRADE), IMAGE_BITMAP, 16, 16, 0);
  ImageList_Add(hImgLst, hBitmap, NULL); DeleteObject(hBitmap);
  hBitmap = (HBITMAP)LoadImage(g_hInst, MAKEINTRESOURCE(IDB_UPGRADE), IMAGE_BITMAP, 16, 16, 0);
  ImageList_Add(hImgLst, hBitmap, NULL); DeleteObject(hBitmap);
  hBitmap = (HBITMAP)LoadImage(g_hInst, MAKEINTRESOURCE(IDB_UPTODATE), IMAGE_BITMAP, 16, 16, 0);
  ImageList_Add(hImgLst, hBitmap, NULL); DeleteObject(hBitmap);
  hBitmap = (HBITMAP)LoadImage(g_hInst, MAKEINTRESOURCE(IDB_AVAILABLE), IMAGE_BITMAP, 16, 16, 0);
  ImageList_Add(hImgLst, hBitmap, NULL); DeleteObject(hBitmap);
  SendMessageW(GetDlgItem(g_hwndRepUpd, LVM_MODSUPDLIST), LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)hImgLst);
  SendMessageW(GetDlgItem(g_hwndRepUpd, LVM_MODSUPDLIST), LVM_SETIMAGELIST, LVSIL_NORMAL, (LPARAM)hImgLst);

  /* set style for list view */
  SendMessageW(GetDlgItem(g_hwndRepUpd, LVM_MODSUPDLIST), LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

  /* create colums in list view */
  LVCOLUMNW lvcol;
  memset(&lvcol, 0, sizeof(LV_COLUMNW));
  lvcol.mask = LVCF_WIDTH;
  lvcol.cx = 450;
  lvcol.iSubItem = 0; // mod name
  SendMessageW(GetDlgItem(g_hwndRepUpd, LVM_MODSUPDLIST), LVM_INSERTCOLUMNW, 0, (LPARAM)&lvcol);
  lvcol.cx = 60;
  lvcol.iSubItem = 1; // version
  SendMessageW(GetDlgItem(g_hwndRepUpd, LVM_MODSUPDLIST), LVM_INSERTCOLUMNW, 1, (LPARAM)&lvcol);
  lvcol.cx = 158;
  lvcol.iSubItem = 2; // status
  SendMessageW(GetDlgItem(g_hwndRepUpd, LVM_MODSUPDLIST), LVM_INSERTCOLUMNW, 2, (LPARAM)&lvcol);
  lvcol.cx = 40;
  lvcol.iSubItem = 3; // progress
  SendMessageW(GetDlgItem(g_hwndRepUpd, LVM_MODSUPDLIST), LVM_INSERTCOLUMNW, 3, (LPARAM)&lvcol);

  HFONT Lucida = CreateFont(12,0,0,0,FW_DONTCARE,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
                          CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,VARIABLE_PITCH,TEXT("Lucida Console"));

  SendMessage(GetDlgItem(g_hwndRepUpd, ENT_MODDESC), WM_SETFONT, (WPARAM)Lucida, 1);
}

/*
  message callback for repositories update
*/
BOOL CALLBACK GME_DlgRepUpd(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
  case WM_INITDIALOG:
    SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)g_hicnMain);
    g_hwndRepUpd = hwndDlg;
    GME_DlgRepUpdInit();

    EnableWindow(GetDlgItem(g_hwndRepUpd, BTN_CANCEL), false);
    EnableWindow(GetDlgItem(g_hwndRepUpd, BTN_REPOUPDSEL), false);
    EnableWindow(GetDlgItem(g_hwndRepUpd, BTN_REPOUPDALL), false);
    GME_RepoQueryUpd();
    return true;

  case WM_CLOSE:
    GME_RepoQueryCancel();
    EndDialog(hwndDlg, 0);
    return true;

  case WM_NOTIFY:
    if(LOWORD(wParam) == LVM_MODSUPDLIST) {
      if(((LPNMHDR)lParam)->code == NM_DBLCLK) {
        GME_RepoDownloadSel();
        return true;
      }
      if(((LPNMHDR)lParam)->code == NM_CLICK) {
        GME_RepoChkDesc();
        return true;
      }
    }
    return true;

  case WM_COMMAND:
    switch(LOWORD(wParam))
    {
    case BTN_CLOSE:
      GME_RepoQueryCancel();
      EndDialog(hwndDlg, 0);
      return true;

    case BTN_REPOUPDSEL:
      GME_RepoDownloadSel();
      return true;

    case BTN_REPOUPDALL:
      GME_RepoDownloadAll();
      return true;

    case BTN_REPORETRY:
      EnableWindow(GetDlgItem(g_hwndRepUpd, BTN_CANCEL), false);
      EnableWindow(GetDlgItem(g_hwndRepUpd, BTN_REPOUPDSEL), false);
      EnableWindow(GetDlgItem(g_hwndRepUpd, BTN_REPOUPDALL), false);
      GME_RepoQueryUpd();
      return true;

    case BTN_CANCEL:
      GME_RepoQueryCancel();
      return true;
    }
  }

  return false;
}


void GME_DlgRepXmlInit()
{
    HFONT courier = CreateFont(14,0,0,0,FW_DONTCARE,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
                            CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,VARIABLE_PITCH,TEXT("Courier New"));

    SendMessage(GetDlgItem(g_hwndRepXml, ENT_OUTPUT), WM_SETFONT, (WPARAM)courier, 1);
}

/*
  message callback for repositories configure
*/
BOOL CALLBACK GME_DlgRepXml(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
  case WM_INITDIALOG:
    SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)g_hicnMain);
    g_hwndRepXml = hwndDlg;
    GME_DlgRepXmlInit();

    EnableWindow(GetDlgItem(hwndDlg, BTN_MAKEXMLSRC), false);
    return true;

  case WM_CLOSE:
    EndDialog(hwndDlg, 0);
    return true;

  case WM_COMMAND:
    switch(LOWORD(wParam))
    {
    case CHK_CUSTXMLPATH:
      if(SendMessage(GetDlgItem(hwndDlg, CHK_CUSTXMLPATH), BM_GETCHECK, 0, 0)) {
        EnableWindow(GetDlgItem(hwndDlg, ENT_MAKEXMLPATH), true);
        EnableWindow(GetDlgItem(hwndDlg, BTN_BROWSEXMLPATH), true);
      } else {
        EnableWindow(GetDlgItem(hwndDlg, ENT_MAKEXMLPATH), false);
        EnableWindow(GetDlgItem(hwndDlg, BTN_BROWSEXMLPATH), false);
      }
      return true;

    case BTN_MAKEXMLSRC:
      GetDlgItemText(hwndDlg, ENT_MAKEXMLURL, tmp_srurl, 256);
      if(SendMessage(GetDlgItem(hwndDlg, CHK_CUSTXMLPATH), BM_GETCHECK, 0, 0)) {
        GetDlgItemTextW(hwndDlg, ENT_MAKEXMLPATH, tmp_spath, 260);
        GME_RepoMakeXml(tmp_srurl, true, tmp_spath);
      } else {
        GME_RepoMakeXml(tmp_srurl, false, NULL);
      }
      return true;

    case BTN_BROWSEXMLPATH:
      GME_DialogDirChooser(hwndDlg, tmp_spath, 260);
      SetDlgItemTextW(hwndDlg, ENT_MAKEXMLPATH, tmp_spath);
      return true;

    case BTN_MAKEXMLSAVE:
      GME_RepoSaveXml();
      return true;

    case BTN_CLOSE:
      EndDialog(hwndDlg, 0);
      return true;

    case ENT_MAKEXMLPATH:
    case ENT_MAKEXMLURL:
      // tbool or disable add button
      if(SendMessage(GetDlgItem(hwndDlg, CHK_CUSTXMLPATH), BM_GETCHECK, 0, 0)) {
        GetDlgItemText(hwndDlg, ENT_MAKEXMLURL, tmp_srurl, 256);
        if(strlen(tmp_srurl) > 3) {
          GetDlgItemTextW(hwndDlg, ENT_MAKEXMLPATH, tmp_spath, 260);
          if(wcslen(tmp_spath) > 2) {
            EnableWindow(GetDlgItem(hwndDlg, BTN_MAKEXMLSRC), true);
          } else {
            EnableWindow(GetDlgItem(hwndDlg, BTN_MAKEXMLSRC), false);
          }
        } else {
          EnableWindow(GetDlgItem(hwndDlg, BTN_MAKEXMLSRC), false);
        }
      } else {
        GetDlgItemText(hwndDlg, ENT_MAKEXMLURL, tmp_srurl, 256);
        if(strlen(tmp_srurl) > 3) {
          EnableWindow(GetDlgItem(hwndDlg, BTN_MAKEXMLSRC), true);
        } else {
          EnableWindow(GetDlgItem(hwndDlg, BTN_MAKEXMLSRC), false);
        }
      }
      return true;

    }
  }

  return false;
}


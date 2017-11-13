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
  controls resize & placement routine for repositories configure dialog
*/
void GME_DlgRepConfResize()
{
  HWND hwd = g_hwndRepConf;
  RECT cli;
  GetClientRect(hwd, &cli);

  SetWindowPos(GetDlgItem(hwd, BTN_CLOSE), NULL, cli.right-90, cli.bottom-30, 80, 24, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, BOX_MAIN), NULL, 10, 25, cli.right-20, cli.bottom-60, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, BTN_REPOREM), NULL, 20, cli.bottom-70, 80, 22, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, TVS_REPOSLIST), NULL, 20, 70, cli.right-40, cli.bottom-150, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, LAB_URL), NULL, 20, 45, 30, 16, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, ENT_REPOURL), NULL, 50, 41, cli.right-155, 20, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, BTN_REPOADD), NULL, cli.right-100, 40, 80, 22, SWP_NOZORDER);
}

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
    GME_DlgRepConfResize();
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

  case WM_SIZE:
    GME_DlgRepConfResize();
    return true;

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
    if(lParam) {
      if(((LPNMHDR)lParam)->code == 4294966877) { /* 4294966877 seem to be item change notify... optained by checking raw messages */
        GME_RepoChkList();
      }
    }
    return true;
  }

  return false;
}

/*
  initialization routine for repositories update dialog
*/
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
  controls resize & placement routine for repositories update dialog
*/
void GME_DlgRepUpdResize()
{
  HWND hwd = g_hwndRepUpd;
  RECT cli;
  GetClientRect(hwd, &cli);

  SetWindowPos(GetDlgItem(hwd, BTN_CLOSE), NULL, cli.right-90, cli.bottom-30, 80, 24, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, BOX_MAIN), NULL, 10, 80, cli.right-20, cli.bottom-120, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, ENT_MODDESC), NULL, 20, cli.bottom-160, cli.right-40, 110, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, BTN_REPOUPDSEL), NULL, 20, cli.bottom-190, 100, 22, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, BTN_REPOUPDALL), NULL, 125, cli.bottom-190, 100, 22, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, PBM_DONWLOAD), NULL, 20, cli.bottom-220, cli.right-190, 20, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, TXT_DOWNSPEED), NULL, cli.right-160, cli.bottom-216, 60, 16, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, BTN_CANCEL), NULL, cli.right-100, cli.bottom-220, 80, 22, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, LVM_MODSUPDLIST), NULL, 20, 100, cli.right-40, cli.bottom-330, SWP_NOZORDER);

  LVCOLUMNW lvcol;
  memset(&lvcol, 0, sizeof(LV_COLUMNW));
  lvcol.mask = LVCF_WIDTH;
  lvcol.cx = cli.right-339;
  lvcol.iSubItem = 0; // mod name
  SendMessageW(GetDlgItem(hwd, LVM_MODSUPDLIST), LVM_SETCOLUMN, 0, (LPARAM)&lvcol);
  lvcol.cx = 60;
  lvcol.iSubItem = 1; // version
  SendMessageW(GetDlgItem(hwd, LVM_MODSUPDLIST), LVM_SETCOLUMN, 1, (LPARAM)&lvcol);
  lvcol.cx = 158;
  lvcol.iSubItem = 2; // status
  SendMessageW(GetDlgItem(hwd, LVM_MODSUPDLIST), LVM_SETCOLUMN, 2, (LPARAM)&lvcol);
  lvcol.cx = 60;
  lvcol.iSubItem = 3; // progress
  SendMessageW(GetDlgItem(hwd, LVM_MODSUPDLIST), LVM_SETCOLUMN, 3, (LPARAM)&lvcol);

  SetWindowPos(GetDlgItem(hwd, TXT_REPOQRYURL), NULL, 10, 60, cli.right-20, 16, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, PBM_REPOQRY), NULL, 10, 30, cli.right-75, 20, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, BTN_REPORETRY), NULL, cli.right-60, 29, 50, 22, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, TXT_REPOQRYSTATUS), NULL, 10, 10, cli.right-20, 16, SWP_NOZORDER);
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
    GME_DlgRepUpdResize();

    EnableWindow(GetDlgItem(g_hwndRepUpd, BTN_CANCEL), false);
    EnableWindow(GetDlgItem(g_hwndRepUpd, BTN_REPOUPDSEL), false);
    EnableWindow(GetDlgItem(g_hwndRepUpd, BTN_REPOUPDALL), false);
    GME_RepoQueryUpd();
    return true;

  case WM_CLOSE:
    GME_RepoQueryCancel();
    EndDialog(hwndDlg, 0);
    return true;

  case WM_SIZE:
    GME_DlgRepUpdResize();
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

/*
  initialization routine for repositories make XML source dialog
*/
void GME_DlgRepXmlInit()
{
    HFONT courier = CreateFont(14,0,0,0,FW_DONTCARE,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
                            CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,VARIABLE_PITCH,TEXT("Courier New"));

    SendMessage(GetDlgItem(g_hwndRepXml, ENT_OUTPUT), WM_SETFONT, (WPARAM)courier, 1);
}

/*
  controls resize & placement routine for repositories make XML source dialog
*/
void GME_DlgRepXmlResize()
{
  HWND hwd = g_hwndRepXml;
  RECT cli;
  GetClientRect(hwd, &cli);

  SetWindowPos(GetDlgItem(hwd, BTN_CLOSE), NULL, cli.right-90, cli.bottom-30, 80, 24, SWP_NOZORDER);

  SetWindowPos(GetDlgItem(hwd, TXT_TITLE), NULL, 10, 10, 300, 15, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, CHK_CUSTXMLPATH), NULL, 20, 50, 300, 16, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, ENT_MAKEXMLPATH), NULL, 40, 70, cli.right-125, 20, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, BTN_BROWSEXMLPATH), NULL, cli.right-80, 69, 60, 22, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, TXT_URL), NULL, 20, 100, 300, 15, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, ENT_MAKEXMLURL), NULL, 40, 120, cli.right-125, 20, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, BOX_MAIN), NULL, 10, 30, cli.right-20, cli.bottom-70, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, BTN_MAKEXMLSRC), NULL, 20, 155, 100, 22, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, ENT_OUTPUT), NULL, 20, 180, cli.right-40, cli.bottom-255, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, BTN_MAKEXMLSAVE), NULL, 20, cli.bottom-70, 100, 22, SWP_NOZORDER);
}

/*
  message callback for repositories make XML source
*/
BOOL CALLBACK GME_DlgRepXml(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
  case WM_INITDIALOG:
    SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)g_hicnMain);
    g_hwndRepXml = hwndDlg;
    GME_DlgRepXmlInit();
    GME_DlgRepXmlResize();

    EnableWindow(GetDlgItem(hwndDlg, BTN_MAKEXMLSRC), false);
    return true;

  case WM_SIZE:
    GME_DlgRepXmlResize();
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

/*
  initialization routine for repositories XML test dialog
*/
void GME_DlgRepXtsInit()
{
    HFONT courier = CreateFont(14,0,0,0,FW_DONTCARE,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
                            CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,VARIABLE_PITCH,TEXT("Courier New"));

    SendMessage(GetDlgItem(g_hwndRepXts, ENT_OUTPUT), WM_SETFONT, (WPARAM)courier, 1);
}

/*
  controls resize & placement routine for repositories XML test dialog
*/
void GME_DlgRepXtsResize()
{
  HWND hwd = g_hwndRepXts;
  RECT cli;
  GetClientRect(hwd, &cli);

  SetWindowPos(GetDlgItem(hwd, BTN_CLOSE), NULL, cli.right-90, cli.bottom-30, 80, 24, SWP_NOZORDER);

  SetWindowPos(GetDlgItem(hwd, TXT_TITLE), NULL, 10, 10, 300, 15, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, BOX_MAIN), NULL, 10, 30, cli.right-20, cli.bottom-80, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, TXT_MESSAGE), NULL, 20, 50, cli.right-40, 30, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, LAB_RESULT), NULL, 20, 100, 120, 15, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, ENT_OUTPUT), NULL, 20, 115, cli.right-40, cli.bottom-180, SWP_NOZORDER);
}

/*
  message callback for repositories XML test
*/
BOOL CALLBACK GME_DlgRepXts(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
  case WM_INITDIALOG:
    SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)g_hicnMain);
    g_hwndRepXts = hwndDlg;
    GME_DlgRepXtsInit();
    if(GME_DialogFileOpen(g_hwndRepXts, tmp_spath, 260, &tmp_idummy, L"XML file (*.xml)\0*.XML;\0", L"Choose XML file")) {
      GME_RepoTestXml(tmp_spath, tmp_idummy);
    }
    GME_DlgRepXtsResize();
    return true;

  case WM_SIZE:
    GME_DlgRepXtsResize();
    return true;

  case WM_CLOSE:
    EndDialog(hwndDlg, 0);
    return true;

  case WM_COMMAND:
    switch(LOWORD(wParam))
    {
    case BTN_CLOSE:
      EndDialog(hwndDlg, 0);
      return true;
    }
  }

  return false;
}

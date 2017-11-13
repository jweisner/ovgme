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
#include "gme_tools.h"
#include "gme_game.h"
#include "gme_conf.h"
#include "gme_mods.h"
#include "gme_prof.h"
#include "gme_repo.h"
#include "gme_netw.h"
#include "gme_logs.h"

#include "gme_dlg_about.h"
#include "gme_dlg_debug.h"
#include "gme_dlg_repo.h"
#include "gme_dlg_snap.h"
#include "gme_dlg_mods.h"
#include "gme_dlg_prof.h"
#include "gme_dlg_game.h"
#include "gme_dlg_main.h"

/*
  function for main init, startup process
*/
void GME_MainInit()
{
  GME_Logs(GME_LOG_NOTICE, "GME_MainInit", "OvGME Initialization", "Log history start");

  GME_ConfLoadCfg();

  /* create main icon */
  g_hicnMain = (HICON)LoadImage(g_hInst, MAKEINTRESOURCE(IDI_ICON),IMAGE_ICON,0,0,LR_DEFAULTSIZE|LR_SHARED);

  /* create main menu */
  g_hmnuMain = LoadMenu(NULL, MAKEINTRESOURCE(IDR_MENU));

  /* get profile submenu */
  g_hmnuSubProf = GetSubMenu(GetSubMenu(g_hmnuMain, 1), 3);

  /* list view image list */
  HIMAGELIST hImgLst;
  HBITMAP hBitmap;
  hImgLst = ImageList_Create( 16, 16, ILC_COLOR32, 3, 0 );
  hBitmap = (HBITMAP)LoadImage(g_hInst, MAKEINTRESOURCE(IDB_MODDIR), IMAGE_BITMAP, 16, 16, 0);
  ImageList_Add(hImgLst, hBitmap, NULL); DeleteObject(hBitmap);
  hBitmap = (HBITMAP)LoadImage(g_hInst, MAKEINTRESOURCE(IDB_MODZIP), IMAGE_BITMAP, 16, 16, 0);
  ImageList_Add(hImgLst, hBitmap, NULL); DeleteObject(hBitmap);
  hBitmap = (HBITMAP)LoadImage(g_hInst, MAKEINTRESOURCE(IDB_MODENABLED), IMAGE_BITMAP, 16, 16, 0);
  ImageList_Add(hImgLst, hBitmap, NULL); DeleteObject(hBitmap);
  SendMessageW(GetDlgItem(g_hwndMain, LVM_MODSLIST), LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)hImgLst);
  SendMessageW(GetDlgItem(g_hwndMain, LVM_MODSLIST), LVM_SETIMAGELIST, LVSIL_NORMAL, (LPARAM)hImgLst);
  DeleteObject(hImgLst);

  /* set list window-style */
  if(!GME_ConfGetEnSort()) {
    int dwstyle = GetWindowLongPtrA(GetDlgItem(g_hwndMain, LVM_MODSLIST), GWL_STYLE);
    SetWindowLong(GetDlgItem(g_hwndMain, LVM_MODSLIST), GWL_STYLE, dwstyle|LVS_SORTASCENDING);
  } else {
    SendMessageW(GetDlgItem(g_hwndMain, CHK_SORTMODS), BM_SETCHECK, 1, 1);
  }

  /* set style for modlist */
  SendMessageW(GetDlgItem(g_hwndMain, LVM_MODSLIST), LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT|LVS_EX_SUBITEMIMAGES);

  /* create colums in list view */
  wchar_t buff[64];
  LVCOLUMNW lvcol;
  memset(&lvcol, 0, sizeof(LV_COLUMNW));
  lvcol.mask = LVCF_TEXT|LVCF_WIDTH;
  lvcol.cchTextMax = 255;

  wcscpy(buff, L"Mod");
  lvcol.pszText = buff;
  lvcol.cx = 650;
  lvcol.iSubItem = 0;
  SendMessageW(GetDlgItem(g_hwndMain, LVM_MODSLIST), LVM_INSERTCOLUMNW, 0, (LPARAM)&lvcol);

  wcscpy(buff, L"Version");
  lvcol.pszText = buff;
  lvcol.cx = 70;
  lvcol.iSubItem = 1;
  SendMessageW(GetDlgItem(g_hwndMain, LVM_MODSLIST), LVM_INSERTCOLUMNW, 1, (LPARAM)&lvcol);

  HFONT Lucida = CreateFont(12,0,0,0,FW_DONTCARE,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
                          CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,VARIABLE_PITCH,TEXT("Lucida Console"));

  SendMessage(GetDlgItem(g_hwndMain, ENT_MODDESC), WM_SETFONT, (WPARAM)Lucida, 1);

  SetWindowPos(g_hwndMain, NULL, GME_ConfGetWinX(), GME_ConfGetWinY(), GME_ConfGetWinW(), GME_ConfGetWinH(), SWP_NOZORDER);
}

/*
  function to create pop menu when right-click on LV mods list
*/
void GME_MainPopMenuMods()
{
  /* mouse cursor position */
  POINT mouse_p;
  GetCursorPos(&mouse_p);


  HWND hlv = GetDlgItem(g_hwndMain, LVM_MODSLIST);

  bool single = (1 == SendMessage(hlv, LVM_GETSELECTEDCOUNT, 0, 0));

  /* get single selected Mod type */
  int type = -1;

  if(single) {
    wchar_t name_buff[255];

    LV_ITEMW lvitm;
    memset(&lvitm, 0, sizeof(LV_ITEMW));
    lvitm.mask = LVIF_TEXT|LVIF_IMAGE;
    lvitm.cchTextMax = 255;
    lvitm.pszText = name_buff;

    bool found = false;

    unsigned c = SendMessageW(hlv, LVM_GETITEMCOUNT, 0, 0);
    for(unsigned i = 0; i < c; i++) {
      if(SendMessageW(hlv, LVM_GETITEMSTATE, i, LVIS_SELECTED)) {
        lvitm.iItem = i;
        SendMessageW(hlv ,LVM_GETITEMW, 0, (LPARAM)&lvitm);
        type = lvitm.iImage;
        found = true;
        break; // single selection
      }
    }
  }

  HMENU hMenuPopup = CreatePopupMenu();
  if(single) {
    AppendMenu(hMenuPopup, MF_STRING, POP_TOGGLE, "Toggle");
    AppendMenu(hMenuPopup, MFT_SEPARATOR, MFT_SEPARATOR, "---");
    AppendMenu(hMenuPopup, MF_STRING, POP_OPENDIR, "Explore...");
    AppendMenu(hMenuPopup, MF_STRING, POP_MODDELETE, "Delete...");
    AppendMenu(hMenuPopup, MFT_SEPARATOR, MFT_SEPARATOR, "---");
    AppendMenu(hMenuPopup, MF_STRING, POP_MODCREATE, "Make Mod-Archive...");
  } else {
    AppendMenu(hMenuPopup, MF_STRING, POP_TOGGLE, "Toggle selected");
    AppendMenu(hMenuPopup, MF_STRING, POP_ENABLE, "Enable selected");
    AppendMenu(hMenuPopup, MF_STRING, POP_DISABLE, "Disable selected");
  }

  switch(type)
  {
  case 0:
    break;
  case 1:
    EnableMenuItem(hMenuPopup, POP_MODCREATE, MF_GRAYED);
    break;
  case 2:
    EnableMenuItem(hMenuPopup, POP_MODCREATE, MF_GRAYED);
    EnableMenuItem(hMenuPopup, POP_MODDELETE, MF_GRAYED);
    EnableMenuItem(hMenuPopup, POP_OPENDIR, MF_GRAYED);
    EnableMenuItem(hMenuPopup, POP_MODEDIT, MF_GRAYED);
    break;
  }

  TrackPopupMenu(hMenuPopup, TPM_TOPALIGN|TPM_LEFTALIGN, mouse_p.x, mouse_p.y, 0, g_hwndMain, NULL);
}

/*
  function for main exit, cleanup and exit process
*/
void GME_MainExit()
{
  GME_ConfSaveCfg();
  GME_ModsClean();
  GME_GameClean();
  GME_RepoClean();
}

/*
  controls resize & placement routine for main dialog window
*/
void GME_DlgMainResize()
{
  HWND hwd = g_hwndMain;
  RECT cli;
  GetClientRect(hwd, &cli);

  SetWindowPos(GetDlgItem(hwd, BOX_MAIN), NULL, 10, 77, cli.right-20, cli.bottom-115, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, BTN_QUIT), NULL, cli.right-90, cli.bottom-30, 80, 23, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, ENT_MODDESC), NULL, 20, cli.bottom-160, cli.right-40, 110, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, LAB_MODDESC), NULL, 20, cli.bottom-175, 150, 15, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, BTN_MODDIS), NULL, 20, cli.bottom-203, 100, 23, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, BTN_MODENA), NULL, 120, cli.bottom-203, 100, 23, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, BTN_MODCANCEL), NULL, cli.right-100, cli.bottom-203, 80, 23, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, PBM_MODPROC), NULL, 225, cli.bottom-203, cli.right-330, 22, SWP_NOZORDER);

  SetWindowPos(GetDlgItem(hwd, LVM_MODSLIST), NULL, 20, 135, cli.right-40, cli.bottom-345, SWP_NOZORDER);

  /* modify colums in list view */
  LVCOLUMNW lvcol;
  memset(&lvcol, 0, sizeof(LV_COLUMNW));
  lvcol.mask = LVCF_WIDTH;
  lvcol.cx = cli.right-125;
  lvcol.iSubItem = 0;
  SendMessageW(GetDlgItem(hwd, LVM_MODSLIST), LVM_SETCOLUMN, 0, (LPARAM)&lvcol);
  lvcol.cx = 60;
  lvcol.iSubItem = 1;
  SendMessageW(GetDlgItem(hwd, LVM_MODSLIST), LVM_SETCOLUMN, 1, (LPARAM)&lvcol);

  SetWindowPos(GetDlgItem(hwd, CHK_SORTMODS), NULL, 20, 115, cli.right-40, 20, SWP_NOZORDER);

  SetWindowPos(GetDlgItem(hwd, BTN_IMPORTMOD), NULL, cli.right-105, 90, 85, 23, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, ENT_MODSPATH), NULL, 20, 91, cli.right-130, 21, SWP_NOZORDER);

  SetWindowPos(GetDlgItem(hwd, BOX_TOP), NULL, 155, 0, cli.right-165, 80, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, CMB_GAMELIST), NULL, 165, 20, cli.right-260, 22, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, BTN_EDIGAME), NULL, cli.right-92, 19, 35, 23, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, BTN_ADDGAME), NULL, cli.right-55, 19, 35, 23, SWP_NOZORDER);
  SetWindowPos(GetDlgItem(hwd, ENT_CFGRPATH), NULL, 165, 45, cli.right-185, 21, SWP_NOZORDER);
}


/*
  Mods list view automatic sorting enabled or not
*/
void GME_DlgMainSortList(bool byenabled)
{
  int dwstyle = GetWindowLongPtrA(GetDlgItem(g_hwndMain, LVM_MODSLIST), GWL_STYLE);

  if(byenabled) {
    dwstyle = dwstyle & ~LVS_SORTASCENDING;
    SetWindowLong(GetDlgItem(g_hwndMain, LVM_MODSLIST), GWL_STYLE, dwstyle);
  } else {
    SetWindowLong(GetDlgItem(g_hwndMain, LVM_MODSLIST), GWL_STYLE, dwstyle|LVS_SORTASCENDING);
  }
}

/*
  message callback for main dialog window
*/
BOOL CALLBACK GME_DlgMain(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
  case WM_INITDIALOG:
    g_hwndMain = hwndDlg;

    /* global init */
    GME_MainInit();
    /* set main icon */
    SendMessage(g_hwndMain, WM_SETICON, ICON_BIG, (LPARAM)g_hicnMain);
    /* set main menu */
    SetMenu(hwndDlg, g_hmnuMain);
    /* disable cancel button */
    EnableWindow(GetDlgItem(g_hwndMain, BTN_MODCANCEL), false);
    /* Get game cfg and create list */
    if(!GME_GameUpdList()) {
      DialogBox(g_hInst, MAKEINTRESOURCE(DLG_WIZARD), hwndDlg, (DLGPROC)GME_DlgGameAdd);
    }
    GME_DlgMainResize();
    return true;

  case WM_SIZE:
    GME_DlgMainResize();
    return true;

  case WM_CLOSE:
    GME_MainExit();
    EndDialog(hwndDlg, 0);
    return true;

  case WM_NOTIFY:
    if(LOWORD(wParam) == LVM_MODSLIST) {
      if(((LPNMHDR)lParam)->code == NM_DBLCLK) {
        GME_ModsToggleSel(MODS_TOGGLE);
        return true;
      }
      if(((LPNMHDR)lParam)->code == NM_CLICK) {
        GME_ModsChkDesc();
        return true;
      }
      if(((LPNMHDR)lParam)->code == NM_RCLICK) {
        GME_MainPopMenuMods();
        return true;
      }
      if(((LPNMHDR)lParam)->code == LVN_ITEMCHANGED) {
        GME_ModsChkDesc();
        return true;
      }
    }
    return true;

  case WM_COMMAND:
    if(LOWORD(wParam) > 40100) {
      GME_ProfApply(LOWORD(wParam));
      return true;
    }
    switch(LOWORD(wParam))
    {
    case BTN_QUIT:
      GME_MainExit();
      EndDialog(hwndDlg, 0);
      return true;

    case CHK_SORTMODS:
      GME_DlgMainSortList(SendMessage(GetDlgItem(hwndDlg, CHK_SORTMODS), BM_GETCHECK, 0, 0));
      GME_ModsUpdList();
      return true;

    case MNU_GAMEADD:
    case BTN_ADDGAME:
      DialogBox(g_hInst, MAKEINTRESOURCE(DLG_GAME_ADD), hwndDlg, (DLGPROC)GME_DlgGameAdd);
      return true;

    case MNU_MODIMPORT:
    case BTN_IMPORTMOD:
      GME_ModsImport();
      return true;

    case MNU_MODCREATE:
      DialogBox(g_hInst, MAKEINTRESOURCE(DLG_MOD_MAKE), hwndDlg, (DLGPROC)GME_DlgModsMake);
      return true;

    case MNU_GAMEREM:
      GME_GameRemCurCfg();
      return true;

    case MNU_GAMEEDIT:
    case BTN_EDIGAME:
      DialogBox(g_hInst, MAKEINTRESOURCE(DLG_GAME_EDIT), hwndDlg, (DLGPROC)GME_DlgGameEdit);
      return true;

    case MNU_MODENA:
    case BTN_MODENA:
    case POP_ENABLE:
      GME_ModsToggleSel(MODS_ENABLE);
      return true;

    case MNU_MODDIS:
    case BTN_MODDIS:
    case POP_DISABLE:
      GME_ModsToggleSel(MODS_DISABLE);
      return true;

    case MNU_MODENAALL:
      GME_ModsToggleAll(MODS_ENABLE);
      return true;

    case MNU_MODDISALL:
      GME_ModsToggleAll(MODS_DISABLE);
      return true;

    case BTN_MODCANCEL:
      GME_ModsProcCancel();
      return true;

    case MNU_SNAPCREATE:
      DialogBox(g_hInst, MAKEINTRESOURCE(DLG_SNAP_NEW), hwndDlg, (DLGPROC)GME_DlgSnapNew);
      return true;

    case MNU_SNAPCOMPARE:
      DialogBox(g_hInst, MAKEINTRESOURCE(DLG_SNAP_CMP), hwndDlg, (DLGPROC)GME_DlgSnapComp);
      return true;

    case MNU_PROFILSAVE:
      DialogBox(g_hInst, MAKEINTRESOURCE(DLG_PROF_NEW), hwndDlg, (DLGPROC)GME_DlgProfNew);
      return true;

    case MNU_PROFILDELT:
      DialogBox(g_hInst, MAKEINTRESOURCE(DLG_PROF_DEL), hwndDlg, (DLGPROC)GME_DlgProfDel);
      return true;

    case MNU_REPOSCONFIG:
      DialogBox(g_hInst, MAKEINTRESOURCE(DLG_REPOS_LST), hwndDlg, (DLGPROC)GME_DlgRepConf);
      return true;

    case MNU_REPOSQUERY:
      DialogBox(g_hInst, MAKEINTRESOURCE(DLG_REPOS_UPD), hwndDlg, (DLGPROC)GME_DlgRepUpd);
      return true;

    case MNU_REPOSMKXML:
      DialogBox(g_hInst, MAKEINTRESOURCE(DLG_REPOS_XML), hwndDlg, (DLGPROC)GME_DlgRepXml);
      return true;

    case MNU_REPOSTSXML:
      DialogBox(g_hInst, MAKEINTRESOURCE(DLG_REPOS_XTS), hwndDlg, (DLGPROC)GME_DlgRepXts);
      return true;

    case MNU_ABOUT:
      DialogBox(g_hInst, MAKEINTRESOURCE(DLG_ABOUT), hwndDlg, (DLGPROC)GME_DlgAbout);
      return true;

    case MNU_HELP:
      ShellExecute(hwndDlg, _T( "open" ), _T( "ovgme.chm" ), NULL, NULL, SW_NORMAL );
      return true;

    case MNU_LOGS:
      DialogBox(g_hInst, MAKEINTRESOURCE(DLG_LOGS), hwndDlg, (DLGPROC)GME_DlgDebug);
      return true;

    case CMB_GAMELIST:
      if(HIWORD(wParam) == CBN_SELCHANGE) {
        GME_GameChkSelect();
      }
      return true;

    case POP_TOGGLE:
      GME_ModsToggleSel(MODS_TOGGLE);
      return true;

    case POP_OPENDIR:
      GME_ModsExploreCur();
      return true;

    case POP_MODCREATE:
      DialogBox(g_hInst, MAKEINTRESOURCE(DLG_MOD_QMAKE), hwndDlg, (DLGPROC)GME_DlgModsQuickMake);
      return true;

    case POP_MODDELETE:
      GME_ModsDeleteCur();
      return true;
    }
  }

  return false;
}


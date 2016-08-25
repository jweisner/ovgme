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

#include "gme_snap.h"
#include "gme_game.h"
#include "gme_tools.h"

/* backup entry struct */
struct GME_SnapEntry_Struct
{
  bool is_dir;
  wchar_t path[260];
  unsigned xhash;
};

/*
  snapshot process can become horibly long... so to keep control on dialog
  we work with threads...
*/
HANDLE g_SnapProc_hT;
DWORD g_SnapProc_iT;
bool g_SnapProc_Cancel = false;
bool g_SnapProc_Running = false;


/* threaded function to create snapshot */
DWORD WINAPI GME_SnapCreate_Th(void* pargs)
{
  g_SnapProc_Running = true;

  HWND hpb = GetDlgItem(g_hwndSnapNew, PBM_SNAP_NEW);

  SetDlgItemTextW(g_hwndSnapNew, TXT_STATUS, L"Analysing file tree...");

  std::wstring game_path = GME_GameGetCurRoot();
  std::wstring back_path = GME_GameGetCurBackPath();
  std::wstring mods_path = GME_GameGetCurModsPath();
  std::wstring conf_path = GME_GameGetCurConfPath();
  std::wstring file_path;

  /* get full tree from game root directory */
  GMEnode* game_tree = new GMEnode();
  GME_TreeBuildFromDir(game_tree, game_path);

  /* we must exclude potential extra dir configured by user for
   OvGME, like backup dir or mods stock, we identify them first */

  std::wstring abs_path;
  std::vector<GMEnode*> rem_list;

  game_tree->initTraversal();
  while(game_tree->nextChild()) {

    SetDlgItemTextW(g_hwndSnapNew, TXT_COMMENT, game_tree->currChild()->getPath(true).c_str());

    if(game_tree->currChild()->isDir()) {
      abs_path = GME_GameGetCurRoot() + game_tree->currChild()->getPath(true);
      if(back_path == abs_path)
        rem_list.push_back(game_tree->currChild());
      if(mods_path == abs_path)
        rem_list.push_back(game_tree->currChild());
    }

    /* check if must abort */
    if(g_SnapProc_Cancel) {
      delete game_tree;
      g_SnapProc_Running = false;
      return 0;
    }
  }

  /* we now remove excluded branchs from tree */
  for(unsigned i = 0; i < rem_list.size(); i++) {
    rem_list[i]->setParent(NULL);
    delete rem_list[i];
  }

  /* get item count for progress bar */
  unsigned c = 0;
  game_tree->initTraversal();
  while(game_tree->nextChild()) c++;

  SetDlgItemTextW(g_hwndSnapNew, TXT_STATUS, L"Creating xxHash....");

  /* set progress bar */
  SendMessage(hpb, PBM_SETRANGE, 0, MAKELPARAM(0, c));
  SendMessage(hpb, PBM_SETSTEP, (WPARAM)1, 0);
  SendMessage(hpb, PBM_SETPOS, (WPARAM)0, 0);

  /* here we go, we can create our snapshot data */
  FILE* fp;

  GME_SnapEntry_Struct snpentry;
  std::vector<GME_SnapEntry_Struct> snpentry_list;

  game_tree->initTraversal();
  while(game_tree->nextChild()) {

    file_path = game_tree->currChild()->getPath(true);

    SetDlgItemTextW(g_hwndSnapNew, TXT_COMMENT, file_path.c_str());

    memset(&snpentry, 0, sizeof(snpentry));
    if(game_tree->currChild()->isDir()) {
      snpentry.is_dir = true;
      wcscpy(snpentry.path, file_path.c_str());
    } else {
      snpentry.is_dir = false;
      wcscpy(snpentry.path, file_path.c_str());
      /* get file xxHash32 */
      abs_path = GME_GameGetCurRoot() + file_path;
      snpentry.xhash = GME_FileGetXxH32(abs_path);
    }
    snpentry_list.push_back(snpentry);

    /* step progress bar */
    SendMessage(hpb, PBM_STEPIT, 0, 0);

    /* check if must abort */
    if(g_SnapProc_Cancel) {
      delete game_tree;
      g_SnapProc_Running = false;
      return 0;
    }
  }

  /* write the snashot data in config dir */
  std::wstring snp_file = conf_path + L"\\snapshot.dat";
  fp = _wfopen(snp_file.c_str(), L"wb");
  if(fp) {
    /* first 4 bytes is count of entries */
    unsigned c = snpentry_list.size();
    fwrite(&c, 4, 1, fp);
    for(unsigned i = 0; i < snpentry_list.size(); i++) {
      fwrite(&snpentry_list[i], sizeof(GME_SnapEntry_Struct), 1, fp);
    }
    fclose(fp);
  }

  delete game_tree;

  SetDlgItemTextW(g_hwndSnapNew, TXT_TITLE, L"Snapshot successfully completed !");
  ShowWindow(GetDlgItem(g_hwndSnapNew, IDOK), false);
  ShowWindow(GetDlgItem(g_hwndSnapNew, IDCANCEL), false);
  ShowWindow(GetDlgItem(g_hwndSnapNew, BTN_CLOSE), true);
  ShowWindow(GetDlgItem(g_hwndSnapNew, TXT_STATUS), false);
  ShowWindow(GetDlgItem(g_hwndSnapNew, TXT_COMMENT), false);
  //ShowWindow(GetDlgItem(g_hwndSnapNew, PBM_PROGRESS), false);

  SendMessage(hpb, PBM_SETPOS, (WPARAM)c, 0);

  /* update menus */
  GME_GameUpdMenu();

  g_SnapProc_Running = false;
  return 0;
}


/* threaded function to compare snapshot */
DWORD WINAPI GME_SnapCompare_Th(void* pargs)
{
  g_SnapProc_Running = true;

  HWND hpb = GetDlgItem(g_hwndSnapCmp, PBM_SNAP_CMP);
  HWND hen_output = GetDlgItem(g_hwndSnapCmp, ENT_OUTPUT);

  std::wstring game_path = GME_GameGetCurRoot();
  std::wstring back_path = GME_GameGetCurBackPath();
  std::wstring mods_path = GME_GameGetCurModsPath();
  std::wstring conf_path = GME_GameGetCurConfPath();
  std::wstring file_path;
  std::wstring abs_path;

  /* stuff for file */
  FILE* fp;

  /* sumary */
  std::wstring outlog;
  unsigned iteration = 0;
  unsigned changes = 0;
  unsigned addition = 0;
  unsigned removes = 0;
  clock_t time = clock();

  /* first we get the last snapshot */
  GME_SnapEntry_Struct snpentry;
  std::vector<GME_SnapEntry_Struct> snpentry_list;

  /* read the snashot data in config dir */
  std::wstring snp_file = conf_path + L"\\snapshot.dat";
  fp = _wfopen(snp_file.c_str(), L"rb");
  if(fp) {
    /* first 4 bytes is count of entries */
    unsigned c;
    fread(&c, 1, 4, fp);
    for(unsigned i = 0; i < c; i++) {
      fread(&snpentry, 1, sizeof(GME_SnapEntry_Struct), fp);
      snpentry_list.push_back(snpentry);
    }
    fclose(fp);
  }

  SetDlgItemTextW(g_hwndSnapCmp, TXT_STATUS, L"Analysing file tree...");

  /* get full tree from game root directory */
  GMEnode* game_tree = new GMEnode();
  GME_TreeBuildFromDir(game_tree, game_path);

  /* we must exclude potential extra dir configured by user for
   OvGME, like backup dir or mods stock, we identify them first */

  std::vector<GMEnode*> rem_list;

  game_tree->initTraversal();
  while(game_tree->nextChild()) {

    iteration++;

    SetDlgItemTextW(g_hwndSnapCmp, TXT_COMMENT, game_tree->currChild()->getPath(true).c_str());

    if(game_tree->currChild()->isDir()) {
      abs_path = GME_GameGetCurRoot() + game_tree->currChild()->getPath(true);
      if(back_path == abs_path)
        rem_list.push_back(game_tree->currChild());
      if(mods_path == abs_path)
        rem_list.push_back(game_tree->currChild());
    }

    /* check if must abort */
    if(g_SnapProc_Cancel) {
      delete game_tree;
      g_SnapProc_Running = false;
      return 0;
    }
  }

  /* we now remove excluded branchs from tree */
  for(unsigned i = 0; i < rem_list.size(); i++) {
    SetDlgItemTextW(g_hwndSnapCmp, ENT_OUTPUT, outlog.c_str());
    rem_list[i]->setParent(NULL);
    delete rem_list[i];
  }

  outlog += L"Type\tChange\tPath\r\n";
  SetDlgItemTextW(g_hwndSnapCmp, ENT_OUTPUT, outlog.c_str());

  /* temporary flag for file not found */
  bool not_found;

  /* get item count for progress bar */
  unsigned c = 0;
  game_tree->initTraversal();
  while(game_tree->nextChild()) c++;

  /* set progress bar */
  SendMessage(hpb, PBM_SETRANGE, 0, MAKELPARAM(0, c));
  SendMessage(hpb, PBM_SETSTEP, (WPARAM)1, 0);
  SendMessage(hpb, PBM_SETPOS, (WPARAM)0, 0);

  SetDlgItemTextW(g_hwndSnapCmp, TXT_STATUS, L"Comparing tree...");

  /* found file list */
  std::vector<bool> found_list;
  found_list.resize(snpentry_list.size(), false);

  /* check for added items */
  game_tree->initTraversal();
  while(game_tree->nextChild()) {

    /* step progress bar */
    SendMessage(hpb, PBM_STEPIT, 0, 0);

    file_path = game_tree->currChild()->getPath(true);

    SetDlgItemTextW(g_hwndSnapCmp, TXT_COMMENT, file_path.c_str());

    if(game_tree->currChild()->isDir()) {
      /* search for dir in the snapshot */
      not_found = true;
      for(unsigned i = 0; i < snpentry_list.size(); i++) {
        if(snpentry_list[i].path == file_path) {
          found_list[i] = true;
          not_found = false;
          break;
        }
      }
      if(not_found) {
        outlog += L"(d)\t+\t" + file_path + L"\r\n";
        SetDlgItemTextW(g_hwndSnapCmp, ENT_OUTPUT, outlog.c_str());
        SendMessage(hen_output, EM_SETSEL, 0, -1);
        SendMessage(hen_output, EM_SETSEL, -1, -1);
        SendMessage(hen_output, EM_SCROLLCARET, 0, 0);
        addition++;
      }
    } else {
      /* search for file in the snapshot */
      not_found = true;
      for(unsigned i = 0; i < snpentry_list.size(); i++) {
        if(snpentry_list[i].path == file_path) {
          found_list[i] = true;
          not_found = false;
          break;
        }
      }
      if(not_found) {
        outlog += L"(f)\t+\t" + file_path + L"\r\n";
        SetDlgItemTextW(g_hwndSnapCmp, ENT_OUTPUT, outlog.c_str());
        SendMessage(hen_output, EM_SETSEL, 0, -1);
        SendMessage(hen_output, EM_SETSEL, -1, -1);
        SendMessage(hen_output, EM_SCROLLCARET, 0, 0);
        addition++;
      }
    }

    /* check if must abort */
    if(g_SnapProc_Cancel) {
      delete game_tree;
      g_SnapProc_Running = false;
      return 0;
    }
  }

  /* check removed entries... */
  c = 0;
  for(unsigned i = 0; i < found_list.size(); i++) {
    if(!found_list[i]) {
      outlog += L"(f)\t-\t" + file_path + L"\r\n";
      SetDlgItemTextW(g_hwndSnapCmp, ENT_OUTPUT, outlog.c_str());
      SendMessage(hen_output, EM_SETSEL, 0, -1);
      SendMessage(hen_output, EM_SETSEL, -1, -1);
      SendMessage(hen_output, EM_SCROLLCARET, 0, 0);
      removes++;
    } else {
      c++; /* increase for progress bar */
    }
  }

  /* set progress bar */
  SendMessage(hpb, PBM_SETRANGE, 0, MAKELPARAM(0, c));
  SendMessage(hpb, PBM_SETSTEP, (WPARAM)1, 0);
  SendMessage(hpb, PBM_SETPOS, (WPARAM)0, 0);

  SetDlgItemTextW(g_hwndSnapCmp, TXT_STATUS, L"Comparing xxHash...");

  /* new compare entries for xxHash32... */
  for(unsigned i = 0; i < found_list.size(); i++) {
    if(found_list[i]) {
      if(!snpentry_list[i].is_dir) {

        SetDlgItemTextW(g_hwndSnapCmp, TXT_COMMENT, snpentry_list[i].path);

        abs_path = GME_GameGetCurRoot() + snpentry_list[i].path;
        /* check file xxHash32 */
        if(snpentry_list[i].xhash != GME_FileGetXxH32(abs_path)) {
          outlog += L"(f)\t~\t";
          outlog += snpentry_list[i].path;
          outlog += L"\r\n";
          SetDlgItemTextW(g_hwndSnapCmp, ENT_OUTPUT, outlog.c_str());
          SendMessage(hen_output, EM_SETSEL, 0, -1);
          SendMessage(hen_output, EM_SETSEL, -1, -1);
          SendMessage(hen_output, EM_SCROLLCARET, 0, 0);
          changes++;
        }
      }
      /* step progress bar */
      SendMessage(hpb, PBM_STEPIT, 0, 0);
    }

    /* check if must abort */
    if(g_SnapProc_Cancel) {
      delete game_tree;
      g_SnapProc_Running = false;
      return 0;
    }
  }

  outlog += L"\r\n";

  float duration = ((float)clock()-time)/CLOCKS_PER_SEC;
  int mn = duration / 60.0f;
  int sc = duration - (mn * 60.0f);

  wchar_t wpbuff[128];
  swprintf(wpbuff, 128, L"%d entries analysed in %d minute(s) and %d second(s).\r\n", iteration, mn, sc);
  outlog += wpbuff;
  swprintf(wpbuff, 128, L"%d addition(s).\r\n%d remove(s).\r\n%d change(s).\r\n", addition, removes, changes);
  outlog += wpbuff;

  SetDlgItemTextW(g_hwndSnapCmp, ENT_OUTPUT, outlog.c_str());
  SendMessage(hen_output, EM_SETSEL, 0, -1);
  SendMessage(hen_output, EM_SETSEL, -1, -1);
  SendMessage(hen_output, EM_SCROLLCARET, 0, 0);


  SetDlgItemTextW(g_hwndSnapCmp, TXT_TITLE, L"Comparison completed.");
  ShowWindow(GetDlgItem(g_hwndSnapCmp, IDOK), false);
  ShowWindow(GetDlgItem(g_hwndSnapCmp, IDCANCEL), false);
  ShowWindow(GetDlgItem(g_hwndSnapCmp, BTN_CLOSE), true);
  ShowWindow(GetDlgItem(g_hwndSnapCmp, TXT_STATUS), false);
  ShowWindow(GetDlgItem(g_hwndSnapCmp, TXT_COMMENT), false);
  //ShowWindow(GetDlgItem(g_hwndSnapCmp, PBM_PROGRESS), false);

  SendMessage(hpb, PBM_SETPOS, (WPARAM)c, 0);

  g_SnapProc_Running = false;
  return 0;
}

/*
  cancel a working snapshot process
*/
void GME_SnapCancel()
{
  if(g_SnapProc_Running) {
    g_SnapProc_Cancel = true;
  }
}

/*
  create snapshot for current game
*/
bool GME_SnapCreate()
{
  if(GME_GameGetCurId() == -1) {
    GME_DialogWarning(g_hwndSnapNew, L"No game selected.");
    return false;
  }

  /* and, here we go... */
  g_SnapProc_Cancel = false;
  g_SnapProc_hT = CreateThread(NULL,0,GME_SnapCreate_Th,NULL,0,&g_SnapProc_iT);

  return true;
}


/*
  create snapshot for current game
*/
bool GME_SnapCompare()
{
  if(GME_GameGetCurId() == -1) {
    GME_DialogWarning(g_hwndSnapCmp, L"No game selected.");
    return false;
  }

  if(!GME_IsFile(GME_GameGetCurConfPath() + L"\\snapshot.dat")) {
    GME_DialogWarning(g_hwndSnapCmp, L"No snapshot available for this game.");
    return false;
  }

  /* and, here we go... */
  g_SnapProc_Cancel = false;
  g_SnapProc_hT = CreateThread(NULL,0,GME_SnapCompare_Th,NULL,0,&g_SnapProc_iT);

  return true;
}

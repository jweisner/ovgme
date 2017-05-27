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

#include "gme_tools.h"
#include "gme_game.h"
#include "gme_mods.h"
#include "gme_prof.h"
#include "gme_logs.h"

/* macro for backup entry type */
#define GME_BCK_RESTORE_DELETE -1
#define GME_BCK_RESTORE_SWAP 1

/* backup entry struct */
struct GME_BckEntry_Struct
{
  int action;
  int isdir;
  wchar_t path[260];
};

/* global bool to now if list is empty */
bool g_ModsList_IsEmpty = true;

/*
  global stuff for threaded mod apply and restore
*/
/* thread handles */
HANDLE g_ModsProc_hT;
DWORD g_ModsProc_iT;
/* thread cancel flag */
bool g_ModsProc_Cancel;
bool g_ModsProc_Running;

/* structure for mod proc thread arguments list... */
struct GME_ModsProc_Arg_Struct
{
  int action;
  HWND hwnd;
  std::wstring name;
  int type;
};

/* arg list for threaded mods apply & restore */
std::vector<GME_ModsProc_Arg_Struct> g_ModsProc_ArgList;


/* structure for mod make thread arguments */
struct GME_ModsMake_Arg_Struct
{
  bool end_exit;
  GMEnode* zip_root;
  wchar_t zip_path[260];
  int zip_level;
};

/* thread handles */
HANDLE g_ModsMake_hT;
DWORD g_ModsMake_iT;
/* thread cancel flag */
bool g_ModsMake_Cancel;
bool g_ModsMake_Running;

/*
  function to safely clean threads and/or memory usage by mods process
*/
void GME_ModsClean()
{
  if(!GME_ModsProc_IsReady()) {
    g_ModsProc_Cancel = true;
  }

  if(g_ModsMake_Running) {
    g_ModsMake_Cancel = true;
  }
}

/*
  simply get empty status of the mod list
*/
bool GME_ModsListIsEmpty()
{
  return g_ModsList_IsEmpty;
}

/*
  function to undo Mod installation, same as Restore, but without error messages
*/
void GME_ModsUndoMod(HWND hpb, const std::vector<GME_BckEntry_Struct>& bckentry_list)
{
  GME_Logs(GME_LOG_NOTICE, "GME_ModsUndoMod", "Undoing mod", "...");

  std::wstring dst_path;
  std::wstring src_path;
  std::wstring back_path = GME_GameGetCurBackPath();
  std::wstring game_path = GME_GameGetCurRoot();
  std::wstring conf_path = GME_GameGetCurConfPath();

  /* a first pass to get processing file count */
  unsigned c = 0;
  for(unsigned i = 0; i < bckentry_list.size(); i++) {
    if(bckentry_list[i].action == GME_BCK_RESTORE_SWAP) {
      if(!bckentry_list[i].isdir) c++;
    }
  }

  /* set progress bar */
  SendMessage(hpb, PBM_SETRANGE, 0, MAKELPARAM(0, c));
  SendMessage(hpb, PBM_SETSTEP, (WPARAM)1, 0);
  SendMessage(hpb, PBM_SETPOS, (WPARAM)0, 0);

  /* read backup entries in backward so we process data
     in reversed depth-first order: begin by leafs, end by root */
  unsigned i = bckentry_list.size();
  while(i--) {
    dst_path = game_path + bckentry_list[i].path;
    if(bckentry_list[i].action == GME_BCK_RESTORE_DELETE) {
      if(bckentry_list[i].isdir) {
        if(GME_IsDir(dst_path)) {
          if(!GME_DirRemove(dst_path)) {
            GME_Logs(GME_LOG_WARNING, "GME_ModsUndoMod", "Unable to delete dir", GME_StrToMbs(dst_path).c_str());
          }
        }
      } else {
        if(GME_IsFile(dst_path)) {
          if(!GME_FileDelete(dst_path)) {
            GME_Logs(GME_LOG_WARNING, "GME_ModsUndoMod", "Unable to delete file", GME_StrToMbs(dst_path).c_str());
          }
        }
      }
    } else {
      if(bckentry_list[i].isdir) {
        // this should logically never happen...
      } else {
        src_path = back_path + bckentry_list[i].path;
        /* copy file, overwrite dest */
        if(!GME_FileCopy(src_path, dst_path, true)) {
          GME_Logs(GME_LOG_WARNING, "GME_ModsUndoMod", "File copy failed", GME_StrToMbs(dst_path).c_str());
        }
        /* step progress bar */
        SendMessage(hpb, PBM_STEPIT, 0, 0);
      }
    }
  }

  /* clean backup files */
  GME_ModsCleanBackup();

  SendMessage(hpb, PBM_SETPOS, (WPARAM)0, 0);
}

/*
  function to apply a mod: create backup, check overlap, apply changes...
*/
void GME_ModsApplyMod(HWND hpb, const std::wstring& name, int type)
{
  GME_Logs(GME_LOG_NOTICE, "GME_ModsApplyMod", "Applying mod", GME_StrToMbs(name).c_str());

  bool is_zip_mod;

  /* check if we have a dir or a zip */
  if(type == 0) {
    is_zip_mod = false;
  } else if(type == 1) {
    is_zip_mod = true;
  } else {
    GME_Logs(GME_LOG_WARNING, "GME_ModsApplyMod", "Invalid mod type", "Aborting");
    return;/* ni dir ni zip ? caca */
  }

  std::string mbs_path;
  std::wstring src_path;
  std::wstring dst_path;
  std::wstring rel_path;
  std::wstring game_path = GME_GameGetCurRoot();
  std::wstring mods_path = GME_GameGetCurModsPath();
  std::wstring conf_path = GME_GameGetCurConfPath();
  std::wstring back_path = GME_GameGetCurBackPath();

  /* check backup file */
  if(GME_IsFile(conf_path + L"\\" + name + L".bck")) {
    /* already a backup file, no apply */
    GME_Logs(GME_LOG_WARNING, "GME_ModsApplyMod", "Backup file already exists", "Aborting");
    return;
  }

  /* temporary for file read/write */
  FILE* fp;

  /* backup entry struct and list */
  GME_BckEntry_Struct bckentry;
  std::vector<GME_BckEntry_Struct> bckentry_list;

  /* ---------------------- create mod tree abstraction --------------------- */

  GMEnode* mod_tree;

  if(is_zip_mod) {
    mod_tree = new GMEnode();
    /* create mod tree from zip file */
    if(!GME_TreeBuildFromZip(mod_tree, mods_path + L"\\" + name + L".zip")) {
      GME_DialogError(g_hwndMain, L"Mod-Archive '" + name + L"' extraction failed, the Mod cannot be installed.");
      delete mod_tree;
      SendMessage(hpb, PBM_SETPOS, (WPARAM)0, 0);
      GME_Logs(GME_LOG_ERROR, "GME_ModsApplyMod", "Build tree from zip failed", "Aborting");
      return;
    }
    /* get the count of items for progress bar */
    unsigned c = 0;
    mod_tree->initTraversal();
    while(mod_tree->nextChild()) if(!mod_tree->currChild()->isDir()) c++;
    /* set the progress bar */
    SendMessage(hpb, PBM_SETRANGE, 0, MAKELPARAM(0, c*2));
    SendMessage(hpb, PBM_SETSTEP, (WPARAM)1, 0);
    SendMessage(hpb, PBM_SETPOS, (WPARAM)0, 0);
    /* re-root tree to have mod root folder as mod_tree root */
    GMEnode* mod_root = mod_tree;
    mod_tree = mod_root->getChild(name);
    mod_tree->setParent(NULL);
    delete mod_root;

    GME_Logs(GME_LOG_NOTICE, "GME_ModsApplyMod", "Create mod tree (zipped mod)", "Done");

  } else {
    /* here the root node must have the name of the mod root folder*/
    mod_tree = new GMEnode(name, true);
    /* create mod tree from directory  */
    GME_TreeBuildFromDir(mod_tree, mods_path);
    /* get the count of items for progress bar */
    unsigned c = 0;
    mod_tree->initTraversal();
    while(mod_tree->nextChild()) if(!mod_tree->currChild()->isDir()) c++;
    /* set the progress bar */
    SendMessage(hpb, PBM_SETRANGE, 0, MAKELPARAM(0, c*2));
    SendMessage(hpb, PBM_SETSTEP, (WPARAM)1, 0);
    SendMessage(hpb, PBM_SETPOS, (WPARAM)0, 0);

    GME_Logs(GME_LOG_NOTICE, "GME_ModsApplyMod", "Create mod tree (folder mod)", "Done");
  }



  /* ------------------------- check mod overlap -------------------------- */

  std::vector<std::wstring> imod_flist;
  std::vector<std::wstring> dpnd_flist;
  std::vector<std::wstring> olap_flist;

  /* create mod file list */
  mod_tree->initTraversal();
  while(mod_tree->nextChild()) {
    if(!mod_tree->currChild()->isDir()) {
      imod_flist.push_back(mod_tree->currChild()->getPath(true));
    }
  }

  /* temporary unsigned entry count */
  unsigned c;

  /* path for bck file, used just below and after */
  std::wstring bck_file;
  std::wstring olap_msg;

  /* search for each .bck file in current game config folder */
  std::wstring bck_srch = conf_path + L"\\*.bck";
  WIN32_FIND_DATAW fdw;
  HANDLE hnd = FindFirstFileW(bck_srch.c_str(), &fdw);
  if(hnd != INVALID_HANDLE_VALUE) {
    do {
      if(!(fdw.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        bck_file = conf_path + L"\\" + fdw.cFileName;
        /* read backup data entry */
        dpnd_flist.clear();
        if(NULL != (fp = _wfopen(bck_file.c_str(), L"rb"))) {
          /* first 4 bytes is count of entries */
          fread(&c, 1, 4, fp);
          for(unsigned i = 0; i < c; i++) {
            fread(&bckentry, 1, sizeof(GME_BckEntry_Struct), fp);
            if(bckentry.action == GME_BCK_RESTORE_SWAP && !bckentry.isdir) {
              dpnd_flist.push_back(bckentry.path);
            }
          }
          fclose(fp);
        }

        if(dpnd_flist.empty())
          continue;

        /* cross check between mod files and bck files */
        olap_flist.clear();
        for(unsigned i = 0; i < imod_flist.size(); i++) {
          for(unsigned j = 0; j < dpnd_flist.size(); j++) {
            if(dpnd_flist[j].find(imod_flist[i]) != std::wstring::npos) {
              olap_flist.push_back(imod_flist[i]);
            }
          }
        }

        /* if we get files overlap */
        if(!olap_flist.empty()) {

          /* create LE message... */
          olap_msg = L"The Mod '" + name + L"' files overlap with the already installed Mod: '" + GME_FilePathToName(fdw.cFileName) + L"'.";
          olap_msg += L"\n\nOverlapped file(s):\n";
          for(unsigned k = 0; k < olap_flist.size(); k++) olap_msg += L"  " + olap_flist[k] + L"\n";
          olap_msg += L"\nThis will alter already installed Mod files, do you want to install it anyway ?";

          GME_Logs(GME_LOG_WARNING, "GME_ModsApplyMod", "Mod overlaps with", GME_StrToMbs(fdw.cFileName).c_str());

          /* show confirmation dialog */
          if(IDYES != GME_DialogWarningConfirm(g_hwndMain, olap_msg)) {
            delete mod_tree;
            FindClose(hnd);
            SendMessage(hpb, PBM_SETPOS, (WPARAM)0, 0);
            GME_Logs(GME_LOG_WARNING, "GME_ModsApplyMod", "Mod apply canceled by user", GME_StrToMbs(name).c_str());
            return;
          }
        }
      }
    } while(FindNextFileW(hnd, &fdw));
  }
  FindClose(hnd);

  GME_Logs(GME_LOG_NOTICE, "GME_ModsApplyMod", "Check mod overlap", "Done");

  /* -------------------------- add backup data -------------------------- */

  /* copy/backup original files and create backup entries */
  mod_tree->initTraversal();
  while(mod_tree->nextChild()) {

    rel_path = mod_tree->currChild()->getPath(true);

    /* we first create the backup entry */
    memset(&bckentry, 0, sizeof(GME_BckEntry_Struct));
    if(mod_tree->currChild()->isDir()) {
      bckentry.isdir = 1;
      /* check if folder exists in game */
      if(GME_IsDir(game_path + rel_path)) {
        dst_path = back_path + rel_path;
        /* this is useless, but we keep data, maybe useful later */
        bckentry.action = GME_BCK_RESTORE_SWAP;
        if(!GME_IsDir(dst_path)) {
          if(!GME_DirCreate(dst_path)) {
            GME_DialogError(g_hwndMain, L"Backup creation error for Mod '" + name + L"', the Mod cannot be installed.");
            GME_Logs(GME_LOG_ERROR, "GME_ModsApplyMod", "Unable to backup dir", GME_StrToMbs(dst_path).c_str());
            GME_ModsUndoMod(hpb, bckentry_list);
            delete mod_tree;
            return;
          }
        }
      } else {
        bckentry.action = GME_BCK_RESTORE_DELETE;
      }
    } else {
      bckentry.isdir = 0;
      /* check if file exists in game */
      if(GME_IsFile(game_path + rel_path)) {
        bckentry.action = GME_BCK_RESTORE_SWAP;
        dst_path = back_path + rel_path;
        src_path = game_path + rel_path;
        /* copy file, no overwrite dest */
        if(!GME_FileCopy(src_path, dst_path, false)) {
          GME_DialogError(g_hwndMain, L"Backup creation error for Mod '" + name + L"', the Mod cannot be installed.");
          GME_Logs(GME_LOG_ERROR, "GME_ModsApplyMod", "Unable to backup file", GME_StrToMbs(src_path).c_str());
          GME_ModsUndoMod(hpb, bckentry_list);
          delete mod_tree;
          return;
        }
      } else {
        bckentry.action = GME_BCK_RESTORE_DELETE;
      }
      /* step progress bar */
      SendMessage(hpb, PBM_STEPIT, 0, 0);
    }
    wcscpy(bckentry.path, rel_path.c_str());
    bckentry_list.push_back(bckentry);

    if(g_ModsProc_Cancel) {
      GME_Logs(GME_LOG_WARNING, "GME_ModsApplyMod", "Mod apply canceled by user", GME_StrToMbs(name).c_str());
      delete mod_tree;
      SendMessage(hpb, PBM_SETPOS, (WPARAM)0, 0);
      GME_ModsUndoMod(hpb, bckentry_list);
      return;
    }
  }

  /* write the backup entry data */
  bck_file = conf_path + L"\\" + name + L".bck";
  fp = _wfopen(bck_file.c_str(), L"wb");
  if(fp) {
    /* first 4 bytes is count of entries */
    unsigned c = bckentry_list.size();
    fwrite(&c, 4, 1, fp);
    for(unsigned i = 0; i < c; i++) {
      fwrite(&bckentry_list[i], sizeof(GME_BckEntry_Struct), 1, fp);
    }
    fclose(fp);
  }

  GME_Logs(GME_LOG_NOTICE, "GME_ModsApplyMod", "Create backup data", "Done");

  /* -------------------------- apply mod files -------------------------- */

  /* special zip stuff...  */
  mz_zip_archive za; /* miniz zip archive handle */

  if(is_zip_mod) {
    memset(&za, 0, sizeof(mz_zip_archive));
    if(!mz_zip_reader_init_file(&za, GME_StrToMbs(mods_path + L"\\" + name + L".zip").c_str(), 0)) {
      GME_DialogError(g_hwndMain, L"Mod-Archive '" + name + L"' Zip extraction error, the Mod cannot be installed.");
      GME_Logs(GME_LOG_ERROR, "GME_ModsApplyMod", "Zip reader init (extraction) failed", GME_StrToMbs(name).c_str());
      delete mod_tree;
      SendMessage(hpb, PBM_SETPOS, (WPARAM)0, 0);
      GME_ModsUndoMod(hpb, bckentry_list);
      return;
    }
  }

  mod_tree->initTraversal();
  while(mod_tree->nextChild()) {

    dst_path = game_path + mod_tree->currChild()->getPath(true);

    if(mod_tree->currChild()->isDir()) {
      /* create new directory */
      if(!GME_IsDir(dst_path)) {
        if(!GME_DirCreate(dst_path)) {
          if(is_zip_mod) mz_zip_reader_end(&za);
          GME_DialogError(g_hwndMain, L"Create directory error for Mod '" + name + L"', the Mod cannot be installed.");
          GME_Logs(GME_LOG_ERROR, "GME_ModsApplyMod", "Unable to create dir", GME_StrToMbs(dst_path).c_str());
          delete mod_tree;
          SendMessage(hpb, PBM_SETPOS, (WPARAM)0, 0);
          GME_FileDelete(bck_file);
          GME_ModsUndoMod(hpb, bckentry_list);
          return;
        }
      }
    } else {
      /* overwrite or create file */
      if(is_zip_mod) {

        /* extract to file */
        if(!mz_zip_reader_extract_to_file(&za, mod_tree->currChild()->getId(), GME_StrToMbs(dst_path).c_str(), 0)) {
          mz_zip_reader_end(&za);
          GME_DialogError(g_hwndMain, L"Mod-Archive '" + name + L"' Zip extraction error, the Mod cannot be installed.");
          GME_Logs(GME_LOG_ERROR, "GME_ModsApplyMod", "Zip extract to file (extraction) failed", GME_StrToMbs(mod_tree->currChild()->getPath(true)).c_str());
          delete mod_tree;
          SendMessage(hpb, PBM_SETPOS, (WPARAM)0, 0);
          GME_FileDelete(bck_file);
          GME_ModsUndoMod(hpb, bckentry_list);
          return;
        }

      } else {
        /* copy with overwrite */
        if(!GME_FileCopy(mod_tree->currChild()->getSource(), dst_path, true)) {
          mz_zip_reader_end(&za);
          GME_DialogError(g_hwndMain, L"File copy error for Mod '" + name + L"', the Mod cannot be installed.");
          GME_Logs(GME_LOG_ERROR, "GME_ModsApplyMod", "File copy failed", GME_StrToMbs(mod_tree->currChild()->getSource()).c_str());
          delete mod_tree;
          SendMessage(hpb, PBM_SETPOS, (WPARAM)0, 0);
          GME_FileDelete(bck_file);
          GME_ModsUndoMod(hpb, bckentry_list);
          return;
        }
      }
      /* step progress bar */
      SendMessage(hpb, PBM_STEPIT, 0, 0);
    }

    if(g_ModsProc_Cancel) {
      GME_Logs(GME_LOG_WARNING, "GME_ModsApplyMod", "Mod apply canceled by user", GME_StrToMbs(name).c_str());
      if(is_zip_mod) mz_zip_reader_end(&za);
      delete mod_tree;
      SendMessage(hpb, PBM_SETPOS, (WPARAM)0, 0);
      GME_FileDelete(bck_file);
      GME_ModsUndoMod(hpb, bckentry_list);
      return;
    }
  }

  if(is_zip_mod) mz_zip_reader_end(&za);

  delete mod_tree;

  SendMessage(hpb, PBM_SETPOS, (WPARAM)0, 0);

  GME_Logs(GME_LOG_NOTICE, "GME_ModsApplyMod", "Copy to destination", "Done");

}


/*
  function to restore initial game data from backup (i.e. disabling a mod)
*/
void GME_ModsRestoreMod(HWND hpb, const std::wstring& name)
{
  GME_Logs(GME_LOG_NOTICE, "GME_ModsRestoreMod", "Restoring mod", GME_StrToMbs(name).c_str());

  std::wstring dst_path;
  std::wstring src_path;
  std::wstring back_path = GME_GameGetCurBackPath();
  std::wstring game_path = GME_GameGetCurRoot();
  std::wstring conf_path = GME_GameGetCurConfPath();

  /* read the backup entry data */
  std::wstring bck_file = conf_path + L"\\" + name + L".bck";

  /* check backup file */
  if(!GME_IsFile(bck_file)) {
    /* no backup file, no restore */
    GME_Logs(GME_LOG_NOTICE, "GME_ModsRestoreMod", "Check for backup data", "Mod already disabled");
    return;
  }

  /* temporary to read/write file data */
  FILE* fp;

  /* backup entry list */
  GME_BckEntry_Struct bckentry;
  std::vector<GME_BckEntry_Struct> bckentry_list;

  fp = _wfopen(bck_file.c_str(), L"rb");
  if(fp) {
    /* first 4 bytes is count of entries */
    unsigned c;
    fread(&c, 1, 4, fp);
    for(unsigned i = 0; i < c; i++) {
      memset(&bckentry, 0, sizeof(GME_BckEntry_Struct));
      fread(&bckentry, 1, sizeof(GME_BckEntry_Struct), fp);
      bckentry_list.push_back(bckentry);
    }
    fclose(fp);
  }

  /* set progress bar */
  SendMessage(hpb, PBM_SETRANGE, 0, MAKELPARAM(0, bckentry_list.size()));
  SendMessage(hpb, PBM_SETSTEP, (WPARAM)1, 0);
  SendMessage(hpb, PBM_SETPOS, (WPARAM)0, 0);

  bool got_error = false;

  /* read backup entries in backward so we process data
     in reversed depth-first order: begin by leafs, end by root */
  unsigned i = bckentry_list.size();
  while(i--) {
    dst_path = game_path + bckentry_list[i].path;
    if(bckentry_list[i].action == GME_BCK_RESTORE_DELETE) {
      if(bckentry_list[i].isdir) {
        if(GME_IsDir(dst_path)) {
          if(!GME_DirRemove(dst_path)) {
            GME_Logs(GME_LOG_WARNING, "GME_ModsRestoreMod", "Unable to delete dir", GME_StrToMbs(dst_path).c_str());
            got_error = true;
          }
        } else {
        }
      } else {
        if(GME_IsFile(dst_path)) {
          if(!GME_FileDelete(dst_path)) {
            GME_Logs(GME_LOG_WARNING, "GME_ModsRestoreMod", "Unable to delete file", GME_StrToMbs(dst_path).c_str());
            got_error = true;
          }
        } else {
          GME_Logs(GME_LOG_NOTICE, "GME_ModsRestoreMod", "Unable to delete file (it does not exists)", GME_StrToMbs(dst_path).c_str());
        }
      }
    } else {
      if(bckentry_list[i].isdir) {
        // this should logically never happen...
      } else {
        src_path = back_path + bckentry_list[i].path;
        /* copy file, overwrite dest */
        if(!GME_FileCopy(src_path, dst_path, true)) {
          GME_Logs(GME_LOG_WARNING, "GME_ModsRestoreMod", "File copy failed", GME_StrToMbs(dst_path).c_str());
          got_error = true;
        }
      }
    }
    /* step progress bar */
    SendMessage(hpb, PBM_STEPIT, 0, 0);
  }

  GME_FileDelete(bck_file); /* delete .bck file */

  SendMessage(hpb, PBM_SETPOS, (WPARAM)0, 0);

  if(got_error) {
    GME_DialogWarning(g_hwndMain, L"Restore process for Mod '" + name + L"' encountered one or more error, please see log text for more details.");
  }
}


/*
  function to clean backup files according installed mods dependencies
*/
void GME_ModsCleanBackup()
{
  GME_Logs(GME_LOG_NOTICE, "GME_ModsCleanBackup", "Cleaning backup tree", GME_StrToMbs(GME_GameGetCurBackPath()).c_str());

  std::wstring conf_path = GME_GameGetCurConfPath();
  std::wstring back_path = GME_GameGetCurBackPath();

  /* the backup dependency list */
  std::vector<std::wstring> dpend_list;

  /* temporary to read/write file data */
  FILE* fp;

  /* temporary for backup entry .bck file path */
  std::wstring bck_file;

  /* temporary backup entry struct */
  GME_BckEntry_Struct bckentry;

  /* temporary unsigned backup entry count */
  unsigned c;

  /* open all backup entry data files and gather dependencies */
  std::wstring gbck_srch = conf_path + L"\\*.bck";
  WIN32_FIND_DATAW fdw;
  HANDLE hnd = FindFirstFileW(gbck_srch.c_str(), &fdw);
  if(hnd != INVALID_HANDLE_VALUE) {
    do {
      if(!(fdw.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        /* read backup entry .bck file */
        bck_file = conf_path + L"\\" + fdw.cFileName;
        if(NULL != (fp = _wfopen(bck_file.c_str(), L"rb"))) {
          /* first 4 bytes is count of entries */
          fread(&c, 1, 4, fp);
          for(unsigned i = 0; i < c; i++) {
            fread(&bckentry, 1, sizeof(GME_BckEntry_Struct), fp);
            if(bckentry.action == GME_BCK_RESTORE_SWAP && !bckentry.isdir) {
              /* name to upper case to avoid case sensitive */
              dpend_list.push_back(GME_StrToUpper(bckentry.path));
            }
          }
          fclose(fp);
        } else {
          GME_Logs(GME_LOG_ERROR, "GME_ModsCleanBackup", "Unable to open backup file", GME_StrToMbs(bck_file).c_str());
        }
      }
    } while(FindNextFileW(hnd, &fdw));
  }
  FindClose(hnd);


  /* list for directory to be removed if empty */
  std::vector<std::wstring> rmdir_list;

  std::wstring dst_path;
  std::wstring src_path;

  bool is_depend; /* depend file flag */

  /* create node tree from backup folder tree */
  GMEnode* back_tree = new GMEnode();
  GME_TreeBuildFromDir(back_tree, back_path);

  /* remove files no longer needed */
  back_tree->initTraversal();
  while(back_tree->nextChild()) {
    if(!back_tree->currChild()->isDir()) {
      /* upper case to prevent case sensitive problems... */
      dst_path = GME_StrToUpper(back_tree->currChild()->getPath(true));
      /* check if backup file is in the depend list */
      is_depend = false;
      for(unsigned i = 0; i < dpend_list.size(); i++) {
        if(dst_path == dpend_list[i]) {
          is_depend = true; break;
        }
      }
      /* backup file is no longer in the depend list, we should remove it */
      if(!is_depend) {
        dst_path = back_path + back_tree->currChild()->getPath(true);
        if(!GME_FileDelete(dst_path)) {
          GME_Logs(GME_LOG_WARNING, "GME_ModsCleanBackup", "Unable to delete file", GME_StrToMbs(dst_path).c_str());
        }
      }
    } else {
      /* keep directory path in list for cleaning  */
      rmdir_list.push_back(back_tree->currChild()->getPath(true));
    }
  }

  delete back_tree;

  /* check if directories are empty and remove
    all in backward to invert depth-first order */
  unsigned i = rmdir_list.size();
  while(i--) {
    dst_path = back_path + rmdir_list[i];
    if(PathIsDirectoryEmptyW(dst_path.c_str())) {
      if(!GME_DirRemove(dst_path)) {
        GME_Logs(GME_LOG_WARNING, "GME_ModsCleanBackup", "Unable to delete dir", GME_StrToMbs(dst_path).c_str());
      }
    }
  }
}


/*
  function to enable, disable or toggle selected mods in list view
*/
bool GME_ModsToggleSel(int action)
{
  if(!GME_ModsProc_IsReady()) {
    GME_DialogWarning(g_hwndMain, L"Mod(s) installation is currently processing, please wait until current process finish before enabling or disabling mod(s).");
    return false;
  }

  HWND hlv = GetDlgItem(g_hwndMain, LVM_MODSLIST);

  std::vector<std::wstring> name_list;
  std::vector<int> type_list;
  wchar_t name_buff[255];

  /* get list of selected item in the list view control */
  LV_ITEMW lvitm;
  memset(&lvitm, 0, sizeof(LV_ITEMW));
  lvitm.mask = LVIF_TEXT|LVIF_IMAGE;
  lvitm.cchTextMax = 255;
  lvitm.pszText = name_buff;

  unsigned c = SendMessageW(hlv, LVM_GETITEMCOUNT, 0, 0);
  for(unsigned i = 0; i < c; i++) {
    if(SendMessageW(hlv, LVM_GETITEMSTATE, i, LVIS_SELECTED)) {
      lvitm.iItem = i;
      SendMessageW(hlv ,LVM_GETITEMW, 0, (LPARAM)&lvitm);
      name_list.push_back(lvitm.pszText);
      type_list.push_back(lvitm.iImage);
    }
  }

  /* enable, disable or toggle items */
  for(unsigned i = 0; i < name_list.size(); i++) {
    if(action == MODS_ENABLE) {
      if(GME_IsFile(GME_GameGetCurConfPath() + L"\\" + name_list[i] + L".bck")) {
        continue;
      } else {
        GME_ModsProc_PushApply(name_list[i], type_list[i]);

      }
    }
    if(action == MODS_DISABLE) {
      if(GME_IsFile(GME_GameGetCurConfPath() + L"\\" + name_list[i] + L".bck")) {
        GME_ModsProc_PushRestore(name_list[i]);
      } else {
        continue;
      }
    }
    if(action == MODS_TOGGLE) {
      if(GME_IsFile(GME_GameGetCurConfPath() + L"\\" + name_list[i] + L".bck")) {
        GME_ModsProc_PushRestore(name_list[i]);
      } else {
        GME_ModsProc_PushApply(name_list[i], type_list[i]);
      }
    }
  }

  GME_ModsProc_Launch();

  return true;
}


/*
  function to enable, disable or toggle all mods in list view
*/
bool GME_ModsToggleAll(int action)
{
  if(!GME_ModsProc_IsReady()) {
    GME_DialogWarning(g_hwndMain, L"Mod(s) installation is currently processing, please wait until current process finish before enabling or disabling Mod(s).");
    return false;
  }

  HWND hlv = GetDlgItem(g_hwndMain, LVM_MODSLIST);

  /* get list of all item in the list view control */
  std::vector<std::wstring> name_list;
  std::vector<int> type_list;
  wchar_t name_buff[255];

  LV_ITEMW lvitm;
  memset(&lvitm, 0, sizeof(LV_ITEMW));
  lvitm.mask = LVIF_TEXT|LVIF_IMAGE;
  lvitm.cchTextMax = 255;
  lvitm.pszText = name_buff;

  unsigned c = SendMessageW(hlv, LVM_GETITEMCOUNT, 0, 0);
  for(unsigned i = 0; i < c; i++) {
    lvitm.iItem = i;
    SendMessageW(hlv ,LVM_GETITEMW, 0, (LPARAM)&lvitm);
    name_list.push_back(lvitm.pszText);
    type_list.push_back(lvitm.iImage);
  }

  /* enable, disable or toggle items */
  for(unsigned i = 0; i < name_list.size(); i++) {
    if(action == MODS_ENABLE) {
      if(GME_IsFile(GME_GameGetCurConfPath() + L"\\" + name_list[i] + L".bck")) {
        continue;
      } else {
        GME_ModsProc_PushApply(name_list[i], type_list[i]);
      }
    }
    if(action == MODS_DISABLE) {
      if(GME_IsFile(GME_GameGetCurConfPath() + L"\\" + name_list[i] + L".bck")) {
        GME_ModsProc_PushRestore(name_list[i]);
      } else {
        continue;
      }
    }
    if(action == MODS_TOGGLE) {
      if(GME_IsFile(GME_GameGetCurConfPath() + L"\\" + name_list[i] + L".bck")) {
        GME_ModsProc_PushRestore(name_list[i]);
      } else {
        GME_ModsProc_PushApply(name_list[i], type_list[i]);
      }
    }
  }

  GME_ModsProc_Launch();

  return true;
}


/*
  import one or more mod archive to current mods stock
*/
bool GME_ModsImport()
{
  wchar_t flist[2048];
  unsigned path_offset;

  if(!GME_DialogFileOpen(g_hwndMain, flist, sizeof(flist), &path_offset, L"Mod-Archive (*.zip)\0*.ZIP;\0", L"")) {
    return false;
  }

  std::wstring dst_path = GME_GameGetCurModsPath();

  wchar_t buff[260];
  wcsncpy(buff, flist, path_offset);
  std::wstring src_path = buff;

  /* if multi-selection, we must add the '\' at the end of path... */
  if(src_path[src_path.size()-1] != L'\\') {
    src_path.append(1, L'\\');
  }

  /* create the file path list from 0/ separated list  */
  std::wstring fname;
  std::vector<std::wstring> fmod_list;
  wchar_t* pstr = flist + path_offset;
  do {

    if(*pstr) {
      fname = pstr;
      fmod_list.push_back(fname);
    }
    pstr += ( fname.size() + 1 );

  } while( *pstr );

  std::wstring src, dst;
  for(unsigned i = 0; i < fmod_list.size(); i++) {

    src = src_path + fmod_list[i];

    if(GME_ZipIsValidMod(src)) {
      /* copy file to mods stock folder */
      dst = GME_GameGetCurModsPath() + L"\\" + fmod_list[i];
      if(!GME_FileCopy(src, dst, true)) {
        GME_Logs(GME_LOG_ERROR, "GME_ModsImport", "Unable to copy file ", GME_StrToMbs(dst).c_str());
      }
    } else {
      GME_DialogWarning(g_hwndMain, L"The file '" + fmod_list[i] + L"' is not a valid Mod-Archive and will not be imported.");
    }
  }

  return true;
}


/*
  function to display mod description when single selection is made
*/
bool GME_ModsChkDesc()
{
  HWND hlv = GetDlgItem(g_hwndMain, LVM_MODSLIST);
  HWND het = GetDlgItem(g_hwndMain, ENT_MODDESC);

  /* get current mod list selection */
  std::vector<std::wstring> name_list;
  std::vector<int> type_list;
  wchar_t name_buff[255];

  LV_ITEMW lvitm;
  memset(&lvitm, 0, sizeof(LV_ITEMW));
  lvitm.mask = LVIF_TEXT|LVIF_IMAGE;
  lvitm.cchTextMax = 255;
  lvitm.pszText = name_buff;

  unsigned c = SendMessageW(hlv, LVM_GETITEMCOUNT, 0, 0);
  for(unsigned i = 0; i < c; i++) {
    if(SendMessageW(hlv, LVM_GETITEMSTATE, i, LVIS_SELECTED)) {
      lvitm.iItem = i;
      SendMessageW(hlv ,LVM_GETITEMW, 0, (LPARAM)&lvitm);
      name_list.push_back(lvitm.pszText);
      type_list.push_back(lvitm.iImage);
    }
  }

  /* if it is a single selection */
  if(name_list.size() == 1) {

    std::wstring content;

    switch(type_list[0])
    {
    case 0:
      if(GME_IsFile(GME_GameGetCurModsPath() + L"\\" + name_list[0] + L".txt")) {
        GME_FileGetAsciiContent(GME_GameGetCurModsPath() + L"\\" + name_list[0] + L".txt", &content);
      }
      break;
    case 1:
      GME_ZipGetModDesc(GME_GameGetCurModsPath() + L"\\" + name_list[0] + L".zip", &content);
      break;
    case 2:
      if(!GME_ZipGetModDesc(GME_GameGetCurModsPath() + L"\\" + name_list[0] + L".zip", &content)) {
        if(GME_IsFile(GME_GameGetCurModsPath() + L"\\" + name_list[0] + L".txt")) {
          GME_FileGetAsciiContent(GME_GameGetCurModsPath() + L"\\" + name_list[0] + L".txt", &content);
        }
      }
    }

    if(content.empty())
      content = L"No description available.";

    SendMessageW(het, WM_SETTEXT, 0, (LPARAM)content.c_str());

  } else {
    SendMessageW(het, WM_SETTEXT, 0, (LPARAM)L"[Multiple selection]");
  }

  return true;
}


/*
  function to explore folder or zip file
*/
void GME_ModsExploreCur()
{
  HWND hlv = GetDlgItem(g_hwndMain, LVM_MODSLIST);

  /* get current mod list selection */
  std::vector<std::wstring> name_list;
  std::vector<int> type_list;
  wchar_t name_buff[255];

  LV_ITEMW lvitm;
  memset(&lvitm, 0, sizeof(LV_ITEMW));
  lvitm.mask = LVIF_TEXT|LVIF_IMAGE;
  lvitm.cchTextMax = 255;
  lvitm.pszText = name_buff;

  unsigned c = SendMessageW(hlv, LVM_GETITEMCOUNT, 0, 0);
  for(unsigned i = 0; i < c; i++) {
    if(SendMessageW(hlv, LVM_GETITEMSTATE, i, LVIS_SELECTED)) {
      lvitm.iItem = i;
      SendMessageW(hlv ,LVM_GETITEMW, 0, (LPARAM)&lvitm);
      name_list.push_back(lvitm.pszText);
      type_list.push_back(lvitm.iImage);
      break; // single selection
    }
  }

  /* ready for multiple selection, but, dangerous but only one is authorised */
  for(unsigned i = 0; i < name_list.size(); i++) {

    std::wstring mod_path = GME_GameGetCurModsPath() + L"\\" + name_list[i];
    switch(type_list[i])
    {
    case 0:
      ShellExecuteW(g_hwndMain, L"explore", mod_path.c_str(), NULL, NULL, SW_NORMAL );
      break;
    case 1:
      mod_path += L".zip";
      ShellExecuteW(g_hwndMain, L"open", mod_path.c_str(), NULL, NULL, SW_NORMAL );
      break;
    case 2:
      if(GME_IsDir(mod_path)) {
        ShellExecuteW(g_hwndMain, L"explore", mod_path.c_str(), NULL, NULL, SW_NORMAL );
      }
      if(GME_ZipIsValidMod(mod_path + L".zip")) {
        mod_path += L".zip";
        ShellExecuteW(g_hwndMain, L"open", mod_path.c_str(), NULL, NULL, SW_NORMAL );
      }
      break;
    }
  }
}

/*
  function to remove Mod (to trash)
*/
void GME_ModsDeleteCur()
{
  HWND hlv = GetDlgItem(g_hwndMain, LVM_MODSLIST);

  /* get current mod list selection */
  std::vector<std::wstring> name_list;
  std::vector<int> type_list;
  wchar_t name_buff[255];

  LV_ITEMW lvitm;
  memset(&lvitm, 0, sizeof(LV_ITEMW));
  lvitm.mask = LVIF_TEXT|LVIF_IMAGE;
  lvitm.cchTextMax = 255;
  lvitm.pszText = name_buff;

  unsigned c = SendMessageW(hlv, LVM_GETITEMCOUNT, 0, 0);
  for(unsigned i = 0; i < c; i++) {
    if(SendMessageW(hlv, LVM_GETITEMSTATE, i, LVIS_SELECTED)) {
      lvitm.iItem = i;
      SendMessageW(hlv ,LVM_GETITEMW, 0, (LPARAM)&lvitm);
      name_list.push_back(lvitm.pszText);
      type_list.push_back(lvitm.iImage);
      break; // single selection
    }
  }

  /* ready for multiple selection, but, dangerous but only one is authorised */
  for(unsigned i = 0; i < name_list.size(); i++) {


    std::wstring mod_path = GME_GameGetCurModsPath() + L"\\" + name_list[i];

    GME_Logs(3, "GME_ModsDeleteCur", "Deleting mod", GME_StrToMbs(mod_path).c_str());

    switch(type_list[i])
    {
    case 0:
      if(IDYES == GME_DialogWarningConfirm(g_hwndMain, L"Are you sure you want to move the Mod '" + name_list[i] + L"' to Recycle Bin ?")){
        GME_DirRemToTrash(mod_path);
      }
      break;
    case 1:
      if(IDYES == GME_DialogWarningConfirm(g_hwndMain, L"Are you sure you want to move the Mod '" + name_list[i] + L"' to Recycle Bin ?")){
        mod_path += L".zip";
        GME_DirRemToTrash(mod_path);
      }
      break;
    case 2:
      GME_DialogWarning(g_hwndMain, L"Please disable the Mod before...");
      break;
    }
  }
}


/*
  function to quick enable or disable mods item in list
*/
void GME_ModsListQuickEnable(const std::wstring& name, bool enable)
{
  HWND hlv = GetDlgItem(g_hwndMain, LVM_MODSLIST);

  /* save current game mod list status */
  wchar_t name_buff[255];

  LV_ITEMW lvitm;
  memset(&lvitm, 0, sizeof(LV_ITEMW));
  lvitm.mask = LVIF_TEXT|LVIF_IMAGE;
  lvitm.cchTextMax = 255;
  lvitm.pszText = name_buff;

  unsigned c = SendMessageW(hlv, LVM_GETITEMCOUNT, 0, 0);
  for(unsigned i = 0; i < c; i++) {
    lvitm.iItem = i;
    SendMessageW(hlv ,LVM_GETITEMW, 0, (LPARAM)&lvitm);
    if(name_buff == name) {
      if(enable) {
        lvitm.iImage = 2;
      } else {
        if(GME_IsDir(GME_GameGetCurModsPath() + L"\\" + name)) {
          lvitm.iImage = 0;
        } else {
          lvitm.iImage = 1;
        }
      }
      SendMessageW(hlv, LVM_SETITEMW, 0, (LPARAM)&lvitm);
      return;
    }
  }
}


/*
  function to create or update the mods list (list view control)
*/
bool GME_ModsUpdList()
{
  HWND hlv = GetDlgItem(g_hwndMain, LVM_MODSLIST);
  HWND het_path = GetDlgItem(g_hwndMain, ENT_MODSPATH);
  HWND het_desc = GetDlgItem(g_hwndMain, ENT_MODDESC);

  /* Save scroll position to lvmRect */
  RECT lvmRect = {};
  SendMessageW(hlv, LVM_GETVIEWRECT, 0, (LPARAM)&lvmRect);

  wchar_t wbuff[260];

  /* empty path entry */
  wbuff[0] = 0;
  SendMessageW(het_path, WM_SETTEXT, 0, (LPARAM)wbuff[0]);

  /* empty description */
  SendMessageW(het_desc, WM_SETTEXT, 0, (LPARAM)wbuff[0]);

  /* empty list view */
  SendMessageW(hlv, LVM_DELETEALLITEMS, 0, 0);

  /* disable proper menu items */
  g_ModsList_IsEmpty = true;

  /* check if any game is selected */
  if(GME_GameGetCurId() == -1) {
    /* update menus */
    GME_GameUpdMenu();
    return false;
  }

  /* check if mods dir exists in game root */
  if(!GME_IsDir(GME_GameGetCurModsPath())) {
    GME_DialogWarning(g_hwndMain, L"The Mods stock folder '" + GME_GameGetCurModsPath() + L"' does not exists, you should create it.");
  }

  std::wstring conf_path = GME_GameGetCurConfPath();
  std::wstring mods_path = GME_GameGetCurModsPath();
  std::wstring srch_path;

  std::wstring name;
  std::vector<std::wstring> name_list;
  std::vector<int> type_list;

   /* get uninstalled (available) mod list */
  WIN32_FIND_DATAW fdw;
  HANDLE hnd;

  /* first, get installed mods list, i.e. available backup data */
  srch_path = conf_path + L"\\*.bck";
  hnd = FindFirstFileW(srch_path.c_str(), &fdw);
  if(hnd != INVALID_HANDLE_VALUE) {
    do {
      if(!(fdw.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        name = GME_FilePathToName(fdw.cFileName);
        name_list.push_back(name);
        type_list.push_back(2);
      }
    } while(FindNextFileW(hnd, &fdw));
  }
  FindClose(hnd);

  /* then gather mods from mods list folder */
  srch_path = GME_GameGetCurModsPath() + L"\\*";
  hnd = FindFirstFileW(srch_path.c_str(), &fdw);
  if(hnd != INVALID_HANDLE_VALUE) {
    do {
      if(fdw.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        if(fdw.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
          continue;
        if(!wcscmp(fdw.cFileName, L"."))
          continue;
        if(!wcscmp(fdw.cFileName, L".."))
          continue;
        name = fdw.cFileName;
        if(!GME_IsFile(conf_path + L"\\" + name + L".bck")) {
          name_list.push_back(name);
          type_list.push_back(0);
        }
      } else {
        if(GME_ZipIsValidMod(mods_path + L"\\" + fdw.cFileName)) {
          name = GME_FilePathToName(fdw.cFileName);
          if(!GME_IsFile(conf_path + L"\\" + name + L".bck")) {
            name_list.push_back(name);
            type_list.push_back(1);
          }
        }
      }
    } while(FindNextFileW(hnd, &fdw));
  }
  FindClose(hnd);

  /* then search only .zip files */
  /*
  srch_path = GME_GameGetCurModsPath() + L"\\*.zip";
  hnd = FindFirstFileW(srch_path.c_str(), &fdw);
  if(hnd != INVALID_HANDLE_VALUE) {
    do {
      if(!(fdw.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        if(GME_ZipIsValidMod(mods_path + L"\\" + fdw.cFileName)) {
          name = GME_FilePathToName(fdw.cFileName);
          if(!GME_IsFile(conf_path + L"\\" + name + L".bck")) {
            name_list.push_back(name);
            type_list.push_back(1);
          }
        }
      }
    } while(FindNextFileW(hnd, &fdw));
  }
  FindClose(hnd);
  */
  /* finally check only for folders */
  /*
  srch_path = GME_GameGetCurModsPath() + L"\\*";
  hnd = FindFirstFileW(srch_path.c_str(), &fdw);
  if(hnd != INVALID_HANDLE_VALUE) {
    do {
      if(fdw.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        if(fdw.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
          continue;
        if(!wcscmp(fdw.cFileName, L"."))
          continue;
        if(!wcscmp(fdw.cFileName, L".."))
          continue;
        name = fdw.cFileName;
        if(!GME_IsFile(conf_path + L"\\" + name + L".bck")) {
          name_list.push_back(name);
          type_list.push_back(0);
        }
      }
    } while(FindNextFileW(hnd, &fdw));
  }
  FindClose(hnd);
*/
  if(name_list.empty()) {
    /* update menus */
    GME_GameUpdMenu();
    return true;
  }

  g_ModsList_IsEmpty = false;

  std::wstring version;

  /* add item to list view */
  LVITEMW lvitm;
  memset(&lvitm, 0, sizeof(LVITEMW));
  lvitm.cchTextMax = 255;

  for(unsigned i = 0; i < name_list.size(); i++) {
    lvitm.mask=LVIF_TEXT|LVIF_IMAGE;
    lvitm.iItem = i;
    lvitm.iSubItem = 0;
    wcscpy(wbuff, name_list[i].c_str());
    lvitm.pszText = wbuff;
    switch(type_list[i]) {
    case 0:
      lvitm.iImage = 0;
      break;
    case 1:
      lvitm.iImage = 1;
      break;
    case 2:
      lvitm.iImage = 2;
      break;
    }
    lvitm.iItem = SendMessageW(hlv, LVM_INSERTITEMW, 0, (LPARAM)&lvitm);

    lvitm.mask=LVIF_TEXT;
    lvitm.iSubItem = 1;
    if(type_list[i] == 1 || type_list[i] == 2) {
      if(GME_ZipGetModVers(mods_path + L"\\" + name_list[i] + L".zip", &version)) {
        wcscpy(wbuff, version.c_str());
      } else {
        wcscpy(wbuff, L"0.0.0");
      }
    } else {
      wcscpy(wbuff, L"n/a");
    }
    lvitm.pszText = wbuff;
    SendMessageW(hlv, LVM_SETITEMW, 0, (LPARAM)&lvitm);
  }

  /* Restore scroll position from lvmRect */
  SendMessageW(hlv, LVM_SCROLL, 0, -lvmRect.top );

  /* set mods path in display entry */
  wcscpy(wbuff, GME_GameGetCurModsPath().c_str());
  SendMessageW(het_path, WM_SETTEXT, 0, (LPARAM)wbuff);

  /* update menus */
  GME_GameUpdMenu();

  return true;
}

/*
  mods uninstall thread function
*/
DWORD WINAPI GME_ModsUninstall_Th(void* args)
{
  GME_Logs(3, "GME_ModsUninstall_Th", "Uninstall thread Start", GME_StrToMbs(GME_GameGetCurTitle()).c_str());

  std::wstring status = L"restoring backups for config '" + GME_GameGetCurTitle() + L"'...";
  SetDlgItemTextW(g_hwndUninst, TXT_UNINST_GAME, status.c_str());

  /* get .bck file list */
  std::vector<std::wstring> back_list;

  std::wstring srch_path = GME_GameGetCurConfPath() + L"\\*.bck";
  WIN32_FIND_DATAW fdw;
  HANDLE hnd = FindFirstFileW(srch_path.c_str(), &fdw);
  if(hnd != INVALID_HANDLE_VALUE) {
    do {
      if(!(fdw.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        back_list.push_back(GME_FilePathToName(fdw.cFileName));
      }
    } while(FindNextFileW(hnd, &fdw));
  }
  FindClose(hnd);

  /* restore all backups for this game */
  for(unsigned m = 0; m < back_list.size(); m++) {
    status = L"restore backup of '" + back_list[m] + L"'...";
    SetDlgItemTextW(g_hwndUninst, TXT_UNINST_MOD, status.c_str());
    GME_ModsRestoreMod(GetDlgItem(g_hwndUninst, PBM_UNINST), back_list[m]);
  }

  /* do not miss to clean backup files */
  GME_ModsCleanBackup();

  EndDialog(g_hwndUninst, 0);

  GME_Logs(3, "GME_ModsUninstall_Th", "Uninstall thread Done", GME_StrToMbs(GME_GameGetCurTitle()).c_str());

  return 0;
}

/*
  message callback uninstall mods progress bar
*/
BOOL CALLBACK GME_DlgUninst(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
  case WM_INITDIALOG:
    g_hwndUninst = hwndDlg;
    SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)g_hicnMain);
    g_ModsProc_hT = CreateThread(NULL,0,GME_ModsUninstall_Th,NULL,0,&g_ModsProc_iT);
    return true;
  }
  return false;
}

/*
  function to uninstall all mods for the current game
*/
void GME_ModsUninstall()
{
  DialogBox(g_hInst, MAKEINTRESOURCE(DLG_UNINSTALL), g_hwndMain, (DLGPROC)GME_DlgUninst);
}


/*
  mod creation thread function
*/
DWORD WINAPI GME_ModsMake_Th(void* args)
{
  g_ModsMake_Running = true;

  GME_ModsMake_Arg_Struct* arg = static_cast<GME_ModsMake_Arg_Struct*>(args);

  HWND hpb = GetDlgItem(g_hwndNewAMod, PBM_MAKE);

  EnableWindow(GetDlgItem(g_hwndNewAMod, ENT_SRC), false);
  EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_BROWSESRC), false);
  EnableWindow(GetDlgItem(g_hwndNewAMod, ENT_DST), false);
  EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_BROWSEDST), false);
  EnableWindow(GetDlgItem(g_hwndNewAMod, ENT_VERSMAJOR), false);
  EnableWindow(GetDlgItem(g_hwndNewAMod, ENT_VERSMINOR), false);
  EnableWindow(GetDlgItem(g_hwndNewAMod, ENT_VERSREVIS), false);
  EnableWindow(GetDlgItem(g_hwndNewAMod, ENT_MODDESC), false);
  ShowWindow(GetDlgItem(g_hwndNewAMod, BTN_CREATE), false);
  ShowWindow(GetDlgItem(g_hwndNewAMod, IDCANCEL), true);
  EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_CLOSE), false);


  GMEnode* zip_root = arg->zip_root;
  std::wstring zip_path = arg->zip_path;
  std::wstring tmp_path = zip_path + L".build";

  /* simply get count of nodes */
  unsigned c = 0;
  zip_root->initTraversal();
  while(zip_root->nextChild()) c++;

  /* init our progress bar */
  SendMessage(hpb, PBM_SETRANGE, 0, MAKELPARAM(0, c));
  SendMessage(hpb, PBM_SETSTEP, (WPARAM)1, 0);
  SendMessage(hpb, PBM_SETPOS, (WPARAM)0, 0);

  /* create zip file from tree */
  mz_zip_archive za; // Zip archive struct

  memset(&za, 0, sizeof(mz_zip_archive));
  if(!mz_zip_writer_init_file(&za, GME_StrToMbs(tmp_path).c_str(), 4096)) {
    delete zip_root;
    delete arg;
    GME_Logs(GME_LOG_ERROR, "GME_ModsMake_Th", "mz_zip_writer_init_file failed", GME_StrToMbs(tmp_path).c_str());
    GME_DialogError(g_hwndNewAMod, L"An error occurred during Mod-Archive creation.");
    ShowWindow(GetDlgItem(g_hwndNewAMod, BTN_CREATE), true);
    ShowWindow(GetDlgItem(g_hwndNewAMod, IDCANCEL), false);
    EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_CLOSE), true);
    g_ModsMake_Running = false;
    return 0;
  }

  FILE* fp;
  size_t fs;
  ubyte* data;

  std::string a_name;
  std::string f_name;

  zip_root->initTraversal();
  while(zip_root->nextChild()) {

    GME_StrToMbs(a_name, zip_root->currChild()->getPath());
    a_name.erase(0,1); /* remove the first \ at the begining of the path */
    /* convert from MS standard path separator \ to THE STANDARD */
    std::replace(a_name.begin(), a_name.end(), '\\', '/');

    if(zip_root->currChild()->isDir()) {
      a_name += "/";
      if(!mz_zip_writer_add_mem(&za, a_name.c_str(), NULL, 0, arg->zip_level)) {
        mz_zip_writer_end(&za);
        delete zip_root;
        delete arg;
        GME_FileDelete(tmp_path);
        GME_Logs(GME_LOG_ERROR, "GME_ModsMake_Th", "mz_zip_writer_add_mem (dir) failed", GME_StrToMbs(tmp_path).c_str());
        GME_DialogError(g_hwndNewAMod, L"An error occurred during Mod-Archive creation.");
        ShowWindow(GetDlgItem(g_hwndNewAMod, BTN_CREATE), true);
        ShowWindow(GetDlgItem(g_hwndNewAMod, IDCANCEL), false);
        EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_CLOSE), true);
        g_ModsMake_Running = false;
        return 0;
      }
    } else {
      /* read source file */
      if(!zip_root->currChild()->getSource().empty()) {
        GME_StrToMbs(f_name, zip_root->currChild()->getSource());
        if(!mz_zip_writer_add_file(&za, a_name.c_str(), f_name.c_str(), NULL, 0, arg->zip_level)) {
          mz_zip_writer_end(&za);
          delete zip_root;
          delete arg;
          GME_FileDelete(tmp_path);
          GME_Logs(GME_LOG_ERROR, "GME_ModsMake_Th", "mz_zip_writer_add_file failed", GME_StrToMbs(tmp_path).c_str());
          GME_DialogError(g_hwndNewAMod, L"An error occurred during Mod-Archive creation.");
          ShowWindow(GetDlgItem(g_hwndNewAMod, BTN_CREATE), true);
          ShowWindow(GetDlgItem(g_hwndNewAMod, IDCANCEL), false);
          EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_CLOSE), true);
          g_ModsMake_Running = false;
          return 0;
        }
      } else {
        if(!mz_zip_writer_add_mem(&za, a_name.c_str(), zip_root->currChild()->getData(), zip_root->currChild()->getDataSize(), arg->zip_level)) {
          mz_zip_writer_end(&za);
          delete zip_root;
          delete arg;
          GME_FileDelete(tmp_path);
          GME_Logs(GME_LOG_ERROR, "GME_ModsMake_Th", "mz_zip_writer_add_mem (mem) failed", GME_StrToMbs(tmp_path).c_str());
          GME_DialogError(g_hwndNewAMod, L"An error occurred during Mod-Archive creation.");
          ShowWindow(GetDlgItem(g_hwndNewAMod, BTN_CREATE), true);
          ShowWindow(GetDlgItem(g_hwndNewAMod, IDCANCEL), false);
          EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_CLOSE), true);
          g_ModsMake_Running = false;
          return 0;
        }
      }
    }

    if(g_ModsMake_Cancel) {
      mz_zip_writer_end(&za);
      delete zip_root;
      delete arg;
      GME_FileDelete(tmp_path);
      EnableWindow(GetDlgItem(g_hwndNewAMod, ENT_SRC), true);
      EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_BROWSESRC), true);
      EnableWindow(GetDlgItem(g_hwndNewAMod, ENT_DST), true);
      EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_BROWSEDST), true);
      EnableWindow(GetDlgItem(g_hwndNewAMod, ENT_VERSMAJOR), true);
      EnableWindow(GetDlgItem(g_hwndNewAMod, ENT_VERSMINOR), true);
      EnableWindow(GetDlgItem(g_hwndNewAMod, ENT_VERSREVIS), true);
      EnableWindow(GetDlgItem(g_hwndNewAMod, ENT_MODDESC), true);
      ShowWindow(GetDlgItem(g_hwndNewAMod, BTN_CREATE), true);
      ShowWindow(GetDlgItem(g_hwndNewAMod, IDCANCEL), false);
      EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_CLOSE), true);
      SendMessage(GetDlgItem(g_hwndNewAMod, PBM_MAKE), PBM_SETPOS, (WPARAM)0, 0);
      g_ModsMake_Running = false;
      return 0;
    }

    SendMessage(hpb, PBM_STEPIT, 0, 0);
  }

  mz_zip_writer_finalize_archive(&za);

  mz_zip_writer_end(&za);

  delete zip_root;
  delete arg;

  /* Deleting existing archive if it exists */
  if(GME_IsFile(zip_path)) {
    if(!GME_FileDelete(zip_path)) {
      GME_FileDelete(tmp_path);
      GME_Logs(GME_LOG_ERROR, "GME_ModsMake_Th", "Unable to delete file", GME_StrToMbs(zip_path).c_str());
      GME_DialogError(g_hwndNewAMod, L"An error occurred during Mod-Archive creation.");
      ShowWindow(GetDlgItem(g_hwndNewAMod, BTN_CREATE), true);
      ShowWindow(GetDlgItem(g_hwndNewAMod, IDCANCEL), false);
      EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_CLOSE), true);
      g_ModsMake_Running = false;
      return 0;
    }
  }

  /* rename temporary file to final name */
  if(!GME_FileMove(tmp_path, zip_path)) {
    GME_FileDelete(tmp_path);
    GME_Logs(GME_LOG_ERROR, "GME_ModsMake_Th", "Unable to rename file", GME_StrToMbs(zip_path).c_str());
    GME_DialogError(g_hwndNewAMod, L"An error occurred during Mod-Archive creation.");
    ShowWindow(GetDlgItem(g_hwndNewAMod, BTN_CREATE), true);
    ShowWindow(GetDlgItem(g_hwndNewAMod, IDCANCEL), false);
    EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_CLOSE), true);
    g_ModsMake_Running = false;
    return 0;
  }

  SendMessage(hpb, PBM_SETPOS, (WPARAM)0, 0);

  GME_DialogInfo(g_hwndNewAMod, L"The Mod-Archive '" + zip_path + L".zip' was successfully created.");

  /* just update the mod list now... */
  GME_ModsUpdList();

  if(arg->end_exit) {
    EndDialog(g_hwndNewAMod, 0);
  }

  EnableWindow(GetDlgItem(g_hwndNewAMod, ENT_SRC), true);
  SetDlgItemText(g_hwndNewAMod, ENT_SRC, "");
  EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_BROWSESRC), true);
  EnableWindow(GetDlgItem(g_hwndNewAMod, ENT_DST), true);
  EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_BROWSEDST), true);
  SetDlgItemText(g_hwndNewAMod, ENT_VERSMAJOR, "0");
  EnableWindow(GetDlgItem(g_hwndNewAMod, ENT_VERSMAJOR), true);
  SetDlgItemText(g_hwndNewAMod, ENT_VERSMINOR, "0");
  EnableWindow(GetDlgItem(g_hwndNewAMod, ENT_VERSMINOR), true);
  SetDlgItemText(g_hwndNewAMod, ENT_VERSREVIS, "0");
  EnableWindow(GetDlgItem(g_hwndNewAMod, ENT_VERSREVIS), true);
  SetDlgItemText(g_hwndNewAMod, ENT_MODDESC, "");
  EnableWindow(GetDlgItem(g_hwndNewAMod, ENT_MODDESC), true);
  // disable Add button
  ShowWindow(GetDlgItem(g_hwndNewAMod, BTN_CREATE), true);
  EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_CLOSE), false);
  ShowWindow(GetDlgItem(g_hwndNewAMod, IDCANCEL), false);
  EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_CLOSE), true);
  SendMessage(GetDlgItem(g_hwndNewAMod, PBM_MAKE), PBM_SETPOS, (WPARAM)0, 0);

  g_ModsMake_Running = false;

  return 0;
}

/*
  function to cancel curren make process
*/
void GME_ModsMakeCancel()
{
  if(g_ModsMake_Running) {
    g_ModsMake_Cancel = true;
    EnableWindow(GetDlgItem(g_hwndNewAMod, IDCANCEL), false);
  }
}

/*
  function to create a new mod archive
*/
void GME_ModsMakeArchive(const std::wstring& src_dir, const std::wstring& dst_path, const std::wstring& desc, int vmaj, int vmin, int vrev, int zlevel)
{
  if(g_ModsMake_Running) {
    GME_DialogWarning(g_hwndNewAMod, L"Make Mod-Archive process already running, please wait.");
    return;
  }

  if(!GME_IsDir(dst_path)) {
    GME_DialogWarning(g_hwndNewAMod, L"Invalid destination path.");
    return;
  }

  if(!GME_IsDir(src_dir)) {
    GME_DialogWarning(g_hwndNewAMod, L"Invalid source Directory-Mod path.");
    return;
  }

  std::wstring mod_name = GME_DirPathToName(src_dir);
  std::wstring zip_name = mod_name + L".zip";
  std::wstring txt_name = L"README.txt";
  std::wstring ver_name = L"VERSION.txt";
  std::wstring zip_path = dst_path + L"\\" + zip_name;
  std::string txt_data;
  if(!desc.empty()) txt_data = GME_StrToMbs(desc);
  std::string ver_data;
  char vbuff[64];
  sprintf(vbuff, "%d.%d.%d", vmaj, vmin, vrev);
  ver_data = vbuff;


  if(GME_IsFile(zip_path)) {
    if(IDCANCEL == GME_DialogQuestionConfirm(g_hwndNewAMod, L"The destination file '" + zip_path + L"' already exists, do you want to overwrite ?"))
      return;
  }

  /* node tree for data structure */
  GMEnode* zip_root = new GMEnode();

  /* mod folder */
  GMEnode* mod_tree = new GMEnode();
  GME_TreeBuildFromDir(mod_tree, src_dir);
  mod_tree->setName(mod_name);
  mod_tree->setParent(zip_root);

  /* add the description txt node */
  if(!txt_data.empty()) {
    GMEnode* txt_node = new GMEnode(txt_name, false);
    txt_node->setData(txt_data.c_str(), txt_data.size());
    txt_node->setParent(zip_root);
  }

  /* add the version txt node */
  if(!ver_data.empty()) {
    GMEnode* ver_node = new GMEnode(ver_name, false);
    ver_node->setData(ver_data.c_str(), ver_data.size());
    ver_node->setParent(zip_root);
  }

  GME_ModsMake_Arg_Struct* th_args = new GME_ModsMake_Arg_Struct;
  memset(th_args, 0, sizeof(GME_ModsMake_Arg_Struct));
  th_args->zip_root = zip_root;
  wcscpy(th_args->zip_path, zip_path.c_str());
  th_args->zip_level = zlevel;
  th_args->end_exit = false;

  g_ModsMake_Cancel = false;
  g_ModsMake_hT = CreateThread(NULL,0,GME_ModsMake_Th,th_args,0,&g_ModsMake_iT);

  return;
}


/*
  function to Make Mod-Archive from current selected
*/
void GME_ModsMakeArchiveCur(const std::wstring& desc, int vmaj, int vmin, int vrev, int zlevel)
{
  HWND hlv = GetDlgItem(g_hwndMain, LVM_MODSLIST);

  /* get current mod list selection */
  std::wstring name;
  int type;
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
      name = lvitm.pszText;
      type = lvitm.iImage;
      found = true;
      break; // single selection
    }
  }

  if(!found) {
    GME_DialogWarning(g_hwndMain, L"No Mod selected.");
    return;
  }

  std::wstring src_dir = GME_GameGetCurModsPath() + L"\\" + name;
  std::wstring dst_path = GME_GameGetCurModsPath();

  if(!GME_IsDir(src_dir)) {
    GME_DialogWarning(g_hwndMain, L"Mod '" + name + L"' is not a Directory-Mod.");
    return;
  }

  std::wstring mod_name = GME_DirPathToName(src_dir);
  std::wstring zip_name = mod_name + L".zip";
  std::wstring txt_name = L"README.txt";
  std::wstring ver_name = L"VERSION.txt";
  std::wstring zip_path = dst_path + L"\\" + zip_name;
  std::string txt_data;
  if(!desc.empty()) txt_data = GME_StrToMbs(desc);
  std::string ver_data;
  char vbuff[64];
  sprintf(vbuff, "%d.%d.%d", vmaj, vmin, vrev);
  ver_data = vbuff;


  if(GME_IsFile(zip_path)) {
    if(IDCANCEL == GME_DialogQuestionConfirm(g_hwndNewAMod, L"The destination file '" + zip_path + L"' already exists, do you want to overwrite ?"))
      return;
  }

  /* node tree for data structure */
  GMEnode* zip_root = new GMEnode();

  /* mod folder */
  GMEnode* mod_tree = new GMEnode();
  GME_TreeBuildFromDir(mod_tree, src_dir);
  mod_tree->setName(mod_name);
  mod_tree->setParent(zip_root);

  /* add the description txt node */
  if(!txt_data.empty()) {
    GMEnode* txt_node = new GMEnode(txt_name, false);
    txt_node->setData(txt_data.c_str(), txt_data.size());
    txt_node->setParent(zip_root);
  }

  /* add the version txt node */
  if(!ver_data.empty()) {
    GMEnode* ver_node = new GMEnode(ver_name, false);
    ver_node->setData(ver_data.c_str(), ver_data.size());
    ver_node->setParent(zip_root);
  }

  GME_ModsMake_Arg_Struct* th_args = new GME_ModsMake_Arg_Struct;
  memset(th_args, 0, sizeof(GME_ModsMake_Arg_Struct));
  th_args->zip_root = zip_root;
  wcscpy(th_args->zip_path, zip_path.c_str());
  th_args->zip_level = zlevel;
  th_args->end_exit = true;

  g_ModsMake_Cancel = false;
  g_ModsMake_hT = CreateThread(NULL,0,GME_ModsMake_Th,th_args,0,&g_ModsMake_iT);

  return;
}


/*
  add a mod apply to stack for the threaded mod process
*/
void GME_ModsProc_PushApply(const std::wstring& name, int type)
{
  /* build arg list */
  GME_ModsProc_Arg_Struct args;
  args.action = MODS_ENABLE;
  args.name = name;
  args.type = type;
  g_ModsProc_ArgList.push_back(args);
}

/*
  add a mod restore to stack for the threaded mod process
*/
void GME_ModsProc_PushRestore(const std::wstring& name)
{
  /* build arg list */
  GME_ModsProc_Arg_Struct args;
  args.action = MODS_DISABLE;
  args.name = name;
  g_ModsProc_ArgList.push_back(args);
}

/*
  check if mod process is ended or ready
*/
bool GME_ModsProc_IsReady()
{
  return !g_ModsProc_Running;
}

/*
  threaded function to apply and restore mods
*/
DWORD WINAPI GME_ModsProc_Th(void* args)
{
  g_ModsProc_Running = true;

  /* disable all buttons and combo to prevent dirty actions during process */
  EnableWindow(GetDlgItem(g_hwndMain, CMB_GAMELIST), false);
  EnableWindow(GetDlgItem(g_hwndMain, BTN_IMPORTMOD), false);
  EnableWindow(GetDlgItem(g_hwndMain, BTN_ADDGAME), false);
  EnableWindow(GetDlgItem(g_hwndMain, BTN_MODENA), false);
  EnableWindow(GetDlgItem(g_hwndMain, BTN_MODDIS), false);
  EnableMenuItem(g_hmnuMain, MNU_GAMEADD, MF_GRAYED);
  EnableMenuItem(g_hmnuMain, MNU_GAMEEDIT, MF_GRAYED);
  EnableMenuItem(g_hmnuMain, MNU_GAMEREM, MF_GRAYED);
  EnableMenuItem(g_hmnuMain, MNU_SNAPCREATE, MF_GRAYED);
  EnableMenuItem(g_hmnuMain, MNU_SNAPCOMPARE, MF_GRAYED);
  EnableMenuItem(g_hmnuMain, MNU_MODENAALL, MF_GRAYED);
  EnableMenuItem(g_hmnuMain, MNU_MODENA, MF_GRAYED);
  EnableMenuItem(g_hmnuMain, MNU_MODDISALL, MF_GRAYED);
  EnableMenuItem(g_hmnuMain, MNU_MODDIS, MF_GRAYED);
  EnableMenuItem(g_hmnuMain, MNU_PROFILSAVE, MF_GRAYED);
  //EnableMenuItem(g_hmnuMain, MNU_PROFILELOAD, MF_GRAYED);
  GME_ProfEnaMenu(false);
  EnableMenuItem(g_hmnuMain, MNU_REPOSCONFIG, MF_GRAYED);
  EnableMenuItem(g_hmnuMain, MNU_REPOSQUERY, MF_GRAYED);
  EnableMenuItem(g_hmnuMain, MNU_REPOSMKXML, MF_GRAYED);
  EnableMenuItem(g_hmnuMain, MNU_MODIMPORT, MF_GRAYED);
  EnableMenuItem(g_hmnuMain, MNU_MODCREATE, MF_GRAYED);
  /* enable cancel */
  EnableWindow(GetDlgItem(g_hwndMain, BTN_MODCANCEL), true);

  /* to check if backup cleaning is needed */
  bool bck_restored = false;

  for(unsigned i = 0; i < g_ModsProc_ArgList.size(); i++) {
    if(g_ModsProc_Cancel) {
      break;
    }
    if(g_ModsProc_ArgList[i].action == MODS_ENABLE) {
      GME_ModsApplyMod(GetDlgItem(g_hwndMain, PBM_MODPROC), g_ModsProc_ArgList[i].name, g_ModsProc_ArgList[i].type);
      GME_ModsListQuickEnable(g_ModsProc_ArgList[i].name, true);
    }
    if(g_ModsProc_ArgList[i].action == MODS_DISABLE) {
      bck_restored = true;
      GME_ModsRestoreMod(GetDlgItem(g_hwndMain, PBM_MODPROC), g_ModsProc_ArgList[i].name);
      GME_ModsListQuickEnable(g_ModsProc_ArgList[i].name, false);
    }
  }
  g_ModsProc_ArgList.clear();

  /* do not miss to clean backup files if some backup was restored */
  if(bck_restored)
    GME_ModsCleanBackup();

  GME_ModsUpdList();

  /* restore buttons */
  EnableWindow(GetDlgItem(g_hwndMain, CMB_GAMELIST), true);
  EnableWindow(GetDlgItem(g_hwndMain, BTN_IMPORTMOD), true);
  EnableWindow(GetDlgItem(g_hwndMain, BTN_ADDGAME), true);
  EnableWindow(GetDlgItem(g_hwndMain, BTN_MODENA), true);
  EnableWindow(GetDlgItem(g_hwndMain, BTN_MODDIS), true);
  EnableMenuItem(g_hmnuMain, MNU_GAMEADD, MF_BYCOMMAND);
  EnableMenuItem(g_hmnuMain, MNU_GAMEEDIT, MF_BYCOMMAND);
  EnableMenuItem(g_hmnuMain, MNU_GAMEREM, MF_BYCOMMAND);
  EnableMenuItem(g_hmnuMain, MNU_SNAPCREATE, MF_BYCOMMAND);
  EnableMenuItem(g_hmnuMain, MNU_SNAPCOMPARE, MF_BYCOMMAND);
  EnableMenuItem(g_hmnuMain, MNU_MODENAALL, MF_BYCOMMAND);
  EnableMenuItem(g_hmnuMain, MNU_MODENA, MF_BYCOMMAND);
  EnableMenuItem(g_hmnuMain, MNU_MODDISALL, MF_BYCOMMAND);
  EnableMenuItem(g_hmnuMain, MNU_MODDIS, MF_BYCOMMAND);
  EnableMenuItem(g_hmnuMain, MNU_PROFILSAVE, MF_BYCOMMAND);
  //EnableMenuItem(g_hmnuMain, MNU_PROFILELOAD, MF_BYCOMMAND);
  GME_ProfEnaMenu(true);
  EnableMenuItem(g_hmnuMain, MNU_REPOSCONFIG, MF_BYCOMMAND);
  EnableMenuItem(g_hmnuMain, MNU_REPOSQUERY, MF_BYCOMMAND);
  EnableMenuItem(g_hmnuMain, MNU_REPOSMKXML, MF_BYCOMMAND);
  EnableMenuItem(g_hmnuMain, MNU_MODIMPORT, MF_BYCOMMAND);
  EnableMenuItem(g_hmnuMain, MNU_MODCREATE, MF_BYCOMMAND);
  /* re-disable what should be... */
  GME_GameUpdMenu();
  EnableWindow(GetDlgItem(g_hwndMain, BTN_MODCANCEL), false);

  g_ModsProc_Running = false;
  return 0;
}

/*
  function to run threaded mod process
*/
void GME_ModsProc_Launch()
{
  if(GME_ModsProc_IsReady()) {
    CloseHandle(g_ModsProc_hT);
    /* and, here we go... */
    g_ModsProc_Cancel = false;
    g_ModsProc_hT = CreateThread(NULL,0,GME_ModsProc_Th,NULL,0,&g_ModsProc_iT);
  } else {
    GME_DialogWarning(g_hwndMain, L"Mod(s) installation is currently processing, please wait until current process finish before enabling or disabling Mod(s).");
  }
}

/*
  cancel current mod processing
*/
void GME_ModsProcCancel()
{
  if(!GME_ModsProc_IsReady()) {
    g_ModsProc_Cancel = true;
    EnableWindow(GetDlgItem(g_hwndMain, BTN_MODCANCEL), false);
  }
}



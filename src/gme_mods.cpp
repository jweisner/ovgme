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


/* mods profile struct */
struct GME_ProfilEntry_Struct
{
  wchar_t name[128];
  ubyte type;
  ubyte stat;
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
  GMEnode* zip_root;
  wchar_t zip_path[260];
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
  function to update backup archive according installed mods dependencies
  (this is the vital backup process)
*/
inline bool GME_ModsUpdBackup(HWND hpb)
{
  std::wstring conf_path = GME_GameGetCurConfPath();
  std::wstring back_path = GME_GameGetCurBackPath();
  std::wstring game_path = GME_GameGetCurRoot();

  /* the backup dependency list */
  std::vector<std::wstring> dpend_list;
  std::vector<std::wstring> backd_list;

  /* temporary to read/write file data */
  FILE* fp;

  /* temporary for backentry dat path */
  std::wstring bck_file;

  /* backup entry */
  GME_BckEntry_Struct bckentry;

  /* temporary unsigned entry count */
  unsigned c;

  /* open all backup entry data files and gather dependencies */
  std::wstring gbck_srch = conf_path + L"\\*.bck";
  WIN32_FIND_DATAW fdw;
  HANDLE hnd = FindFirstFileW(gbck_srch.c_str(), &fdw);
  if(hnd != INVALID_HANDLE_VALUE) {
    do {
      if(!(fdw.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        /* read backup entry dat file */
        bck_file = conf_path + L"\\" + fdw.cFileName;
        fp = _wfopen(bck_file.c_str(), L"rb");
        if(fp) {
          /* first 4 bytes is count of entries */
          fread(&c, 4, 1, fp);
          for(unsigned i = 0; i < c; i++) {
            memset(&bckentry, 0, sizeof(GME_BckEntry_Struct));
            fread(&bckentry, sizeof(GME_BckEntry_Struct), 1, fp);
            if(bckentry.action == GME_BCK_RESTORE_SWAP) {
              if(!bckentry.isdir) {
                dpend_list.push_back(bckentry.path);
              }
            }
          }
          fclose(fp);
        } else {
          GME_Logs(GME_LOG_ERROR, "GME_ModsUpdBackup", "Unable to open backup data", GME_StrToMbs(bck_file).c_str());
        }
      }
    } while(FindNextFileW(hnd, &fdw));
  }
  FindClose(hnd);

  /* temporary for split path */
  std::vector<std::wstring> path_split;

  std::wstring dst_path;
  std::wstring src_path;

  bool got_error = false;

  /* create node tree from backup folder tree */
  GMEnode* back_tree = new GMEnode();
  GME_TreeBuildFromDir(back_tree, back_path);

  /* pass the depend list in upper case to prevent
    case sensitive naming problems */
  for(unsigned i = 0; i < dpend_list.size(); i++) {
    GME_StrToUpper(dpend_list[i]);
  }

  /* remove files no longer needed */
  bool is_depend;
  back_tree->initTraversal();
  while(back_tree->nextChild()) {
    if(!back_tree->currChild()->isDir()) {
      /* upper case to prevent case sensitive problems... */
      dst_path = GME_StrToUpper(back_tree->currChild()->getPath(true));
      is_depend = false;
      for(unsigned i = 0; i < dpend_list.size(); i++) {
        if(dst_path == dpend_list[i]) {
          is_depend = true;
          break;
        }
      }
      if(!is_depend) {
        dst_path = back_path + back_tree->currChild()->getPath(true);
        if(!DeleteFileW(dst_path.c_str())) {
          GME_Logs(GME_LOG_ERROR, "GME_ModsUpdBackup", "Unable to delete file", GME_StrToMbs(dst_path).c_str());
          got_error = true;
        }
        /* step progress bar */
        SendMessage(hpb, PBM_STEPIT, 0, 0);
      }
    } else {
      /* keep folder path list for cleaning later */
      backd_list.push_back(back_tree->currChild()->getPath(true));
    }
  }

  /* clean empty folders */
  unsigned i = backd_list.size();
  while(i--) {
    dst_path = back_path + backd_list[i];
    if(PathIsDirectoryEmptyW(dst_path.c_str())) {
      if(!RemoveDirectoryW(dst_path.c_str())) {
        GME_Logs(GME_LOG_ERROR, "GME_ModsUpdBackup", "Unable to delete directory", GME_StrToMbs(dst_path).c_str());
        got_error = true;
      }
    }
  }

  delete back_tree;

  return !got_error;
}


/*
  function to undo Mod installation, same as Restore, but without error messages
*/
void GME_ModsUndoMod(HWND hpb, const std::vector<GME_BckEntry_Struct>& bckentry_list)
{
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
  SendMessage(hpb, PBM_SETRANGE, 0, MAKELPARAM(0, c*2));
  SendMessage(hpb, PBM_SETSTEP, (WPARAM)1, 0);
  SendMessage(hpb, PBM_SETPOS, (WPARAM)0, 0);

  /* read backup entries in backward so we process data
     in reversed depth-first order: begin by leafs, end by root */
  unsigned i = bckentry_list.size();
  while(i--) {
    dst_path = game_path + bckentry_list[i].path;
    if(bckentry_list[i].action == GME_BCK_RESTORE_DELETE) {
      if(bckentry_list[i].isdir) {
        if(!RemoveDirectoryW(dst_path.c_str())) {
          GME_Logs(GME_LOG_WARNING, "GME_ModsUndoMod", "Unable to delete directory", GME_StrToMbs(dst_path).c_str());
        }
      } else {
        if(!DeleteFileW(dst_path.c_str())) {
          GME_Logs(GME_LOG_WARNING, "GME_ModsUndoMod", "Unable to delete file", GME_StrToMbs(dst_path).c_str());
        }
      }
    } else {
      if(bckentry_list[i].isdir) {
        // this should logically never happen...
      } else {
        src_path = back_path + bckentry_list[i].path;
        /* copy file, overwrite dest */
        if(!GME_FileCopy(src_path, dst_path, true)) {
          GME_Logs(GME_LOG_WARNING, "GME_ModsUndoMod", "Unable to copy file", GME_StrToMbs(dst_path).c_str());
        }
        /* step progress bar */
        SendMessage(hpb, PBM_STEPIT, 0, 0);
      }
    }
  }

  /* update backup archive, this will remove no long needed file from archive */
  if(!GME_ModsUpdBackup(hpb)) {
    GME_Logs(GME_LOG_ERROR, "GME_ModsUndoMod", "GME_ModsUpdBackup failed", "...");
  }

  SendMessage(hpb, PBM_SETPOS, (WPARAM)0, 0);
}

/*
  function to apply a mod: create backup, check overlap, apply changes...
*/
void GME_ModsApplyMod(HWND hpb, const std::wstring& name, int type)
{
  bool is_zip_mod;

  /* check if we have a dir or a zip */
  if(type == 0) {
    is_zip_mod = false;
  } else if(type == 1) {
    is_zip_mod = true;
  } else {
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
      GME_DialogError(g_hwndMain, L"Mod archive extraction failed. Aborting.");
      delete mod_tree;
      SendMessage(hpb, PBM_SETPOS, (WPARAM)0, 0);
      GME_Logs(GME_LOG_ERROR, "GME_ModsApplyMod", "GME_TreeBuildFromZip failed", GME_StrToMbs(name).c_str());
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

  /* search for each .dat file in current game config folder */
  std::wstring bck_file;
  std::wstring bck_srch = conf_path + L"\\*.bck";
  WIN32_FIND_DATAW fdw;
  HANDLE hnd = FindFirstFileW(bck_srch.c_str(), &fdw);
  if(hnd != INVALID_HANDLE_VALUE) {
    do {
      if(!(fdw.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        bck_file = conf_path + L"\\" + fdw.cFileName;
        /* read backup data entry */
        fp = _wfopen(bck_file.c_str(), L"rb");
        if(fp) {
          /* first 4 bytes is count of entries */
          fread(&c, 4, 1, fp);
          for(unsigned i = 0; i < c; i++) {
            memset(&bckentry, 0, sizeof(GME_BckEntry_Struct));
            fread(&bckentry, sizeof(GME_BckEntry_Struct), 1, fp);
            if(bckentry.action == GME_BCK_RESTORE_SWAP) {
              if(!bckentry.isdir) {
                dpnd_flist.push_back(bckentry.path);
              }
            }
          }
          fclose(fp);
        }

        /* cross check between mod files and bck files */
        for(unsigned i = 0; i < imod_flist.size(); i++) {
          for(unsigned j = 0; j < dpnd_flist.size(); j++) {
            if(dpnd_flist[j].find(imod_flist[i]) != std::wstring::npos) {
              olap_flist.push_back(imod_flist[i]);
            }
          }
        }

        /* if we get files overlap */
        if(olap_flist.size() > 0) {

          /* create LE message... */
          std::wstring le_message = L"The '" + name + L"' mod files overlap with the already installed '" + GME_FilePathToName(fdw.cFileName) + L"' mod.";

          le_message += L"\n\nOverlaped file(s):\n";
          for(unsigned i = 0; i < olap_flist.size(); i++)
            le_message += L"  " + olap_flist[i] + L"\n";

          le_message += L"\nThis can cause problems if you disable a mod without the other: The backup process restores the original game files, then corrupt the installed mod files (backed data are not affected)";
          le_message += L"\n\nDo you want to continue anyway ?";

          /* show confirmation dialog */
          if(IDCANCEL == GME_DialogWarningConfirm(g_hwndMain, le_message)) {
            delete mod_tree;
            FindClose(hnd);
            SendMessage(hpb, PBM_SETPOS, (WPARAM)0, 0);
            GME_Logs(GME_LOG_WARNING, "GME_ModsApplyMod", "Mod apply cancelled by user", GME_StrToMbs(name).c_str());
            return;
          }
        }
      }
    } while(FindNextFileW(hnd, &fdw));
  }
  FindClose(hnd);

  /* -------------------------- add backup data -------------------------- */

  /* archive original files and create backup entries */
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
          if(!CreateDirectoryW(dst_path.c_str(), 0)) {
            GME_DialogError(g_hwndMain, L"Error on backup for Mod '" + name + L"'. Aborting.");
            GME_Logs(GME_LOG_ERROR, "GME_ModsApplyMod", "Unable to backup directory", GME_StrToMbs(dst_path).c_str());
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
          GME_DialogError(g_hwndMain, L"Error on backup for Mod '" + name + L"'. Aborting.");
          GME_Logs(GME_LOG_ERROR, "GME_ModsApplyMod", "Unable to backup file", GME_StrToMbs(src_path).c_str());
          GME_ModsUndoMod(hpb, bckentry_list);
          delete mod_tree;
          return;
        }
      } else {
        bckentry.action = GME_BCK_RESTORE_DELETE;
      }
    }
    wcscpy(bckentry.path, rel_path.c_str());
    bckentry_list.push_back(bckentry);
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

  /* -------------------------- apply mod files -------------------------- */

  /* special zip stuff... we deal directly with miniz.c functions to optimizes
    zip unpack (both for disk access and memory usage)... */

  mz_zip_archive za; /* miniz zip archive handle */
  mz_zip_archive_file_stat zf; /* miniz zip archive file struct */
  ubyte* unpack;

  if(is_zip_mod) {
    memset(&za, 0, sizeof(mz_zip_archive));
    if(!mz_zip_reader_init_file(&za, GME_StrToMbs(mods_path + L"\\" + name + L".zip").c_str(), 0)) {
      GME_DialogError(g_hwndMain, L"Mod archive extraction failed for mod '" + name + L"'. Aborting.");
      GME_Logs(GME_LOG_ERROR, "GME_ModsApplyMod", "mz_zip_reader_init_file failed", GME_StrToMbs(name).c_str());
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
        CreateDirectoryW(dst_path.c_str(), NULL);
      }
    } else {
      /* overwrite or create file */
      if(is_zip_mod) {
        /* get zip file status */
        if(!mz_zip_reader_file_stat(&za, mod_tree->currChild()->getId(), &zf)){
          mz_zip_reader_end(&za);
          GME_DialogError(g_hwndMain, L"Mod archive extraction failed for mod '" + name + L"'. Aborting.");
          GME_Logs(GME_LOG_ERROR, "GME_ModsApplyMod", "mz_zip_reader_file_stat failed", GME_StrToMbs(name).c_str());
          delete mod_tree;
          SendMessage(hpb, PBM_SETPOS, (WPARAM)0, 0);
          DeleteFileW(bck_file.c_str());
          GME_ModsUndoMod(hpb, bckentry_list);
          return;
        }

        /* unpack file from zip */
        unpack = new ubyte[zf.m_uncomp_size];
        if(!mz_zip_reader_extract_to_mem(&za, mod_tree->currChild()->getId(), unpack, zf.m_uncomp_size, 0)) {
          mz_zip_reader_end(&za);
          GME_DialogError(g_hwndMain, L"Mod archive extraction failed for mod '" + name + L"'. Aborting.");
          GME_Logs(GME_LOG_ERROR, "GME_ModsApplyMod", "mz_zip_reader_extract_to_mem failed", GME_StrToMbs(name).c_str());
          delete mod_tree;
          delete [] unpack;
          SendMessage(hpb, PBM_SETPOS, (WPARAM)0, 0);
          DeleteFileW(bck_file.c_str());
          GME_ModsUndoMod(hpb, bckentry_list);
          return;
        }

        /* write to destination file */
        if(!GME_FileWrite(unpack, zf.m_uncomp_size, dst_path, true)) {
          mz_zip_reader_end(&za);
          GME_DialogError(g_hwndMain, L"Unable to write files for mod '" + name + L"'. Aborting.");
          GME_Logs(GME_LOG_ERROR, "GME_ModsApplyMod", "Unable to write file", GME_StrToMbs(dst_path).c_str());
          delete mod_tree;
          delete [] unpack;
          SendMessage(hpb, PBM_SETPOS, (WPARAM)0, 0);
          DeleteFileW(bck_file.c_str());
          GME_ModsUndoMod(hpb, bckentry_list);
          return;
        }
        delete [] unpack;

      } else {
        /* copy with overwrite */
        if(!GME_FileCopy(mod_tree->currChild()->getSource(), dst_path, true)) {
          mz_zip_reader_end(&za);
          GME_DialogError(g_hwndMain, L"Unable to copy files for mod '" + name + L"'. Aborting.");
          GME_Logs(GME_LOG_ERROR, "GME_ModsApplyMod", "Unable to copy file", GME_StrToMbs(dst_path).c_str());
          delete mod_tree;
          SendMessage(hpb, PBM_SETPOS, (WPARAM)0, 0);
          DeleteFileW(bck_file.c_str());
          GME_ModsUndoMod(hpb, bckentry_list);
          return;
        }
      }
      /* step progress bar */
      SendMessage(hpb, PBM_STEPIT, 0, 0);
    }
  }

  if(is_zip_mod) {
    mz_zip_reader_end(&za);
  }

  delete mod_tree;

  SendMessage(hpb, PBM_SETPOS, (WPARAM)0, 0);
}




/*
  function to restore initial game data from backup (i.e. disabling a mod)
*/
void GME_ModsRestoreMod(HWND hpb, const std::wstring& name)
{
  std::wstring dst_path;
  std::wstring src_path;
  std::wstring back_path = GME_GameGetCurBackPath();
  std::wstring game_path = GME_GameGetCurRoot();
  std::wstring conf_path = GME_GameGetCurConfPath();

  /* check backup file */
  if(!GME_IsFile(conf_path + L"\\" + name + L".bck")) {
    /* no backup file, no restore */
    return;
  }

  /* temporary to read/write file data */
  FILE* fp;

  /* backup entry list */
  GME_BckEntry_Struct bckentry;
  std::vector<GME_BckEntry_Struct> bckentry_list;

  /* read the backup entry data */
  std::wstring bck_file = conf_path + L"\\" + name + L".bck";
  fp = _wfopen(bck_file.c_str(), L"rb");
  if(fp) {
    /* first 4 bytes is count of entries */
    unsigned c;
    fread(&c, 4, 1, fp);
    for(unsigned i = 0; i < c; i++) {
      memset(&bckentry, 0, sizeof(GME_BckEntry_Struct));
      fread(&bckentry, sizeof(GME_BckEntry_Struct), 1, fp);
      bckentry_list.push_back(bckentry);
    }
    fclose(fp);
  }

  /* a first pass to get processing file count */
  unsigned c = 0;
  for(unsigned i = 0; i < bckentry_list.size(); i++) {
    if(bckentry_list[i].action == GME_BCK_RESTORE_SWAP) {
      if(!bckentry_list[i].isdir) c++;
    }
  }

  /* set progress bar */
  SendMessage(hpb, PBM_SETRANGE, 0, MAKELPARAM(0, c*2));
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
        if(!RemoveDirectoryW(dst_path.c_str())) {
          GME_Logs(GME_LOG_ERROR, "GME_ModsRestoreMod", "Unable to delete directory", GME_StrToMbs(dst_path).c_str());
          got_error = true;
        }
      } else {
        if(!DeleteFileW(dst_path.c_str())) {
          GME_Logs(GME_LOG_ERROR, "GME_ModsRestoreMod", "Unable to delete file", GME_StrToMbs(dst_path).c_str());
          got_error = true;
        }
      }
    } else {
      if(bckentry_list[i].isdir) {
        // this should logically never happen...
      } else {
        src_path = back_path + bckentry_list[i].path;
        /* copy file, overwrite dest */
        if(!GME_FileCopy(src_path, dst_path, true)) {
          GME_Logs(GME_LOG_ERROR, "GME_ModsRestoreMod", "Unable to copy file", GME_StrToMbs(dst_path).c_str());
          got_error = true;
        }
        /* step progress bar */
        SendMessage(hpb, PBM_STEPIT, 0, 0);
      }
    }
  }

  DeleteFileW(bck_file.c_str()); /* delete .bck file */

  /* update backup archive, this will remove no long needed file from archive */
  if(!GME_ModsUpdBackup(hpb)) {
    GME_Logs(GME_LOG_ERROR, "GME_ModsRestoreMod", "GME_ModsUpdBackup failed", GME_StrToMbs(name).c_str());
    got_error = true;
  }

  SendMessage(hpb, PBM_SETPOS, (WPARAM)0, 0);

  if(got_error) {
    GME_DialogWarning(g_hwndMain, L"Error encountered while restoring backup for mod '" + name + L"'.");
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
    GME_DialogWarning(g_hwndMain, L"Mod(s) installation is currently processing, please wait until current process finish before enabling or disabling mod(s).");
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
      GME_DialogWarning(g_hwndMain, L"The file '" + fmod_list[i] + L"' is not a valid mod archive and will not be imported.");
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

    std::wstring content = L"No description available.";

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

    SendMessageW(het, WM_SETTEXT, 0, (LPARAM)content.c_str());

  } else {
    SendMessageW(het, WM_SETTEXT, 0, (LPARAM)L"[Multiple selection]");
  }

  return true;
}


/*
  function to display mod description when single selection is made
*/
void GME_ModsExploreSrc()
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
    }
  }

  /* if it is a single selection */
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
  function to save mods profile
*/
void GME_ModsProfileSave()
{
  std::wstring prfl_file = GME_GameGetCurConfPath() + L"\\profile.dat";

  if(GME_IsFile(prfl_file)) {
    if(IDCANCEL == GME_DialogWarningConfirm(g_hwndMain, L"Mods Profile already exists for this game, do you want to overwrite it ?"))
      return;
  }

  HWND hlv = GetDlgItem(g_hwndMain, LVM_MODSLIST);

  /* save current game mod list status */
  GME_ProfilEntry_Struct profilentry;
  std::vector<GME_ProfilEntry_Struct> profilentry_list;
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

    memset(&profilentry, 0, sizeof(profilentry));
    wcscpy(profilentry.name, lvitm.pszText);
    profilentry.type = lvitm.iImage;
    profilentry_list.push_back(profilentry);
  }

  /* for each enabled (type 2) mod, we retrieve the true type */
  std::wstring mod_path;
  for(unsigned i = 0; i < profilentry_list.size(); i++) {
    if(profilentry_list[i].type == 2) {
      profilentry_list[i].stat = 1;
      mod_path = GME_GameGetCurModsPath() + L"\\" + profilentry_list[i].name;
      if(GME_IsDir(mod_path)) {
        profilentry_list[i].type = 0;
      }
      if(GME_ZipIsValidMod(mod_path + L".zip")) {
        profilentry_list[i].type = 1;
      }
    } else {
      profilentry_list[i].stat = 0;
    }
  }

  /* we now can write the profile */
  FILE* fp = _wfopen(prfl_file.c_str(), L"wb");
  if(fp) {
    /* first 4 bytes is count of entries */
    unsigned c = profilentry_list.size();
    fwrite(&c, 4, 1, fp);
    for(unsigned i = 0; i < profilentry_list.size(); i++) {
      fwrite(&profilentry_list[i], sizeof(GME_ProfilEntry_Struct), 1, fp);
    }
    fclose(fp);
  }

  /* update menus */
  GME_GameUpdMenu();

  GME_DialogInfo(g_hwndMain, L"Game Mods list configuration saved as default Mods Profile.");
}

/*
  function to apply mods profile
*/
bool GME_ModsProfileApply()
{
  if(!GME_ModsProc_IsReady()) {
    GME_DialogWarning(g_hwndMain, L"Mod(s) installation is currently processing, please wait until current process finish before enabling or disabling mod(s).");
    return false;
  }

  std::wstring prfl_file = GME_GameGetCurConfPath() + L"\\profile.dat";

  if(!GME_IsFile(prfl_file)) {
    GME_DialogWarning(g_hwndMain, L"No profile available.");
    return false;
  }

  /* load profile data file */
  GME_ProfilEntry_Struct profilentry;
  std::vector<GME_ProfilEntry_Struct> profilentry_list;

  /* read the profile data in config dir */
  FILE* fp = _wfopen(prfl_file.c_str(), L"rb");
  if(fp) {
    /* first 4 bytes is count of entries */
    unsigned c;
    fread(&c, 4, 1, fp);
    for(unsigned i = 0; i < c; i++) {
      fread(&profilentry, sizeof(GME_ProfilEntry_Struct), 1, fp);
      profilentry_list.push_back(profilentry);
    }
    fclose(fp);
  }

  /* apply profile */
  std::vector<std::wstring> missing_list;
  std::wstring mod_path;
  for(unsigned i = 0; i < profilentry_list.size(); i++) {
    /* check if mod exists */
    mod_path = GME_GameGetCurModsPath() + L"\\" + profilentry_list[i].name;
    if(profilentry_list[i].type == 0) {
      if(!GME_IsDir(mod_path)) {
        missing_list.push_back(profilentry_list[i].name);
        continue;
      }
    }
    if(profilentry_list[i].type == 1) {
      if(!GME_ZipIsValidMod(mod_path + L".zip")) {
        missing_list.push_back(profilentry_list[i].name);
        continue;
      }
    }
    if(profilentry_list[i].stat) {
      GME_ModsProc_PushApply(profilentry_list[i].name, profilentry_list[i].type);
    } else {
      GME_ModsProc_PushRestore(profilentry_list[i].name);
    }
  }

  GME_ModsProc_Launch();

  if(missing_list.size()) {
    std::wstring message = L"One or more mod registered in profile was not found:\n\n";
    for(unsigned i = 0; i < missing_list.size(); i++) {
      message += L"  " + missing_list[i] + L"\n";
    }
    GME_DialogWarning(g_hwndMain, message);
  }

  return true;
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
    GME_DialogWarning(g_hwndMain, L"The mods stock folder '" + GME_GameGetCurModsPath() + L"' does not exists, you should create it.");
  }

  std::wstring conf_path = GME_GameGetCurConfPath();
  std::wstring mods_path = GME_GameGetCurModsPath();
  std::wstring srch_path;

  std::wstring name;
  std::vector<std::wstring> name_list;
  std::vector<int> type_list;

   /* get uninstalled (available) mod list */
  srch_path = GME_GameGetCurModsPath() + L"\\*";

  WIN32_FIND_DATAW fdw;
  HANDLE hnd = FindFirstFileW(srch_path.c_str(), &fdw);
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

  /* get installed mods list, i.e. available backup data */
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

  if(name_list.size() == 0) {
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
    if(GME_ZipIsValidMod(mods_path + L"\\" + name_list[i] + L".zip")) {
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
  std::wstring status = L"restoring backups for game '" + GME_GameGetCurTitle() + L"'...";
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

  EndDialog(g_hwndUninst, 0);

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
  function to reset dialog after mod make
*/
void GME_ModsMakeRstDialog()
{
  EnableWindow(GetDlgItem(g_hwndNewAMod, ENT_SRC), true);
  SetDlgItemText(g_hwndNewAMod, ENT_SRC, "");
  EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_BROWSESRC), true);
  SetDlgItemText(g_hwndNewAMod, ENT_DST, "");
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
  EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_CREATE), false);
  EnableWindow(GetDlgItem(g_hwndNewAMod, IDCANCEL), false);
  EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_CLOSE), true);
  SendMessage(GetDlgItem(g_hwndNewAMod, PBM_MAKE), PBM_SETPOS, (WPARAM)0, 0);
}

/*
  mod creation thread function
*/
DWORD WINAPI GME_ModsMake_Th(void* args)
{
  g_ModsMake_Running = true;

  HWND hpb = GetDlgItem(g_hwndNewAMod, PBM_MAKE);

  EnableWindow(GetDlgItem(g_hwndNewAMod, ENT_SRC), false);
  EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_BROWSESRC), false);
  EnableWindow(GetDlgItem(g_hwndNewAMod, ENT_DST), false);
  EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_BROWSEDST), false);
  EnableWindow(GetDlgItem(g_hwndNewAMod, ENT_VERSMAJOR), false);
  EnableWindow(GetDlgItem(g_hwndNewAMod, ENT_VERSMINOR), false);
  EnableWindow(GetDlgItem(g_hwndNewAMod, ENT_VERSREVIS), false);
  EnableWindow(GetDlgItem(g_hwndNewAMod, ENT_MODDESC), false);
  EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_CREATE), false);
  EnableWindow(GetDlgItem(g_hwndNewAMod, IDCANCEL), true);
  EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_CLOSE), false);

  GME_ModsMake_Arg_Struct* arg = static_cast<GME_ModsMake_Arg_Struct*>(args);

  GMEnode* zip_root = arg->zip_root;
  std::wstring zip_path = arg->zip_path;

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
  if(!mz_zip_writer_init_file(&za, GME_StrToMbs(zip_path).c_str(), 0)) {
    delete zip_root;
    delete arg;
    GME_DialogError(g_hwndNewAMod, L"An error occurred during Mod-Archive creation.");
    EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_CREATE), true);
    EnableWindow(GetDlgItem(g_hwndNewAMod, IDCANCEL), false);
    EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_CLOSE), true);
    g_ModsMake_Running = false;
    GME_Logs(GME_LOG_ERROR, "GME_ModsMake_Th", "mz_zip_writer_init_file failed", GME_StrToMbs(zip_path).c_str());
    return 0;
  }

  FILE* fp;
  size_t fs;
  ubyte* data;

  std::string a_name;

  zip_root->initTraversal();
  while(zip_root->nextChild()) {

    a_name = GME_StrToMbs(zip_root->currChild()->getPath());
    a_name.erase(0,1); /* remove the first \ at the begining of the path */
    /* convert from MS standard path separator \ to THE STANDARD */
    std::replace( a_name.begin(), a_name.end(), '\\', '/');

    if(zip_root->currChild()->isDir()) {
      a_name += "/";
      if(!mz_zip_writer_add_mem(&za, a_name.c_str(), NULL, 0, MZ_BEST_COMPRESSION)) {
        mz_zip_writer_end(&za);
        delete zip_root;
        delete arg;
        GME_DialogError(g_hwndNewAMod, L"An error occurred during Mod-Archive creation.");
        EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_CREATE), true);
        EnableWindow(GetDlgItem(g_hwndNewAMod, IDCANCEL), false);
        EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_CLOSE), true);
        g_ModsMake_Running = false;
        GME_Logs(GME_LOG_ERROR, "GME_ModsMake_Th", "mz_zip_writer_add_mem (dir) failed", GME_StrToMbs(zip_path).c_str());
        return 0;
      }
    } else {
      /* read source file */
      if(zip_root->currChild()->getSource().size()) {
        fp = _wfopen(zip_root->currChild()->getSource().c_str(), L"rb");
        if(fp) {
          fseek(fp, 0, SEEK_END);
          fs = ftell(fp);
          fseek(fp, 0, SEEK_SET);

          try {
            data = new ubyte[fs];
          } catch (const std::bad_alloc&) {
            mz_zip_writer_end(&za);
            delete zip_root;
            delete arg;
            GME_DialogError(g_hwndNewAMod, L"An error occurred during Mod-Archive creation.");
            EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_CREATE), true);
            EnableWindow(GetDlgItem(g_hwndNewAMod, IDCANCEL), false);
            EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_CLOSE), true);
            g_ModsMake_Running = false;
            GME_Logs(GME_LOG_ERROR, "GME_ModsMake_Th", "bad alloc", GME_StrToMbs(zip_root->currChild()->getSource().c_str()).c_str());
            return 0;
          }
          if(data) {
            fread(data, fs, 1, fp);
            if(!mz_zip_writer_add_mem(&za, a_name.c_str(), data, fs, MZ_BEST_COMPRESSION)) {
              mz_zip_writer_end(&za);
              delete [] data;
              delete zip_root;
              delete arg;
              GME_DialogError(g_hwndNewAMod, L"An error occurred during Mod-Archive creation.");
              EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_CREATE), true);
              EnableWindow(GetDlgItem(g_hwndNewAMod, IDCANCEL), false);
              EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_CLOSE), true);
              g_ModsMake_Running = false;
              GME_Logs(GME_LOG_ERROR, "GME_ModsMake_Th", "mz_zip_writer_add_mem (file) failed", GME_StrToMbs(zip_path).c_str());
              return 0;
            }
            delete [] data;
          }
        } else {
          GME_Logs(GME_LOG_WARNING, "GME_ModsMake_Th", "Unable to open file", GME_StrToMbs(zip_root->currChild()->getSource().c_str()).c_str());
        }
      } else {
        if(!mz_zip_writer_add_mem(&za, a_name.c_str(), zip_root->currChild()->getData(), zip_root->currChild()->getDataSize(), MZ_BEST_COMPRESSION)) {
          mz_zip_writer_end(&za);
          delete zip_root;
          delete arg;
          GME_DialogError(g_hwndNewAMod, L"An error occurred during Mod-Archive creation.");
          EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_CREATE), true);
          EnableWindow(GetDlgItem(g_hwndNewAMod, IDCANCEL), false);
          EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_CLOSE), true);
          g_ModsMake_Running = false;
          GME_Logs(GME_LOG_ERROR, "GME_ModsMake_Th", "mz_zip_writer_add_mem (mem) failed", GME_StrToMbs(zip_path).c_str());
          return 0;
        }
      }
    }

    if(g_ModsMake_Cancel) {
      mz_zip_writer_end(&za);
      delete zip_root;
      delete arg;
      DeleteFileW(zip_path.c_str());
      g_ModsMake_Running = false;
      return 0;
    }

    SendMessage(hpb, PBM_STEPIT, 0, 0);
  }
  mz_zip_writer_finalize_archive(&za);
  mz_zip_writer_end(&za);

  delete zip_root;
  delete arg;

  SendMessage(hpb, PBM_SETPOS, (WPARAM)0, 0);

  GME_ModsMakeRstDialog();

  GME_DialogInfo(g_hwndNewAMod, L"The Mod-Archive '" + zip_path + L".zip' was successfully created.");

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
    EnableWindow(GetDlgItem(g_hwndNewAMod, ENT_SRC), true);
    EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_BROWSESRC), true);
    EnableWindow(GetDlgItem(g_hwndNewAMod, ENT_DST), true);
    EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_BROWSEDST), true);
    EnableWindow(GetDlgItem(g_hwndNewAMod, ENT_VERSMAJOR), true);
    EnableWindow(GetDlgItem(g_hwndNewAMod, ENT_VERSMINOR), true);
    EnableWindow(GetDlgItem(g_hwndNewAMod, ENT_VERSREVIS), true);
    EnableWindow(GetDlgItem(g_hwndNewAMod, ENT_MODDESC), true);
    EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_CREATE), true);
    EnableWindow(GetDlgItem(g_hwndNewAMod, IDCANCEL), false);
    EnableWindow(GetDlgItem(g_hwndNewAMod, BTN_CLOSE), true);
    SendMessage(GetDlgItem(g_hwndNewAMod, PBM_MAKE), PBM_SETPOS, (WPARAM)0, 0);
  }
}

/*
  function to create a new mod archive
*/
void GME_ModsMakeArchive(const std::wstring& src_dir, const std::wstring& dst_path, const std::wstring& desc, int vmaj, int vmin, int vrev)
{
  if(!GME_IsDir(dst_path)) {
    GME_DialogQuestionConfirm(g_hwndNewAMod, L"Invalid destination path.");
    return;
  }

  if(!GME_IsDir(src_dir)) {
    GME_DialogQuestionConfirm(g_hwndNewAMod, L"Invalid source Directory-Mod path.");
    return;
  }

  std::wstring mod_name = GME_DirPathToName(src_dir);
  std::wstring zip_name = mod_name + L".zip";
  std::wstring txt_name = L"README.txt";
  std::wstring ver_name = L"VERSION.txt";
  std::wstring zip_path = dst_path + L"\\" + zip_name;
  std::string txt_data;
  if(desc.size()) txt_data = GME_StrToMbs(desc);
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
  mod_tree->setParent(zip_root);
  GME_TreeBuildFromDir(mod_tree, src_dir);
  mod_tree->setName(mod_name);

  /* add the description txt node */
  if(txt_data.size()) {
    GMEnode* txt_node = new GMEnode(txt_name, false);
    txt_node->setData(txt_data.c_str(), txt_data.size());
    txt_node->setParent(zip_root);
  }

  /* add the version txt node */
  if(ver_data.size()) {
    GMEnode* ver_node = new GMEnode(ver_name, false);
    ver_node->setData(ver_data.c_str(), ver_data.size());
    ver_node->setParent(zip_root);
  }

  GME_ModsMake_Arg_Struct* th_args = new GME_ModsMake_Arg_Struct;
  th_args->zip_root = zip_root;
  wcscpy(th_args->zip_path, zip_path.c_str());

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
  EnableMenuItem(g_hmnuMain, MNU_PROFILELOAD, MF_GRAYED);
  EnableMenuItem(g_hmnuMain, MNU_REPOSCONFIG, MF_GRAYED);
  EnableMenuItem(g_hmnuMain, MNU_REPOSQUERY, MF_GRAYED);
  EnableMenuItem(g_hmnuMain, MNU_REPOSMKXML, MF_GRAYED);
  EnableMenuItem(g_hmnuMain, MNU_MODIMPORT, MF_GRAYED);
  EnableMenuItem(g_hmnuMain, MNU_MODCREATE, MF_GRAYED);
  /* enable cancel */
  EnableWindow(GetDlgItem(g_hwndMain, BTN_MODCANCEL), true);


  for(unsigned i = 0; i < g_ModsProc_ArgList.size(); i++) {
    if(g_ModsProc_Cancel) {
      g_ModsProc_ArgList.clear();
      GME_ModsUpdList();
      return 0;
    }
    if(g_ModsProc_ArgList[i].action == MODS_ENABLE) {
      GME_ModsApplyMod(GetDlgItem(g_hwndMain, PBM_MOD), g_ModsProc_ArgList[i].name, g_ModsProc_ArgList[i].type);
      GME_ModsListQuickEnable(g_ModsProc_ArgList[i].name, true);

    }
    if(g_ModsProc_ArgList[i].action == MODS_DISABLE) {
      GME_ModsRestoreMod(GetDlgItem(g_hwndMain, PBM_MOD), g_ModsProc_ArgList[i].name);
      GME_ModsListQuickEnable(g_ModsProc_ArgList[i].name, false);
    }
  }

  g_ModsProc_ArgList.clear();

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
  EnableMenuItem(g_hmnuMain, MNU_PROFILELOAD, MF_BYCOMMAND);
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
    GME_DialogWarning(g_hwndMain, L"Mod(s) installation is currently processing, please wait until current process finish before enabling or disabling mod(s).");
  }
}

/*
  cancel current mod processing
*/
void GME_ModsProcCancel()
{
  if(!GME_ModsProc_IsReady()) {
    g_ModsProc_Cancel = true;
  }
}



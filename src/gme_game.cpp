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

#include "gme_conf.h"
#include "gme_logs.h"
#include "gme_game.h"
#include "gme_mods.h"
#include "gme_prof.h"
#include "gme_tools.h"

/* global list of configured games and current selected id */
std::vector<GME_GameCfg_Struct> g_GameCfg_List;
int g_GameCur_Id = -1;

void GME_GameUpdMenu()
{
  if(g_GameCur_Id != -1) {
    /* check if snapshot file exists */
    if(GME_IsFile(GME_GameGetCurConfPath() + L"\\snapshot.dat")) {
      EnableMenuItem(g_hmnuMain, MNU_SNAPCOMPARE, MF_BYCOMMAND);
    } else {
      EnableMenuItem(g_hmnuMain, MNU_SNAPCOMPARE, MF_GRAYED);
    }

    EnableMenuItem(g_hmnuMain, MNU_REPOSCONFIG, MF_BYCOMMAND);
    /* check if repos file exists */
    if(GME_IsFile(GME_GameGetCurConfPath() + L"\\repos.dat")) {
      EnableMenuItem(g_hmnuMain, MNU_REPOSQUERY, MF_BYCOMMAND);
    } else {
      EnableMenuItem(g_hmnuMain, MNU_REPOSQUERY, MF_GRAYED);
    }

    if(GME_ModsListIsEmpty()) {
      EnableMenuItem(g_hmnuMain, MNU_MODENAALL, MF_GRAYED);
      EnableMenuItem(g_hmnuMain, MNU_MODDISALL, MF_GRAYED);
      EnableMenuItem(g_hmnuMain, MNU_MODENA, MF_GRAYED);
      EnableMenuItem(g_hmnuMain, MNU_MODDIS, MF_GRAYED);
      EnableMenuItem(g_hmnuMain, MNU_PROFILSAVE, MF_GRAYED);
    } else {
      EnableMenuItem(g_hmnuMain, MNU_MODENAALL, MF_BYCOMMAND);
      EnableMenuItem(g_hmnuMain, MNU_MODDISALL, MF_BYCOMMAND);
      EnableMenuItem(g_hmnuMain, MNU_MODENA, MF_BYCOMMAND);
      EnableMenuItem(g_hmnuMain, MNU_MODDIS, MF_BYCOMMAND);
      EnableMenuItem(g_hmnuMain, MNU_PROFILSAVE, MF_BYCOMMAND);
      /* update profiles menu list */
      GME_ProfUpdMenu();
    }
  } else {
    EnableMenuItem(g_hmnuMain, MNU_GAMEREM, MF_GRAYED);
    EnableMenuItem(g_hmnuMain, MNU_GAMEEDIT, MF_GRAYED);
    EnableMenuItem(g_hmnuMain, MNU_SNAPCREATE, MF_GRAYED);
    EnableMenuItem(g_hmnuMain, MNU_SNAPCOMPARE, MF_GRAYED);
    EnableMenuItem(g_hmnuMain, MNU_REPOSQUERY, MF_GRAYED);
    EnableMenuItem(g_hmnuMain, MNU_REPOSCONFIG, MF_GRAYED);
    EnableMenuItem(g_hmnuMain, MNU_MODENAALL, MF_GRAYED);
    EnableMenuItem(g_hmnuMain, MNU_MODDISALL, MF_GRAYED);
    EnableMenuItem(g_hmnuMain, MNU_MODENA, MF_GRAYED);
    EnableMenuItem(g_hmnuMain, MNU_MODDIS, MF_GRAYED);
    EnableMenuItem(g_hmnuMain, MNU_PROFILSAVE, MF_GRAYED);
  }
}


/*
  global stuff for threaded mod stock dir tracking
*/

/* thread handles */
HANDLE g_ModsDir_hT;
DWORD g_ModsDir_iT;

/* handle for change notification */
HANDLE g_ModsDir_hChge;

/* threaded function to track mods stock folder changes */
DWORD WINAPI GME_ModsDir_Th(void* pargs)
{
  while(true) {
    if(WaitForSingleObject(g_ModsDir_hChge, INFINITE) == WAIT_OBJECT_0) {
      GME_ModsUpdList();
      UpdateWindow(g_hwndMain);
      FindNextChangeNotification(g_ModsDir_hChge);
    }
  }
}


/*
  function to safely clean threads and/or memory usage by game process
*/
void GME_GameClean()
{
  if(g_ModsDir_hT) {
    /* disable tracking for old folder */
    DWORD dwExit;
    GetExitCodeThread(g_ModsDir_hT, &dwExit);
    TerminateThread(g_ModsDir_hT, dwExit);
    CloseHandle(g_ModsDir_hT);
    g_ModsDir_hT = NULL;
  }
}


/*
  read a game config file
*/
bool GME_GameReadCfg(const std::wstring& path, GME_GameCfg_Struct* data)
{
  size_t r = 0;
  std::wstring gdat_path = path + L"\\game.dat";

  FILE* fp = _wfopen(gdat_path.c_str(), L"rb");
  if(fp) {
    r += fread(data, 1, sizeof(GME_GameCfg_Struct), fp);
    fclose(fp);
  }

  return (r == sizeof(GME_GameCfg_Struct));
}


/*
  write a game config file
*/
bool GME_GameWritCfg(const std::wstring& path, const GME_GameCfg_Struct* data)
{
  size_t w = 0;
  std::wstring gdat_path = path + L"\\game.dat";

  FILE* fp = _wfopen(gdat_path.c_str(), L"wb");
  if(fp) {
    w += fwrite(data, 1, sizeof(GME_GameCfg_Struct), fp);
    fclose(fp);
  }

  return (w == sizeof(GME_GameCfg_Struct));
}


/*
  add a new game config
*/
bool GME_GameNewCfg(const std::wstring& title, const std::wstring& root, const std::wstring& mods, bool use_custom_back, const std::wstring& backp)
{
  GME_Logs(GME_LOG_NOTICE, "GME_GameNewCfg", "Create new config", GME_StrToMbs(root).c_str());

  std::wstring conf_path = GME_GetAppdataPath() + L"\\" + GME_Md5(root);

  /* check if game cfg already exists */
  if(GME_IsDir(conf_path)) {
    GME_DialogWarning(g_hwndAddGame, L"Config for '" + root + L"' already exists.");
    return false;
  }

  /* check if game title already exists */
  for(unsigned i = 0; i < g_GameCfg_List.size(); i++) {
    if(title == g_GameCfg_List[i].title) {
      GME_DialogWarning(g_hwndAddGame, L"Config title '" + title + L"' already exists.");
      return false;
    }
  }

  /* check if game folder exists */
  if(!GME_IsDir(root)) {
    GME_DialogWarning(g_hwndAddGame, L"The root path '" + root + L"' is invalid.");
    return false;
  }

  /* check if mods folder exists */
  if(!GME_IsDir(mods)) {
    GME_DialogWarning(g_hwndAddGame, L"Mods stock folder path '" + mods + L"' is invalid.");
    return false;
  }

  if(use_custom_back) {
    /* check if custom backup path is valid */
    if(!GME_IsDir(backp)) {
      GME_DialogWarning(g_hwndAddGame, L"Backup folder path '" + backp + L"' is invalid.");
      return false;
    }
    /* check if custom backup path is already used by another game config */
    for(int i = 0; i < g_GameCfg_List.size(); i++) {
      if(i != g_GameCur_Id) {
        if(!wcscmp(backp.c_str(), g_GameCfg_List[i].back_dir)) {
          if(IDYES != GME_DialogWarningConfirm(g_hwndEdiGame, L"The backup folder path\n'" + backp + L"'\nis already used by the config '" + g_GameCfg_List[i].title + L"'.\n\nUsing same backup folder for two configurations is NOT a good idea, do you really want to continue ?")) {
            return false;
          }
        }
      }
    }
  }

  /* temporary */
  std::wstring temp_str;

  /* build default params */
  GME_GameCfg_Struct data;
  memset(&data, 0, sizeof(GME_GameCfg_Struct));
  /* game active (always active for now) */
  data.active = true;
  /* game title */
  wcscpy(data.title, title.c_str());
  /* game root path */
  wcscpy(data.root, root.c_str());
  /* game config path */
  wcscpy(data.conf_dir, conf_path.c_str());
  /* game use custom backup dir */
  data.cust_bdir = use_custom_back;
  /* game backed file path */
  if(data.cust_bdir) {
    temp_str = backp;
  } else {
    temp_str = conf_path + L"\\backups";
  }
  wcscpy(data.back_dir, temp_str.c_str());
  /* game mods stock path */
  temp_str = mods;
  wcscpy(data.mods_dir, temp_str.c_str());


  /* create game config path within home dir */
  if(!GME_IsDir(conf_path)) {
    /* create game config dir */
    if(!GME_DirCreate(conf_path)) {
      GME_DialogError(g_hwndAddGame, L"Unable to create configuration folder.");
      GME_Logs(GME_LOG_ERROR, "GME_GameNewCfg", "Unable to create config folder", GME_StrToMbs(conf_path).c_str());
      return false;
    }
    /* write config file */
    if(!GME_GameWritCfg(conf_path, &data)) {
      GME_DialogError(g_hwndAddGame, L"Unable to write configuration file.");
      GME_Logs(GME_LOG_ERROR, "GME_GameNewCfg", "Unable to write config file", GME_StrToMbs(conf_path).c_str());
      return false;
    }
    /* create backups dir */
    if(!GME_DirCreate(std::wstring(conf_path + L"\\backups"))) {
      GME_DialogError(g_hwndAddGame, L"Unable to create backup folder.");
      GME_Logs(GME_LOG_ERROR, "GME_GameNewCfg", "Unable to create backup subfolder", GME_StrToMbs(conf_path).c_str());
      return false;
    }
  }

  /* set the new game as default one */
  GME_ConfSetLastGame(GME_Md5(root));

  /* update game list */
  GME_GameUpdList();

  return true;
}

/*
  Delete a game config
*/
bool GME_GameRemCurCfg()
{
  GME_Logs(GME_LOG_NOTICE, "GME_GameRemCurCfg", "Delete current config", GME_StrToMbs(GME_GameGetCurTitle()).c_str());

  /* confirmation dialog */
  if(IDYES != GME_DialogWarningConfirm(g_hwndMain, L"Are you sure you want to remove config '" + GME_GameGetCurTitle() + L"' from management list ?")) {
    return false;
  }

  /* uninstall process */
  GME_ModsUninstall();

  /* remove from default game */
  GME_ConfResetLastGame();

  /* remove game config directory and return */
  GME_DirRemRecursive(GME_GetAppdataPath() + L"\\" + GME_Md5(GME_GameGetCurRoot()));

  /* no game selected */
  g_GameCur_Id = -1;

  /* update game list */
  if(!GME_GameUpdList()) {
    GME_DialogInfo(g_hwndMain, L"The configuration list is empty.");
  }

  /* remove game config directory and return */
  return true;
}


/*
  edit an existing game config
*/
bool GME_GameEditCurCfg(const std::wstring& title, const std::wstring& mods, bool use_custom_back, const std::wstring& backp)
{
  GME_Logs(GME_LOG_NOTICE, "GME_GameEditCurCfg", "Edit current config", GME_StrToMbs(GME_GameGetCurTitle()).c_str());

  /* check if game title already exists */
  for(int i = 0; i < (int)g_GameCfg_List.size(); i++) {
    if(i != g_GameCur_Id) {
      if(title == g_GameCfg_List[i].title) {
        GME_DialogWarning(g_hwndEdiGame, L"Config title '" + title + L"' already exists.");
        return false;
      }
    }
  }

  /* check if mods folder is a valid name */
  if(!GME_IsDir(mods)) {
    GME_DialogWarning(g_hwndEdiGame, L"Mods stock folder path '" + mods + L"' is invalid.");
    return false;
  }

  if(use_custom_back) {
    /* check if custom backup path is valid */
    if(!GME_IsDir(backp)) {
      GME_DialogWarning(g_hwndEdiGame, L"Backup folder path '" + backp + L"' is invalid.");
      return false;
    }

    /* check if custom backup path is already used by another game config */
    for(int i = 0; i < g_GameCfg_List.size(); i++) {
      if(i != g_GameCur_Id) {
        if(!wcscmp(backp.c_str(), g_GameCfg_List[i].back_dir)) {
          if(IDYES != GME_DialogWarningConfirm(g_hwndEdiGame, L"The backup folder path\n'" + backp + L"'\nis already used by the config '" + g_GameCfg_List[i].title + L"'.\n\nUsing same backup folder for two configurations is NOT a good idea, do you really want to continue ?")) {
            return false;
          }
        }
      }
    }
  }

  std::wstring conf_path = GME_GameGetCurConfPath();

  /* temporary */
  std::wstring temp_str;

  /* build default params */
  GME_GameCfg_Struct data;
  memset(&data, 0, sizeof(GME_GameCfg_Struct));
  /* game active (always active for now) */
  data.active = true;
  /* game title */
  wcscpy(data.title, title.c_str());
  /* game root path */
  wcscpy(data.root, GME_GameGetCurRoot().c_str());
  /* game config path */
  wcscpy(data.conf_dir, conf_path.c_str());
  /* game use custom backup dir */
  data.cust_bdir = use_custom_back;
  /* game backed file path */
  if(data.cust_bdir) {
    temp_str = backp;
  } else {
    temp_str = conf_path + L"\\backups";
  }
  wcscpy(data.back_dir, temp_str.c_str());
  /* game mods stock path */
  temp_str = mods;
  wcscpy(data.mods_dir, temp_str.c_str());

  /* if the backup path changed, we restore all backup */
  if(GME_GameGetCurBackPath() != data.back_dir) {
    GME_DialogWarning(g_hwndEdiGame, L"Backup folder changed for  '" + GME_GameGetCurTitle() + L"', all enabled Mod(s) will be disabled to empty old backup folder.");
    /* uninstall process */
    GME_ModsUninstall();
  }

  /* write config file */
  if(!GME_GameWritCfg(conf_path, &data)) {
    GME_DialogError(g_hwndEdiGame, L"Unable to write configuration file.");
    GME_Logs(GME_LOG_ERROR, "GME_GameEditCurCfg", "Unable to write config file", GME_StrToMbs(conf_path).c_str());
    return true;
  }

  /* set the game as default one */
  GME_ConfSetLastGame(GME_Md5(GME_GameGetCurRoot()));

  /* update game list */
  GME_GameUpdList();

  return true;
}

/*
  select a game config
*/
bool GME_GameSelectCfg(const std::wstring& title)
{
  GME_Logs(GME_LOG_NOTICE, "GME_GameSelectCfg", "Selecting config", GME_StrToMbs(title).c_str());

  /* no game selected */
  g_GameCur_Id = -1;

  /* disable proper menu items */
  EnableMenuItem(g_hmnuMain, MNU_GAMEEDIT, MF_GRAYED);
  EnableMenuItem(g_hmnuMain, MNU_GAMEREM, MF_GRAYED);
  EnableMenuItem(g_hmnuMain, MNU_SNAPCREATE, MF_GRAYED);

  /* disable tracking for old folder */
  FindCloseChangeNotification(g_ModsDir_hChge);
  DWORD dwExit;
  GetExitCodeThread(g_ModsDir_hT, &dwExit);
  TerminateThread(g_ModsDir_hT, dwExit);
  CloseHandle(g_ModsDir_hT);

  /* check if list is empty */
  if(g_GameCfg_List.empty()) {
    /* update mods list */
    GME_ModsUpdList();
    GME_DialogWarning(g_hwndMain, L"The configuration list is empty.");
    GME_Logs(GME_LOG_WARNING, "GME_GameSelectCfg", "Empty config list", "Nothing to select");
    return false;
  }

  for(unsigned i = 0; i < g_GameCfg_List.size(); i++) {
    /* search for game by title */
    if(title == g_GameCfg_List[i].title) {
      g_GameCur_Id = i;
      GME_ConfSetLastGame(GME_Md5(GME_GameGetCurRoot()));
      break;
    }
  }

  /* enable folder tracking for new folder */
  DWORD mask = FILE_NOTIFY_CHANGE_FILE_NAME|FILE_NOTIFY_CHANGE_DIR_NAME|FILE_NOTIFY_CHANGE_SIZE|FILE_NOTIFY_CHANGE_LAST_WRITE;
  g_ModsDir_hChge = FindFirstChangeNotificationW(GME_GameGetCurModsPath().c_str(), false, mask);
  g_ModsDir_hT = CreateThread(NULL,0,GME_ModsDir_Th,NULL,0,&g_ModsDir_iT);

  /* enable proper menu items */
  EnableMenuItem(g_hmnuMain, MNU_GAMEEDIT, MF_BYCOMMAND);
  EnableMenuItem(g_hmnuMain, MNU_GAMEREM, MF_BYCOMMAND);
  EnableMenuItem(g_hmnuMain, MNU_SNAPCREATE, MF_BYCOMMAND);

  /* set root path in UI entry*/
  SendMessageW(GetDlgItem(g_hwndMain, ENT_CFGRPATH), WM_SETTEXT, 0, (LPARAM)GME_GameGetCurRoot().c_str());

  /* update mods list */
  GME_ModsUpdList();

  return true;
}

/*
  Load game config list
*/
bool GME_GameUpdList()
{
  GME_Logs(GME_LOG_NOTICE, "GME_GameUpdList", "Updating config list", GME_StrToMbs(GME_GetAppdataPath()).c_str());

  HWND hcb = GetDlgItem(g_hwndMain, CMB_GAMELIST);

  /* reload game cfg list */
  g_GameCfg_List.clear();

  std::wstring cfg_path;
  GME_GameCfg_Struct cfg_data;
  FILE* cfg_fp;

  std::wstring srch_path = GME_GetAppdataPath() + L"\\*";

  WIN32_FIND_DATAW fdw;
  HANDLE hnd = FindFirstFileW(srch_path.c_str(), &fdw);
  if(hnd != INVALID_HANDLE_VALUE) {
    do {
      if(fdw.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        memset(&cfg_data, 0, sizeof(GME_GameCfg_Struct));
        cfg_path = GME_GetAppdataPath() + L"\\" + fdw.cFileName + L"\\game.dat";
        cfg_fp = _wfopen(cfg_path.c_str(), L"rb");
        if(cfg_fp) {
          fread(&cfg_data, 1, sizeof(GME_GameCfg_Struct), cfg_fp);
          g_GameCfg_List.push_back(cfg_data);
          GME_Logs(GME_LOG_NOTICE, "GME_GameUpdList", "Retrieving config", GME_StrToMbs(fdw.cFileName).c_str());
          fclose(cfg_fp);
        }
      }
    } while(FindNextFileW(hnd, &fdw));
  }
  FindClose(hnd);

  /* empty combo box */
  unsigned s = SendMessageW(hcb, CB_GETCOUNT, 0, 0);
  if(s) for(unsigned i = 0; i < s; i++) SendMessageW(hcb, CB_DELETESTRING, 0, 0);

  /* no game selected */
  g_GameCur_Id = -1;

  /* check if list is empty */
  if(g_GameCfg_List.empty()) {
    SendMessageW(hcb, CB_SETCURSEL, 0, 0); /* select first item in combo box */
    /* update mods list */
    GME_ModsUpdList();
    /* update menus */
    GME_GameUpdMenu();
    GME_Logs(GME_LOG_WARNING, "GME_GameUpdList", "Empty config list", "No config found");
    return false;
  }

  g_GameCur_Id = 0;
  std::wstring lastgame = GME_ConfGetLastGame();
  for(unsigned i = 0; i < g_GameCfg_List.size(); i++) {

    /* fill combo box */
    SendMessageW(hcb, CB_ADDSTRING, i, (LPARAM)g_GameCfg_List[i].title);

    /* search for last selected game */
    if(lastgame == GME_Md5(g_GameCfg_List[i].root)) {
      g_GameCur_Id = i;
    }
  }

  /* select default game in combo box */
  SendMessageW(hcb, CB_SETCURSEL, g_GameCur_Id, 0);

  /* select default game cfg */
  GME_GameSelectCfg(g_GameCfg_List[g_GameCur_Id].title);

  return true;
}


bool GME_GameChkSelect()
{
  HWND hcb = GetDlgItem(g_hwndMain, CMB_GAMELIST);

  /* retrieve selected item/string in combo box */
  wchar_t title[256];
  int i = SendMessageW(hcb, CB_GETCURSEL, 0, 0);
  SendMessageW(hcb, CB_GETLBTEXT, i, (LPARAM)title);

  /* select game config */
  GME_GameSelectCfg(title);

  return (g_GameCur_Id != -1);
}

/*
  function to get current selected game name
*/
std::wstring GME_GameGetCurTitle()
{
  return std::wstring(g_GameCfg_List[g_GameCur_Id].title);
}

/*
  function to get current selected game root path
*/
std::wstring GME_GameGetCurRoot()
{
  return std::wstring(g_GameCfg_List[g_GameCur_Id].root);
}

/*
  function to get current selected game mods path
*/
std::wstring GME_GameGetCurModsPath()
{
  return g_GameCfg_List[g_GameCur_Id].mods_dir;
}

/*
  function to get current selected game back path
*/
std::wstring GME_GameGetCurBackPath()
{
  return g_GameCfg_List[g_GameCur_Id].back_dir;
}

/*
  function to get current selected game config path
*/
std::wstring GME_GameGetCurConfPath()
{
  return g_GameCfg_List[g_GameCur_Id].conf_dir;
}

/*
  function to get current selected game config path
*/
bool GME_GameGetCurUseCustBack()
{
  return g_GameCfg_List[g_GameCur_Id].cust_bdir;
}

/*
  function to get current selected game id
*/
int GME_GameGetCurId()
{
  return g_GameCur_Id;
}

/*
  function to get current selected game id
*/
void GME_GameSetCurId(unsigned id)
{
  g_GameCur_Id = id;
}

/*
  function to get game config list size
*/
unsigned GME_GameGetCfgCount()
{
  return g_GameCfg_List.size();
}

/*
  function to get game config
*/
GME_GameCfg_Struct& GME_GameGetCfg(unsigned id)
{
  return g_GameCfg_List[id];
}

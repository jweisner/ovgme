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

/* mods profile struct */
struct GME_ProfilEntry_Struct
{
  wchar_t name[128];
  ubyte type;
  ubyte stat;
};


struct GME_Profile_Struct
{
  unsigned id;
  std::wstring name;
  std::wstring path;
};

/* list of available profiles */
std::vector<GME_Profile_Struct> g_GME_Profile_List;


unsigned GME_ProfGetCount()
{
  return g_GME_Profile_List.size();
}

std::wstring GME_ProfGetPath(unsigned id)
{
  return g_GME_Profile_List[id].path;
}

std::wstring GME_ProfGetName(unsigned id)
{
  return g_GME_Profile_List[id].name;
}


void GME_ProfUpdList()
{
  std::wstring conf_path = GME_GameGetCurConfPath();

  std::wstring prf_file;
  GME_Profile_Struct prf_entry;

  g_GME_Profile_List.clear();
  unsigned inc_id = 1;

  /* search for each .prf file in current game config folder */
  std::wstring prf_srch = conf_path + L"\\*.prf";
  WIN32_FIND_DATAW fdw;
  HANDLE hnd = FindFirstFileW(prf_srch.c_str(), &fdw);
  if(hnd != INVALID_HANDLE_VALUE) {
    do {
      if(!(fdw.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        prf_entry.path = conf_path + L"\\" + fdw.cFileName;
        prf_entry.name = GME_FilePathToName(fdw.cFileName);
        prf_entry.id = 40100 + inc_id;
        g_GME_Profile_List.push_back(prf_entry);
        inc_id++;
      }
    } while(FindNextFileW(hnd, &fdw));
  }
  FindClose(hnd);
}

void GME_ProfUpdMenu()
{
  /* update profile list */
  GME_ProfUpdList();

  /* delete previous profiles entries */
  int c = GetMenuItemCount(g_hmnuSubProf);
  for(int i = 3; i < c; i++) {
    RemoveMenu(g_hmnuSubProf, 3, MF_BYPOSITION);
  }

  /* add new profile list */
  if(g_GME_Profile_List.size()) {
    for(unsigned i = 0; i < g_GME_Profile_List.size(); i++) {
      AppendMenuW(g_hmnuSubProf, MF_STRING, g_GME_Profile_List[i].id, g_GME_Profile_List[i].name.c_str());
    }
    EnableMenuItem(g_hmnuMain, MNU_PROFILDELT, MF_BYCOMMAND);
  } else {
    AppendMenuW(g_hmnuSubProf, MF_STRING, 40100, L"No profile available");
    EnableMenuItem(g_hmnuMain, 40100, MF_GRAYED);
    EnableMenuItem(g_hmnuMain, MNU_PROFILDELT, MF_GRAYED);
  }
}

void GME_ProfEnaMenu(bool enable)
{
  /* delete previous profiles entries */
  if(enable) {
    for(int i = 0; i < g_GME_Profile_List.size(); i++) {
      EnableMenuItem(g_hmnuMain, g_GME_Profile_List[i].id, MF_BYCOMMAND);
    }
  } else {
    for(int i = 0; i < g_GME_Profile_List.size(); i++) {
      EnableMenuItem(g_hmnuMain, g_GME_Profile_List[i].id, MF_GRAYED);
    }
  }
}

bool GME_ProfDelete()
{
  HWND hcb = GetDlgItem(g_hwndProfDel, CMB_PROFLIST);
  /* retrieve selected item/string in combo box */
  wchar_t name[256];
  SendMessageW(hcb, CB_GETLBTEXT, SendMessageW(hcb, CB_GETCURSEL, 0, 0), (LPARAM)name);

  std::wstring prfl_name = name;

  int prof_id = -1;
  /* search for profile name in list */
  for(int i = 0; i < g_GME_Profile_List.size(); i++) {
    if(g_GME_Profile_List[i].name == prfl_name) {
      prof_id = i;
      break;
    }
  }

  if(prof_id < 0) {
    GME_DialogWarning(g_hwndProfDel, L"The profile '" + prfl_name + L"' do not exist.");
    GME_Logs(GME_LOG_ERROR, "GME_ProfDelete", "Cannot delete non existing profile", GME_StrToMbs(prfl_name).c_str());
    return false;
  }

  std::wstring prfl_file = g_GME_Profile_List[prof_id].path;

  if(GME_IsFile(prfl_file)) {
    if(IDYES != GME_DialogQuestionConfirm(g_hwndProfDel, L"Do you want to delete the profile '" + prfl_name + L"' ?"))
      return false;
  } else {
    GME_DialogWarning(g_hwndProfDel, L"The profile '" + prfl_name + L"' data file do not exist.");
    GME_Logs(GME_LOG_ERROR, "GME_ProfDelete", "Cannot delete non existing profile file", GME_StrToMbs(prfl_file).c_str());
    return false;
  }

  if(!GME_FileDelete(prfl_file)) {
    GME_DialogWarning(g_hwndProfDel, L"Unable to delete profile data '" + prfl_file + L"'.");
    GME_Logs(GME_LOG_ERROR, "GME_ProfDelete", "Unable to delete profile file", GME_StrToMbs(prfl_file).c_str());
  }

  GME_Logs(GME_LOG_ERROR, "GME_ProfDelete", "Delete mods profile", GME_StrToMbs(prfl_name).c_str());

  return true;
}

bool GME_ProfCreate(const wchar_t* name)
{
  std::wstring prfl_name = name;
  std::wstring prfl_file = GME_GameGetCurConfPath() + L"\\" + prfl_name + L".prf";

  if(GME_IsFile(prfl_file)) {
    if(IDYES != GME_DialogWarningConfirm(g_hwndProfNew, L"Mods profile '" + prfl_name + L"' already exists for this game, do you want to overwrite it ?"))
      return false;
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

  GME_Logs(GME_LOG_NOTICE, "GME_ProfCreate", "Create mods profile", GME_StrToMbs(name).c_str());

  return true;
}




/*
  function to apply mods profile
*/
bool GME_ProfApply(unsigned mid)
{
  if(!GME_ModsProc_IsReady()) {
    GME_DialogWarning(g_hwndMain, L"Mod(s) installation is currently processing, please wait until current process finish before enabling or disabling Mod(s).");
    return false;
  }

  std::wstring prf_path;

  /* retrieve profile by menu id */
  for(unsigned i = 0; i < g_GME_Profile_List.size(); i++) {
    if(g_GME_Profile_List[i].id == mid) {
      prf_path = g_GME_Profile_List[i].path;
    }
  }

  if(!GME_IsFile(prf_path)) {
    GME_DialogWarning(g_hwndMain, L"Unable to open profile data file '" + prf_path + L"'");
    return false;
  }

  GME_Logs(GME_LOG_NOTICE, "GME_ProfApply", "Get mods profile data", GME_StrToMbs(prf_path).c_str());

  /* load profile data file */
  GME_ProfilEntry_Struct profilentry;
  std::vector<GME_ProfilEntry_Struct> profilentry_list;

  /* read the profile data in config dir */
  FILE* fp = _wfopen(prf_path.c_str(), L"rb");
  if(fp) {
    /* first 4 bytes is count of entries */
    unsigned c;
    fread(&c, 1, 4, fp);
    for(unsigned i = 0; i < c; i++) {
      fread(&profilentry, 1, sizeof(GME_ProfilEntry_Struct), fp);
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
        GME_Logs(GME_LOG_WARNING, "GME_ProfApply", "Mods not found in current mods folder", GME_StrToMbs(profilentry_list[i].name).c_str());
        continue;
      }
    }
    if(profilentry_list[i].type == 1) {
      if(!GME_ZipIsValidMod(mod_path + L".zip")) {
        missing_list.push_back(profilentry_list[i].name);
        GME_Logs(GME_LOG_WARNING, "GME_ProfApply", "Mods not found in current mods folder", GME_StrToMbs(profilentry_list[i].name).c_str());
        continue;
      }
    }
    if(profilentry_list[i].stat) {
      GME_ModsProc_PushApply(profilentry_list[i].name, profilentry_list[i].type);
    } else {
      GME_ModsProc_PushRestore(profilentry_list[i].name);
    }
  }

  GME_Logs(GME_LOG_NOTICE, "GME_ProfApply", "Apply profile", "Launch mods process thread");

  GME_ModsProc_Launch();

  if(!missing_list.empty()) {
    std::wstring message = L"One or more mod registered in profile was not found:\n\n";
    for(unsigned i = 0; i < missing_list.size(); i++) {
      message += L"  " + missing_list[i] + L"\n";
    }
    GME_DialogWarning(g_hwndMain, message);
  }

  GME_Logs(GME_LOG_NOTICE, "GME_ProfApply", "Apply profile", "Done");

  return true;
}


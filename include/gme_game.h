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

#ifndef GME_GAME_H_INCLUDED
#define GME_GAME_H_INCLUDED

#include "gme.h"

/* struct for game config file */
struct GME_GameCfg_Struct
{
  bool active;
  wchar_t title[64];
  wchar_t root[260];
  bool cust_bdir;
  wchar_t conf_dir[260];
  wchar_t mods_dir[260];
  wchar_t back_dir[260];
  wchar_t cust_dir[260]; /* provision for future usage */
};


bool GME_GameReadCfg(const std::wstring& path, GME_GameCfg_Struct* data);
bool GME_GameWritCfg(const std::wstring& path, const GME_GameCfg_Struct* data);
bool GME_GameNewCfg(const std::wstring& title, const std::wstring& root, const std::wstring& mods, bool use_custom_back, const std::wstring& backp);
bool GME_GameRemCurCfg();
bool GME_GameEditCurCfg(const std::wstring& title, const std::wstring& mods, bool use_custom_back, const std::wstring& backp);
void GME_GameUpdMenu();
bool GME_GameSelectCfg(const std::wstring& title);
bool GME_GameUpdList();
bool GME_GameChkSelect();
std::wstring GME_GameGetCurTitle();
std::wstring GME_GameGetCurRoot();
std::wstring GME_GameGetCurModsPath();
std::wstring GME_GameGetCurBackPath();
std::wstring GME_GameGetCurConfPath();
bool GME_GameGetCurUseCustBack();
int GME_GameGetCurId();
unsigned GME_GameGetCfgCount();
GME_GameCfg_Struct& GME_GameGetCfg(unsigned id);
void GME_GameClean();



#endif // GME_GAME_H_INCLUDED

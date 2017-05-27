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

#ifndef GME_CONF_H_INCLUDED
#define GME_CONF_H_INCLUDED

#include "gme.h"

/* struct for main config file */
struct GME_ConfCfg_Struct
{
  int vmajor;
  int vminor;
  wchar_t last_game[64];
  int winw;
  int winh;
  int winx;
  int winy;
  bool ensort;
};


bool GME_ConfWritCfg(GME_ConfCfg_Struct* data);
bool GME_ConfReadCfg(GME_ConfCfg_Struct* data);
bool GME_ConfLoadCfg();
bool GME_ConfSaveCfg();
std::wstring GME_ConfGetLastGame();
void GME_ConfResetLastGame();
void GME_ConfSetLastGame(const std::wstring& str);
int GME_ConfGetWinW();
int GME_ConfGetWinH();
int GME_ConfGetWinX();
int GME_ConfGetWinY();
bool GME_ConfGetEnSort();


#endif // GME_CONF_H_INCLUDED

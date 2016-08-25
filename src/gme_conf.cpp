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
#include "gme_tools.h"

/* config file main instance */
GME_ConfCfg_Struct g_ConfCfg;

/*
  Write main config file
*/
bool GME_ConfWritCfg(GME_ConfCfg_Struct* data)
{
  size_t w = 0;
  std::wstring cfg_path = GME_GetAppdataPath() + L"\\ovgme.dat";

  FILE* fp = _wfopen(cfg_path.c_str(), L"wb");
  if(fp) {
    w += fwrite(data, 1, sizeof(GME_ConfCfg_Struct), fp);
    fclose(fp);
  }
  return (w == sizeof(GME_ConfCfg_Struct));
}

/*
  Read main config file
*/
bool GME_ConfReadCfg(GME_ConfCfg_Struct* data)
{
  size_t r = 0;
  std::wstring cfg_path = GME_GetAppdataPath() + L"\\ovgme.dat";

  FILE* fp = _wfopen(cfg_path.c_str(), L"rb");
  if(fp) {
    r += fread(data, 1, sizeof(GME_ConfCfg_Struct), fp);
    fclose(fp);
  }
  return (r == sizeof(GME_ConfCfg_Struct));
}

/*
  Load main config file
*/
bool GME_ConfLoadCfg()
{
  /* init home dir */
  std::wstring home_path = GME_GetAppdataPath();

  /* check if folder exists */
  if(!GME_IsDir(home_path)) {
    /* create home folder */
    if(!CreateDirectoryW(home_path.c_str(), NULL)) {
      GME_DialogError(NULL, L"Fatal error: Unable to create home directory. OvGME can't create its global configuration.");
      return false;
    }
  }

  /* try to read main config file */
  if(!GME_ConfReadCfg(&g_ConfCfg)) {

    /* main config does not exists ? we create it */
    memset(&g_ConfCfg, 0, sizeof(GME_ConfCfg_Struct));
    g_ConfCfg.vmajor = GME_APP_MAJOR;
    g_ConfCfg.vminor = GME_APP_MINOR;

    if(!GME_ConfWritCfg(&g_ConfCfg)) {
      GME_DialogError(NULL, L"Fatal error: Unable to write main configuration file. OvGME can't create its global configuration.");
      return false;
    }
  }

  /* check main config version */
  if(g_ConfCfg.vmajor != GME_APP_MAJOR || g_ConfCfg.vminor != GME_APP_MINOR) {

    /* version mismatch, recreate default config */
    memset(&g_ConfCfg, 0, sizeof(GME_ConfCfg_Struct));
    g_ConfCfg.vmajor = GME_APP_MAJOR;
    g_ConfCfg.vminor = GME_APP_MINOR;

    if(!GME_ConfWritCfg(&g_ConfCfg)) {
      GME_DialogError(NULL, L"Fatal error: Unable to write main configuration file. OvGME can't create its global configuration.");
      return false;
    }
  }

  return true;
}

/*
  Save main config file
*/
bool GME_ConfSaveCfg()
{
  if(!GME_ConfWritCfg(&g_ConfCfg)) {
    GME_DialogError(NULL, L"Fatal error: Unable to write main configuration file. OvGME can't save its global configuration.");
    return false;
  }

  return true;
}

/*
  get last selected game from config
*/
std::wstring GME_ConfGetLastGame()
{
  return std::wstring(g_ConfCfg.last_game);
}

/*
  reset last selected game to config
*/
void GME_ConfResetLastGame()
{
  memset(&g_ConfCfg.last_game, 0, 64);
}

/*
  set last selected game to config
*/
void GME_ConfSetLastGame(const std::wstring& str)
{
  wcscpy(g_ConfCfg.last_game, str.c_str());
}

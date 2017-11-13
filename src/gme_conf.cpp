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
    if(!GME_DirCreate(home_path)) {
      GME_DialogError(NULL, L"Fatal error: Unable to create home directory. OvGME can't create its global configuration.");
      GME_Logs(GME_LOG_ERROR, "GME_ConfLoadCfg", "Create OvGME home directory", "Failed");
      return false;
    }
  }

  /* try to read main config file */
  if(!GME_ConfReadCfg(&g_ConfCfg)) {

    /* main config does not exists ? we create it */
    memset(&g_ConfCfg, 0, sizeof(GME_ConfCfg_Struct));
    g_ConfCfg.vmajor = GME_APP_MAJOR;
    g_ConfCfg.vminor = GME_APP_MINOR;
    /* get main window size */
    RECT rect;
    GetWindowRect(g_hwndMain, &rect);
    g_ConfCfg.winw = rect.right - rect.left;
    g_ConfCfg.winh = rect.bottom - rect.top;
    g_ConfCfg.winx = rect.left;
    g_ConfCfg.winy = rect.top;
    g_ConfCfg.ensort = false;

    if(!GME_ConfWritCfg(&g_ConfCfg)) {
      GME_DialogError(NULL, L"Unable to write main configuration file. OvGME can't save its global configuration.");
      GME_Logs(GME_LOG_ERROR, "GME_ConfLoadCfg", "Write default configuration", "Failed");
      return false;
    }
  }

   /* app was probably closed when minimized, we restore default values */
  if(g_ConfCfg.winx < 0 || g_ConfCfg.winy < 0) {
    RECT rect;
    GetWindowRect(g_hwndMain, &rect);
    g_ConfCfg.winw = rect.right - rect.left;
    g_ConfCfg.winh = rect.bottom - rect.top;
    g_ConfCfg.winx = rect.left;
    g_ConfCfg.winy = rect.top;
    g_ConfCfg.ensort = false;
  }

  /* check main config version */
  if(g_ConfCfg.vmajor != GME_APP_MAJOR || g_ConfCfg.vminor != GME_APP_MINOR) {

    /* version mismatch, recreate default config */
    memset(&g_ConfCfg, 0, sizeof(GME_ConfCfg_Struct));
    g_ConfCfg.vmajor = GME_APP_MAJOR;
    g_ConfCfg.vminor = GME_APP_MINOR;
    /* get main window size */
    RECT rect;
    GetWindowRect(g_hwndMain, &rect);
    g_ConfCfg.winw = rect.right - rect.left;
    g_ConfCfg.winh = rect.bottom - rect.top;
    g_ConfCfg.winx = rect.left;
    g_ConfCfg.winy = rect.top;
    g_ConfCfg.ensort = false;

    if(!GME_ConfWritCfg(&g_ConfCfg)) {
      GME_DialogError(NULL, L"Unable to write main configuration file. OvGME can't save its global configuration.");
      GME_Logs(GME_LOG_ERROR, "GME_ConfLoadCfg", "Write default configuration", "Failed");
      return false;
    }
  }

  GME_Logs(GME_LOG_NOTICE, "GME_ConfLoadCfg", "Load global configuration", "Done");

  return true;
}

/*
  Save main config file
*/
bool GME_ConfSaveCfg()
{
  /* get main window size */
  RECT rect;
  GetWindowRect(g_hwndMain, &rect);
  g_ConfCfg.winw = rect.right - rect.left;
  g_ConfCfg.winh = rect.bottom - rect.top;
  g_ConfCfg.winx = rect.left;
  g_ConfCfg.winy = rect.top;
  g_ConfCfg.ensort = SendMessage(GetDlgItem(g_hwndMain, CHK_SORTMODS), BM_GETCHECK, 0, 0);

  if(!GME_ConfWritCfg(&g_ConfCfg)) {
    GME_DialogError(NULL, L"Unable to write main configuration file. OvGME can't save its global configuration.");
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

/*
  get saved window width from config
*/
int GME_ConfGetWinW()
{
  return g_ConfCfg.winw;
}

/*
  get saved window height from config
*/
int GME_ConfGetWinH()
{
  return g_ConfCfg.winh;
}

/*
  get saved window position X from config
*/
int GME_ConfGetWinX()
{
  return g_ConfCfg.winx;
}

/*
  get saved window position Y from config
*/
int GME_ConfGetWinY()
{
  return g_ConfCfg.winy;
}

/*
  get saved sort by enabled checkbox
*/
bool GME_ConfGetEnSort()
{
  return g_ConfCfg.ensort;
}

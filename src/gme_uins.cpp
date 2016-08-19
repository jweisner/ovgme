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

#include "gme_uins.h"
#include "gme_mods.h"
#include "gme_game.h"
#include "gme_tools.h"

/*
  function for application uninstall, called by setup through command line option
*/
void GME_Uninstall()
{
  /* this is the uninstall process, it restore all backups */
  GME_DialogWarning(NULL, L"All enabled mods will be disabled to restore original game files.");

  /* create the available game list */
  GME_GameUpdList();
  /* for each game*/
  for(unsigned g = 0; g < GME_GameGetCfgCount(); g++) {
    /* set the game as current selected */
    GME_GameSelectCfg(GME_GameGetCfg(g).title);
    /* uninstall process */
    GME_ModsUninstall();
    /* remove game directory */
    GME_DirRemRecursive(GME_GetAppdataPath() + L"\\" + GME_Md5(GME_GameGetCurRoot()));
  }
  /* remove config file */
  DeleteFileW(std::wstring(GME_GetAppdataPath()+L"\\ovgme.dat").c_str());
}

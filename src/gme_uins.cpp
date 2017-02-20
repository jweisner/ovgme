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
#include "gme_logs.h"
#include "gme_mods.h"
#include "gme_game.h"
#include "gme_tools.h"

/*
  function for application uninstall, called by setup through command line option
*/
void GME_Uninstall()
{
  /* create the available game list */
  GME_GameUpdList();

  if(GME_GameGetCfgCount()) {

    /* for each game*/
    for(unsigned i = 0; i < GME_GameGetCfgCount(); i++) {
      /* set the game as current selected */
      GME_GameSetCurId(i);
      /* uninstall process */
      GME_ModsUninstall();
    }

    if(IDYES == GME_DialogQuestionConfirm(NULL, L"Do you want also remove all OvGME settings and configurations ?")) {
      GME_DirRemRecursive(GME_GetAppdataPath());
    }
  }
}

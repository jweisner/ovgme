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

#include "gme.h"
#include "gme_uins.h"
#include "gme_netw.h"
#include "gme_dlg_main.h"

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* argv, int nShowCmd)
{
  g_hInst = hInstance;
  InitCommonControls();

  const char* readme = "Ceci est un text de readme\r\nBLAH!\r\n";

  mz_zip_archive za; // Zip archive struct
  mz_zip_archive_file_stat zf; // Zip file stat struct;

  memset(&za, 0, sizeof(mz_zip_archive));
  if(!mz_zip_reader_init_file(&za, "D:\\README.zip", 0)) {
    std::cout << "caca\n";
  }

  if(!mz_zip_writer_init_from_reader(&za, "D:\\README.zip")) {
    std::cout << "caca\n";
  }

  mz_zip_writer_add_mem(&za, "README.txt", readme, strlen(readme), 6);

  mz_zip_writer_finalize_archive(&za);
  mz_zip_writer_end(&za);

  /* search for uninstall parameter */
  if(strstr(argv, "/uninstall")) {
    /* go uninstall and exit */
    GME_Uninstall();
    return 0;
  }

  /* main dialog window */
  return DialogBox(g_hInst, MAKEINTRESOURCE(DLG_MAIN), NULL, (DLGPROC)GME_DlgMain);
}

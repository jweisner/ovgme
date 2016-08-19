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
#include "gme_logs.h"
#include "gme_tools.h"

std::wstring g_LogFile;

void GME_LogsInit()
{
  g_LogFile = GME_GetAppdataPath() + L"\\log.txt";
  FILE* fp = _wfopen(g_LogFile.c_str(), L"wb");
  if(fp) fclose(fp);
}

void GME_LogsAppend(const std::string& log)
{
  FILE* fp = _wfopen(g_LogFile.c_str(), L"ab");
  if(fp) {
    fwrite(log.c_str(), 1, log.size(), fp);
    fclose(fp);
  }
}


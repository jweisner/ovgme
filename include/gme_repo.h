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

#ifndef GME_REPO_H_INCLUDED
#define GME_REPO_H_INCLUDED

/* struct for mod version */
struct GME_ModVers_Struct
{
  short major;
  short minor;
  short revis;
};

void GME_RepoClean();
bool GME_RepoWritList();
bool GME_RepoReadList();
bool GME_RepoChkList();
bool GME_RepoUpdList();
bool GME_RepoAddUrl(const char* url);
bool GME_RepoRemUrl();
void GME_RepoQueryUpd();
bool GME_RepoChkDesc();
void GME_RepoDownloadSel();
void GME_RepoDownloadAll();
void GME_RepoQueryCancel();
std::string GME_RepoMakeXml(const char* url_str, bool cust_path, const wchar_t* path_str);
bool GME_RepoSaveXml();

#endif // GME_REPO_H_INCLUDED

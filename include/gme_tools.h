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

#ifndef GME_TOOLS_H_INCLUDED
#define GME_TOOLS_H_INCLUDED

#include "gme.h"
#include "gmenode.h"

std::wstring GME_GetAppdataPath();
std::wstring GME_DirPathToName(const std::wstring& path);
std::wstring GME_DirPathToName(const wchar_t* cpath);
std::string GME_DirPathToName(const std::string& path);
std::wstring GME_FilePathToName(const std::wstring& path);
std::wstring GME_FilePathToName(const wchar_t* cpath);
std::string GME_FilePathToName(const std::string& path);
void GME_StrSplit(const std::wstring& str, std::vector<std::wstring>* sbstr, const wchar_t* separator);
void GME_StrSplit(const std::string& str, std::vector<std::wstring>* sbstr, const char* separator);
void GME_StrSplit(const std::string& str, std::vector<std::string>* sbstr, const char* separator);
void GME_StrToUpper(std::wstring& str);
void GME_StrToUpper(std::string& str);
std::string GME_StrToUpper(const char* str);
std::wstring GME_StrToUpper(const std::wstring& str);
void GME_StrToLower(std::wstring& str);
void GME_StrToLower(std::string& str);
std::string GME_StrToLower(const char* str);
std::string GME_StrToMbs(const std::wstring& str);
void GME_StrToMbs(std::string& mbs, const std::wstring& str);
std::wstring GME_StrToWcs(const std::string& str);
void GME_StrToWcs(std::wstring& wcs, const std::string& str);
bool GME_StrIsValidFilename(const std::wstring& name);
void GME_FileList(const wchar_t* origin, std::vector<std::wstring>* lst, const wchar_t* filter);
void GME_DirList(const wchar_t* origin, std::vector<std::wstring>* lst);
void GME_FileListRecursive(const wchar_t* origin, std::vector<std::wstring>* lst, const wchar_t* filter);
bool GME_IsFile(const std::wstring& path);
bool GME_IsDir(const std::wstring& path);
bool GME_DirRemRecursive(const std::wstring& path);
bool GME_DirRemToTrash(const std::wstring& path);
size_t GME_FileSize(const std::wstring& src);
bool GME_FileRead(ubyte* data, size_t size, const std::wstring& src);
bool GME_FileWrite(const ubyte* data, size_t size, const std::wstring& dst, bool overwrite);
bool GME_FileCopy(const std::wstring& src, const std::wstring& dst, bool overwrite);
size_t GME_FileGetAsciiContent(const std::wstring& path, std::wstring* content);
void GME_DialogError(HWND hwnd, std::wstring message);
void GME_DialogWarning(HWND hwnd, std::wstring message);
int GME_DialogWarningConfirm(HWND hwnd, std::wstring message);
int GME_DialogQuestionConfirm(HWND hwnd, std::wstring message);
void GME_DialogInfo(HWND hwnd, std::wstring message);
void GME_DialogDirChooser(HWND hwnd, wchar_t* path, size_t max);
bool GME_DialogFileOpen(HWND hwnd, wchar_t* path, size_t max, unsigned* path_offset, const wchar_t* filter, const wchar_t* title);
bool GME_DialogFileSave(HWND hwnd, wchar_t* path, size_t max, unsigned* path_offset, const wchar_t* ext, const wchar_t* filter, const wchar_t* title);
bool GME_IsZip(const std::wstring& zip);
bool GME_ZipIsValidMod(const std::wstring& zip);
bool GME_ZipGetModDesc(const std::wstring& zip, std::wstring* desc);
bool GME_ZipGetModVers(const std::wstring& zip, std::wstring* vers);
void GME_TreeBuildFromDir(GMEnode* node, const std::wstring& base_dir);
bool GME_TreeBuildFromZip(GMEnode* root, const std::wstring& zip);
std::wstring GME_Md5(const std::wstring& str);
unsigned GME_crc32(unsigned crc, const ubyte *buf, size_t size);
unsigned GME_FileGetXxH32(const std::wstring& src);


#endif // GME_TOOLS_H_INCLUDED

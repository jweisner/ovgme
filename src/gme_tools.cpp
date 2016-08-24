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

#include "gme_tools.h"
#include "gme_logs.h"

/* -------------------------------- System related toolkit ------------------------------------- */

/*
  function to get path to OvGME home (AppData) dir
*/
std::wstring GME_GetAppdataPath()
{
  wchar_t buff[MAX_PATH];
  SHGetFolderPathW(NULL, CSIDL_COMMON_APPDATA, NULL, 0, buff);
  std::wstring hpath = buff;
  hpath.append(L"\\OvGME");
  return hpath;
}

/* -------------------------------- String related toolkit ------------------------------------- */

/*
  function to extract the name of a folder. This simply take the folder
  name without the full path.
*/
std::wstring GME_DirPathToName(const std::wstring& path)
{
  size_t sep_pos;
  sep_pos = path.find_last_of('\\');
  return path.substr(sep_pos+1, -1);
}

std::wstring GME_DirPathToName(const wchar_t* cpath)
{
  size_t sep_pos;
  std::wstring path = cpath;
  sep_pos = path.find_last_of('\\');
  return path.substr(sep_pos+1, -1);
}

std::string GME_DirPathToName(const std::string& path)
{
  size_t sep_pos;
  sep_pos = path.find_last_of('\\');
  return path.substr(sep_pos+1, -1);
}

/*
  function to extract the name of a file. This simply take the file name
  without the full path and without the file extension.
*/
std::wstring GME_FilePathToName(const std::wstring& path)
{
  size_t sep_pos, dot_pos, len;
  sep_pos = path.find_last_of('\\');
  dot_pos = path.find_last_of('.');
  len = (dot_pos-sep_pos)-1;
  return path.substr(sep_pos+1, len);
}

std::wstring GME_FilePathToName(const wchar_t* cpath)
{
  size_t sep_pos, dot_pos, len;
  std::wstring path = cpath;
  sep_pos = path.find_last_of('\\');
  dot_pos = path.find_last_of('.');
  len = (dot_pos-sep_pos)-1;
  return path.substr(sep_pos+1, len);
}

std::string GME_FilePathToName(const std::string& path)
{
  size_t sep_pos, dot_pos, len;
  sep_pos = path.find_last_of('\\');
  dot_pos = path.find_last_of('.');
  len = (dot_pos-sep_pos)-1;
  return path.substr(sep_pos+1, len);
}

/*
  function to split string
*/
void GME_StrSplit(const std::wstring& str, std::vector<std::wstring>* sbstr, const wchar_t* separator)
{
  sbstr->clear();
  size_t b = 0, e = 0;
  while(b < str.size() && e < str.size()) {
    b = str.find_first_not_of(separator, e);
    if(b >= str.size())
      break;
    e = str.find_first_of(separator, b);
    if(e > str.size())
      e = str.size();
    sbstr->push_back(str.substr(b,e-b));
  }
}

void GME_StrSplit(const std::string& str, std::vector<std::wstring>* sbstr, const char* separator)
{
  sbstr->clear();
  size_t b = 0, e = 0;
  while(b < str.size() && e < str.size()) {
    b = str.find_first_not_of(separator, e);
    if(b >= str.size())
      break;
    e = str.find_first_of(separator, b);
    if(e > str.size())
      e = str.size();
    sbstr->push_back(GME_StrToWcs(str.substr(b,e-b)));
  }
}

void GME_StrSplit(const std::string& str, std::vector<std::string>* sbstr, const char* separator)
{
  sbstr->clear();
  size_t b = 0, e = 0;
  while(b < str.size() && e < str.size()) {
    b = str.find_first_not_of(separator, e);
    if(b >= str.size())
      break;
    e = str.find_first_of(separator, b);
    if(e > str.size())
      e = str.size();
    sbstr->push_back(str.substr(b,e-b));
  }
}

/*
  functon to make string upper case
*/
void GME_StrToUpper(std::wstring& str)
{
  size_t s = str.length();
  for(unsigned i = 0; i < s; i++)
    str[i] = std::towupper(str[i]);
}

void GME_StrToUpper(std::string& str)
{
  size_t s = str.length();
  for(unsigned i = 0; i < s; i++)
    str[i] = std::towupper(str[i]);
}

std::string GME_StrToUpper(const char* str)
{
  std::string ret;
  size_t s = strlen(str);
  for(unsigned i = 0; i < s; i++)
    ret.push_back(std::towupper(str[i]));
  return ret;
}

std::wstring GME_StrToUpper(const std::wstring& str)
{
  std::wstring ret;
  size_t s = str.size();
  for(unsigned i = 0; i < s; i++)
    ret.push_back(std::towupper(str[i]));
  return ret;
}

/*
  functon to make string lower case
*/
void GME_StrToLower(std::wstring& str)
{
  size_t s = str.length();
  for(unsigned i = 0; i < s; i++)
    str[i] = std::towlower(str[i]);
}

void GME_StrToLower(std::string& str)
{
  size_t s = str.length();
  for(unsigned i = 0; i < s; i++)
    str[i] = std::towlower(str[i]);
}

std::string GME_StrToLower(const char* str)
{
  std::string ret;
  size_t s = strlen(str);
  for(unsigned i = 0; i < s; i++)
    ret.push_back(std::towlower(str[i]));
  return ret;
}

/*
  function to convert wide char std::wstring to mbs std::string
*/
std::string GME_StrToMbs(const std::wstring& str)
{
  std::string mbs;
  size_t s = str.length()*2;
  char* buff = new char[s];
  memset(buff, 0, sizeof(char)*s);
  wcstombs(buff, str.c_str(), s);
  mbs.assign(buff);
  delete [] buff;
  return mbs;
}

inline void GME_StrToMbs(std::string& mbs, const std::wstring& str)
{
  size_t s = str.length()*2;
  char* buff = new char[s];
  memset(buff, 0, sizeof(char)*s);
  wcstombs(buff, str.c_str(), s);
  mbs.assign(buff);
  delete [] buff;
}

/*
  function to convert mbs std::string to wide char std::wstring
*/
std::wstring GME_StrToWcs(const std::string& str)
{
  std::wstring wcs;
  size_t s = str.length()+1;
  wchar_t* buff = new wchar_t[s];
  memset(buff, 0, sizeof(wchar_t)*s);
  mbstowcs(buff, str.c_str(), s);
  wcs.assign(buff);
  delete[] buff;
  return wcs;
}

void GME_StrToWcs(std::wstring& wcs, const std::string& str)
{
  size_t s = str.length()+1;
  wchar_t* buff = new wchar_t[s];
  memset(buff, 0, sizeof(wchar_t)*s);
  mbstowcs(buff, str.c_str(), s);
  wcs.assign(buff);
  delete[] buff;
}


/*
  function to test if a name contains illegal characters
*/
bool GME_StrIsValidFilename(const std::wstring& name)
{
  for(unsigned i = 0; i < name.size(); i++) {
    if(name[i] == '<') return false;
    if(name[i] == '>') return false;
    if(name[i] == ':') return false;
    if(name[i] == '"') return false;
    if(name[i] == '/') return false;
    if(name[i] == '\\') return false;
    if(name[i] == '|') return false;
    if(name[i] == '?') return false;
    if(name[i] == '*') return false;
  }
  return true;
}

/* ------------------------- Files and Directory related toolkit ------------------------------- */

/*
  function to list all files in a folder.
*/
void GME_FileList(const wchar_t* origin, std::vector<std::wstring>* lst, const wchar_t* filter)
{
  std::wstring item;
  std::wstring path = origin; path += L"\\"; path += filter;

  WIN32_FIND_DATAW fdw;
  HANDLE hnd = FindFirstFileW(path.c_str(), &fdw);
  if(hnd != INVALID_HANDLE_VALUE) {
    do {
      if(!(fdw.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        item = origin; item += L"\\"; item += fdw.cFileName;
        lst->push_back(item);
      }
    } while(FindNextFileW(hnd, &fdw));
  }
  FindClose(hnd);
}

/*
  function to list all subfolders in a folder.
*/
void GME_DirList(const wchar_t* origin, std::vector<std::wstring>* lst)
{
  std::wstring item;
  std::wstring path = origin; path += L"\\*";

  WIN32_FIND_DATAW fdw;
  HANDLE hnd = FindFirstFileW(path.c_str(), &fdw);
  if(hnd != INVALID_HANDLE_VALUE) {
    do {
      if(fdw.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        if(!wcscmp(fdw.cFileName, L".")) continue;
        if(!wcscmp(fdw.cFileName, L"..")) continue;
        item = origin; item += L"\\"; item += fdw.cFileName;
        lst->push_back(item);
      }
    } while(FindNextFileW(hnd, &fdw));
  }
  FindClose(hnd);
}

/*
  function to recursively list all files in folder and sub-folders.
*/
void GME_FileListRecursive(const wchar_t* origin, std::vector<std::wstring>* lst, const wchar_t* filter)
{
  GME_FileList(origin, lst, filter);
  std::vector<std::wstring> subdir;
  GME_DirList(origin, &subdir);
  for(unsigned i = 0; i < subdir.size(); i++) {
    GME_FileListRecursive(subdir[i].c_str(), lst, filter);
  }
}

/*
  function to test if item at specified path is a file
*/
bool GME_IsFile(const std::wstring& path)
{
  DWORD attr = GetFileAttributesW(path.c_str());
  return ((attr != INVALID_FILE_ATTRIBUTES) && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

/*
  function to test if item at specified path is a directory
*/
bool GME_IsDir(const std::wstring& path)
{
  //PathIsDirectory(path.c_str());
  DWORD attr = GetFileAttributesW(path.c_str());
  return ((attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY));
}

/*
  function to delete a folder + content recursively (rm -R)
*/
bool GME_DirRemRecursive(const std::wstring& path)
{
  wchar_t buffer[262];
  wcscpy(buffer, path.c_str());
  buffer[path.size()+1] = 0;

  SHFILEOPSTRUCTW fop;
  memset(&fop, 0, sizeof(SHFILEOPSTRUCTW));
  fop.pFrom = buffer;
  fop.wFunc = FO_DELETE;
  fop.fFlags = FOF_NO_UI;

  return SHFileOperationW(&fop);
}


/*
  custom generic function to get file size.
*/
size_t GME_FileSize(const std::wstring& src)
{
  size_t s = -1;

  FILE* fr = _wfopen(src.c_str(), L"rb");

  if(fr) {
    fseek(fr, 0, SEEK_END);
    s = ftell(fr);
    fclose(fr);
  }

  return s;
}

/*
  custom generic function to read files.
*/
bool GME_FileRead(ubyte* data, size_t size, const std::wstring& src)
{
  FILE* fr = _wfopen(src.c_str(), L"rb");

  if(fr == NULL)
    return false;

  fread(data, 1, size, fr);

  fclose(fr);

  return true;
}


/*
  custom generic function to write files.
  Note: this function is 10x faster than regular way for large files.
*/
bool GME_FileWrite(const ubyte* data, size_t size, const std::wstring& dst, bool overwrite)
{
  ubyte buff[8192];

  if(!overwrite) {
    if(GetFileAttributesW(dst.c_str()) != INVALID_FILE_ATTRIBUTES)
      return true; /* we do not write, but this is not a error */
  }

  FILE* fw = _wfopen(dst.c_str(), L"wb");

  if(fw == NULL) {
    GME_Logs(GME_LOG_ERROR, "GME_FileWrite", "Unable to open file for writing", GME_StrToMbs(dst).c_str());
    return false;
  }

  while(size > sizeof(buff)) {
    memcpy(buff, data, sizeof(buff));
    data += sizeof(buff);
    size -= sizeof(buff);
    if(!fwrite(buff, sizeof(buff), 1, fw)) {
      fclose(fw);
      GME_Logs(GME_LOG_ERROR, "GME_FileWrite", "Write error", GME_StrToMbs(dst).c_str());
      return false;
    }
  }
  memcpy(buff, data, size);
  if(!fwrite(buff, size, 1, fw)) {
    fclose(fw);
    GME_Logs(GME_LOG_ERROR, "GME_FileWrite", "Write error", GME_StrToMbs(dst).c_str());
    return false;
  }

  fclose(fw);
  return true;
}

/*
  custom generic function to copy files.
*/
bool GME_FileCopy(const std::wstring& src, const std::wstring& dst, bool overwrite)
{
  ubyte buff[8192];
  size_t n;

  if(!overwrite) {
    if(GetFileAttributesW(dst.c_str()) != INVALID_FILE_ATTRIBUTES)
      return true; /* we do not write, but this is not a error */
  }

  FILE* fr = _wfopen(src.c_str(), L"rb");
  FILE* fw = _wfopen(dst.c_str(), L"wb");

  if(fr == NULL || fw == NULL) {
    if(fr == NULL) GME_Logs(GME_LOG_ERROR, "GME_FileCopy", "Unable to open file for reading", GME_StrToMbs(src).c_str());
    if(fw == NULL) GME_Logs(GME_LOG_ERROR, "GME_FileCopy", "Unable to open file for writing", GME_StrToMbs(dst).c_str());
    if(fr) fclose(fr);
    if(fw) fclose(fw);
    return false;
  }

  while(0 < (n = fread(buff, 1, sizeof(buff), fr))) {
    if(!fwrite(buff, n, 1, fw)) {
      GME_Logs(GME_LOG_ERROR, "GME_FileCopy", "Write error", GME_StrToMbs(dst).c_str());
      fclose(fr);
      fclose(fw);
      return false;
    }
  }

  fclose(fr);
  fclose(fw);

  return true;
}

/*
  function to get the ASCII content of a simple txt file
*/
size_t GME_FileGetAsciiContent(const std::wstring& path, std::wstring* content)
{

  size_t r = 0;
  long fs = 0;

  FILE* fp = _wfopen(path.c_str(), L"rb");
  if(fp) {
    fseek(fp, 0, SEEK_END);
    fs = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char* buffer = new char[fs+1];
    r += fread(buffer, 1, fs, fp);
    buffer[fs] = 0;
    *content = GME_StrToWcs(buffer);
    delete [] buffer;
    fclose(fp);
  }
  return r;
}

/* ----------------------------- Dialog boxes related toolkit ---------------------------------- */

/*
  function to display an error dialog box
*/
void GME_DialogError(HWND hwnd, std::wstring message)
{
  MessageBoxW(hwnd, message.c_str(), L"OvGME Error", MB_OK|MB_ICONERROR);
}

/*
  function to display a warning dialog box
*/
void GME_DialogWarning(HWND hwnd, std::wstring message)
{
  MessageBoxW(hwnd, message.c_str(), L"OvGME Warning", MB_OK|MB_ICONWARNING);
}

/*
  function to display a confirmation dialog box
*/
int GME_DialogWarningConfirm(HWND hwnd, std::wstring message)
{
  return MessageBoxW(hwnd, message.c_str(), L"OvGME Warning", MB_OKCANCEL|MB_ICONWARNING);
}

/*
  function to display a confirmation dialog box
*/
int GME_DialogQuestionConfirm(HWND hwnd, std::wstring message)
{
  return MessageBoxW(hwnd, message.c_str(), L"OvGME Choose Your Destiny", MB_OKCANCEL|MB_ICONQUESTION);
}

/*
  function to display a information dialog box
*/
void GME_DialogInfo(HWND hwnd, std::wstring message)
{
  MessageBoxW(hwnd, message.c_str(), L"OvGME Notice", MB_OK|MB_ICONINFORMATION);
}

/*
  function for folder path chooser dialog
*/
void GME_DialogDirChooser(HWND hwnd, wchar_t* path, size_t max)
{
  memset(path, 0, max);

  BROWSEINFOW* pBrwsinfo = NULL;
  pBrwsinfo = new BROWSEINFOW;
  memset(pBrwsinfo, 0, sizeof(BROWSEINFO));
  pBrwsinfo->hwndOwner = hwnd;
  pBrwsinfo->pidlRoot = NULL;
  pBrwsinfo->pszDisplayName = NULL;
  pBrwsinfo->lpszTitle = L"Choose a folder";
  pBrwsinfo->ulFlags = 0;
  pBrwsinfo->lpfn = NULL;
  pBrwsinfo->lParam = 0;

  ITEMIDLIST*  pItem;
  pItem = SHBrowseForFolderW(pBrwsinfo);
  if(pItem) SHGetPathFromIDListW(pItem, path);

  delete [] pBrwsinfo;
}

/*
  function for file(s) chooser dialog
*/
bool GME_DialogFileOpen(HWND hwnd, wchar_t* path, size_t max, unsigned* path_offset, const wchar_t* filter, const wchar_t* title)
{
  memset(path, 0, max);

  OPENFILENAMEW* pOpenfilename = NULL;
  pOpenfilename = new OPENFILENAMEW;
  memset(pOpenfilename, 0, sizeof(OPENFILENAMEW));
  pOpenfilename->lStructSize = sizeof(OPENFILENAMEW);

  pOpenfilename->hwndOwner = hwnd;
  pOpenfilename->lpstrFilter = filter; //L"Mod archive (*.zip)\0*.ZIP;\0";
  pOpenfilename->nMaxCustFilter = 0;
  pOpenfilename->nFilterIndex = 0;

  pOpenfilename->lpstrFile = path;
  pOpenfilename->lpstrFile[0] = '\0';
  pOpenfilename->nMaxFile = max;

  pOpenfilename->lpstrFileTitle = NULL;
  pOpenfilename->nMaxFileTitle = 0;
  pOpenfilename->lpstrInitialDir = NULL;
  pOpenfilename->lpstrTitle = title;
  pOpenfilename->Flags = OFN_EXPLORER|OFN_ALLOWMULTISELECT|OFN_NONETWORKBUTTON|OFN_NOTESTFILECREATE|OFN_DONTADDTORECENT;
  pOpenfilename->nFileOffset = 0;
  pOpenfilename->nFileExtension = 0;
  pOpenfilename->lpstrDefExt = NULL;
  pOpenfilename->lCustData = 0;
  pOpenfilename->lpfnHook = NULL;
  pOpenfilename->lpTemplateName = NULL;
  pOpenfilename->pvReserved = NULL;
  pOpenfilename->dwReserved = 0;
  pOpenfilename->FlagsEx = 0;

  bool result = GetOpenFileNameW(pOpenfilename);

  *path_offset = pOpenfilename->nFileOffset;

  delete [] pOpenfilename;

  return result;
}


/*
  function for file(s) chooser dialog
*/
bool GME_DialogFileSave(HWND hwnd, wchar_t* path, size_t max, unsigned* path_offset, const wchar_t* ext, const wchar_t* filter, const wchar_t* title)
{
  memset(path, 0, max);

  OPENFILENAMEW* pOpenfilename = NULL;
  pOpenfilename = new OPENFILENAMEW;
  memset(pOpenfilename, 0, sizeof(OPENFILENAMEW));
  pOpenfilename->lStructSize = sizeof(OPENFILENAMEW);

  pOpenfilename->hwndOwner = hwnd;
  pOpenfilename->lpstrFilter = filter; //L"EXT file (*.ext)\0*.EXT;\0";
  pOpenfilename->nMaxCustFilter = 0;
  pOpenfilename->nFilterIndex = 0;

  pOpenfilename->lpstrFile = path;
  pOpenfilename->lpstrFile[0] = '\0';
  pOpenfilename->nMaxFile = max;

  pOpenfilename->lpstrFileTitle = NULL;
  pOpenfilename->nMaxFileTitle = 0;
  pOpenfilename->lpstrInitialDir = NULL;
  pOpenfilename->lpstrTitle = title;
  pOpenfilename->Flags = OFN_EXPLORER|OFN_NONETWORKBUTTON|OFN_DONTADDTORECENT;
  pOpenfilename->nFileOffset = 0;
  pOpenfilename->nFileExtension = 0;
  pOpenfilename->lpstrDefExt = ext;
  pOpenfilename->lCustData = 0;
  pOpenfilename->lpfnHook = NULL;
  pOpenfilename->lpTemplateName = NULL;
  pOpenfilename->pvReserved = NULL;
  pOpenfilename->dwReserved = 0;
  pOpenfilename->FlagsEx = 0;

  bool result = GetSaveFileNameW(pOpenfilename);

  *path_offset = pOpenfilename->nFileOffset;

  delete [] pOpenfilename;

  return result;
}


/* ------------------------------ Zip archive related toolkit ---------------------------------- */

/*
  function to test if file at specified path is a valid zip archive
*/
bool GME_IsZip(const std::wstring& zip)
{
  mz_zip_archive za; // Zip archive struct
  memset(&za, 0, sizeof(mz_zip_archive));
  if(!mz_zip_reader_init_file(&za, GME_StrToMbs(zip).c_str(), 0)) {
    return false;
  }
  mz_zip_reader_end(&za);
  return true;
}


/*
  function to test if a zip file is a valid mod archive
*/
bool GME_ZipIsValidMod(const std::wstring& zip)
{
  mz_zip_archive za; // Zip archive struct
  mz_zip_archive_file_stat zf; // zip file stat struct

  std::string zip_name = GME_StrToMbs(zip);
  memset(&za, 0, sizeof(mz_zip_archive));
  if(!mz_zip_reader_init_file(&za, zip_name.c_str(), 0)) {
    return false;
  }

  std::string dir_name;
  std::string mod_name = GME_FilePathToName(zip_name);

  unsigned c = mz_zip_reader_get_num_files(&za);
  for(unsigned i = 0; i < c; i++) {
    if(mz_zip_reader_is_file_a_directory(&za, i)) {
      if(!mz_zip_reader_file_stat(&za, i, &zf)){
        mz_zip_reader_end(&za);
        return false;
      }
      dir_name = zf.m_filename;
      dir_name.erase(dir_name.size()-1, 1); /* remove the final '/' */
      if(mod_name == dir_name) {
        mz_zip_reader_end(&za);
        return true;
      }
    }
  }
  mz_zip_reader_end(&za);
  return false;
}


/*
  function to get description data in a valid mod archive
*/
bool GME_ZipGetModDesc(const std::wstring& zip, std::wstring* desc)
{
  std::string zip_name = GME_StrToMbs(zip);
  std::string mod_name = GME_FilePathToName(zip_name);
  std::vector<std::string> txt_name;

  /* standard supported filename for description */
  txt_name.push_back(mod_name + ".txt");
  txt_name.push_back("description.txt");
  txt_name.push_back("readme.txt");

  /* to discard case sensitive */
  std::string cap_name;
  for(unsigned i = 0; i < txt_name.size(); i++) {
    GME_StrToUpper(txt_name[i]);
  }

  mz_zip_archive za; // Zip archive struct
  mz_zip_archive_file_stat zf; // Zip file stat struct;

  memset(&za, 0, sizeof(mz_zip_archive));
  if(!mz_zip_reader_init_file(&za, zip_name.c_str(), 0)) {
    return false;
  }
  unsigned c = mz_zip_reader_get_num_files(&za);
  for(unsigned i = 0; i < c; i++) {
    if(!mz_zip_reader_is_file_a_directory(&za, i)) {
      if(!mz_zip_reader_file_stat(&za, i, &zf)){
        mz_zip_reader_end(&za);
        return false;
      }
      cap_name = GME_StrToUpper(zf.m_filename);
      /* text for compatibles filenames */
      for(unsigned k = 0; k < txt_name.size(); k++) {
        if(cap_name == txt_name[k]) {
          char* buffer =  new char[zf.m_uncomp_size+1];
          if(!mz_zip_reader_extract_to_mem(&za, i, buffer, zf.m_uncomp_size, 0)) {
            mz_zip_reader_end(&za);
            delete [] buffer;
            return false;
          }
          buffer[zf.m_uncomp_size] = 0;
          *desc = GME_StrToWcs(buffer);
          delete [] buffer;
          mz_zip_reader_end(&za);
          return true;
        }
      }
    }
  }
  mz_zip_reader_end(&za);
  return false;
}

/*
  function to get version data in a valid mod archive
*/
bool GME_ZipGetModVers(const std::wstring& zip, std::wstring* vers)
{
  std::string zip_name = GME_StrToMbs(zip);
  std::string mod_name = GME_FilePathToName(zip_name);
  std::vector<std::string> txt_name;

  /* standard supported filename for description */
  txt_name.push_back(mod_name + ".ver");
  txt_name.push_back("version.txt");
  txt_name.push_back("ver.txt");

  /* to discard case sensitive */
  std::string cap_name;
  for(unsigned i = 0; i < txt_name.size(); i++) {
    GME_StrToUpper(txt_name[i]);
  }

  mz_zip_archive za; // Zip archive struct
  mz_zip_archive_file_stat zf; // Zip file stat struct;

  memset(&za, 0, sizeof(mz_zip_archive));
  if(!mz_zip_reader_init_file(&za, zip_name.c_str(), 0)) {
    return false;
  }
  unsigned c = mz_zip_reader_get_num_files(&za);
  for(unsigned i = 0; i < c; i++) {
    if(!mz_zip_reader_is_file_a_directory(&za, i)) {
      if(!mz_zip_reader_file_stat(&za, i, &zf)){
        mz_zip_reader_end(&za);
        return false;
      }
      cap_name = GME_StrToUpper(zf.m_filename);
      /* text for compatibles filenames */
      for(unsigned k = 0; k < txt_name.size(); k++) {
        if(cap_name == txt_name[k]) {
          char* buffer =  new char[zf.m_uncomp_size+1];
          if(!mz_zip_reader_extract_to_mem(&za, i, buffer, zf.m_uncomp_size, 0)) {
            mz_zip_reader_end(&za);
            delete [] buffer;
            return false;
          }
          buffer[zf.m_uncomp_size] = 0;
          *vers = GME_StrToWcs(buffer);
          delete [] buffer;
          mz_zip_reader_end(&za);
          return true;
        }
      }
    }
  }
  mz_zip_reader_end(&za);
  return false;
}

/* ------------------------------- Node tree related toolkit ----------------------------------- */

/*
  function to create node tree from file tree, without loading data (this
  function is recursive)
*/
void GME_TreeBuildFromDir(GMEnode* node, const std::wstring& base_dir)
{
  std::wstring srch_path = base_dir + L"\\" + node->getPath() + L"\\*";
  WIN32_FIND_DATAW fdw;
  HANDLE hnd = FindFirstFileW(srch_path.c_str(), &fdw);
  if(hnd != INVALID_HANDLE_VALUE) {
    do {
      if(fdw.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        if(!wcscmp(fdw.cFileName, L".")) continue;
        if(!wcscmp(fdw.cFileName, L"..")) continue;
        GMEnode* child = new GMEnode(fdw.cFileName, true);
        child->setSource(base_dir + L"\\" + node->getPath() + L"\\" + fdw.cFileName);
        child->setParent(node);
        GME_TreeBuildFromDir(child, base_dir); // recurse
      } else {
        GMEnode* child = new GMEnode(fdw.cFileName, false);
        child->setSource(base_dir + L"\\" + node->getPath() + L"\\" + fdw.cFileName);
        child->setParent(node);
      }
    } while(FindNextFileW(hnd, &fdw));
  }
  FindClose(hnd);
}


/*
  function to create node tree from zip file, without loading data.
*/
bool GME_TreeBuildFromZip(GMEnode* root, const std::wstring& zip)
{
  mz_zip_archive za; // Zip archive struct
  mz_zip_archive_file_stat zf; // zip file stat struct

  memset(&za, 0, sizeof(mz_zip_archive));
  if(!mz_zip_reader_init_file(&za, GME_StrToMbs(zip).c_str(), 0)) {
    return false;
  }

  GMEnode* child;
  GMEnode* parent;
  std::vector<std::wstring> path;

  unsigned c = mz_zip_reader_get_num_files(&za);
  for(unsigned i = 0; i < c; i++) {

    if(!mz_zip_reader_file_stat(&za, i, &zf)){
      mz_zip_reader_end(&za);
      return false;
    }

    /* get splited path elements */
    GME_StrSplit(zf.m_filename, &path, "/");

    /* create tree and/or go in depth, we stops before the last
      path element, which should be eiter a file or a folder */

    parent = root; /* we begin at root node */
    for(unsigned k = 0; k < path.size()-1; k++) {
      if(parent->hasChild(path[k])) {
        // go deep inside the tree
        parent = parent->getChild(path[k]);
      } else {
        // create a new child node
        GMEnode* child = new GMEnode(path[k], true);
        child->setId(i);
        child->setParent(parent);
        parent = child;
      }
    }

    /* now rest the last path element, either a file or folder */
    if(mz_zip_reader_is_file_a_directory(&za, i)) {
      child = new GMEnode(path[path.size()-1], true);
      child->setId(i);
      child->setParent(parent);
    } else {
      child = new GMEnode(path[path.size()-1], false);
      child->setId(i);
      child->setParent(parent);
    }
  }
  mz_zip_reader_end(&za);
  return true;
}

/* -------------------------------- Checksum related toolkit ----------------------------------- */

/*
  function to create md5sum string from an other string
*/
std::wstring GME_Md5(const std::wstring& str)
{
  char md5[33];

  HCRYPTPROV hProv = 0;
  HCRYPTHASH hHash = 0;
  unsigned char rgbHash[16];
  DWORD cbHash = 16;
  char rgbDigits[] = "0123456789abcdef";

  std::string rgbStr = GME_StrToMbs(str);

  CryptAcquireContext(&hProv,NULL,NULL,PROV_RSA_FULL,CRYPT_VERIFYCONTEXT);
  CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash);
  CryptHashData(hHash, (unsigned char*)rgbStr.c_str(), rgbStr.size(), 0);
  CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0);

  unsigned j = 0;
  for(unsigned i = 0; i < cbHash; i++){
    md5[j] = rgbDigits[rgbHash[i] >> 4]; j++;
    md5[j] = rgbDigits[rgbHash[i] & 0xf]; j++;
  }
  md5[j] = 0;

  CryptReleaseContext(hProv, 0);
  CryptDestroyHash(hHash);

  wchar_t wmd5[33];
  memset(wmd5, 0, sizeof(wchar_t)*33);
  mbstowcs(wmd5, md5, 32);
  return std::wstring(wmd5);
}


/*
  CRC table for fast CRC32 function.
 */
static unsigned crc32_tab[] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3,	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de,	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,	0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5,	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,	0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940,	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,	0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

/*
  fast CRC32 implementation, found here:
  http://opensource.apple.com//source/xnu/xnu-1456.1.26/bsd/libkern/crc32.c
 */
unsigned GME_crc32(unsigned crc, const ubyte *buf, size_t size)
{
	const ubyte *p;

	p = buf;
	crc = crc ^ ~0U;

	while(size--)
		crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);

	return crc ^ ~0U;
}

/*
  function to get xxHash32 hash of a file. This is the fastest
  way i found... 0.85sc for 300Mio of data. xxHash is also much
  faster than Crc32...
*/
unsigned GME_FileGetXxH32(const std::wstring& src)
{
  XXH32_hash_t xxh = 0;

  ubyte* buff;
  size_t s;

  FILE* fr = _wfopen(src.c_str(), L"rb");


  if(fr) {
    fseek(fr, 0, SEEK_END);
    s = ftell(fr);
    fseek(fr, 0, SEEK_SET);

    buff = new ubyte[s];
    fread(buff, 1, s, fr);
    fclose(fr);

    xxh = XXH32(buff, s, 0);
    delete [] buff;
  }

  return xxh;
}

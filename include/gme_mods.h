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

#ifndef GME_MODS_H_INCLUDED
#define GME_MODS_H_INCLUDED

#include "gme.h"

/* macros for mod management */
#define MODS_ENABLE 1
#define MODS_DISABLE 0
#define MODS_TOGGLE -1

void GME_ModsUndoMod(HWND hpb, const std::wstring& name);
void GME_ModsApplyMod(HWND hpb, const std::wstring& name, int type);
void GME_ModsRestoreMod(HWND hpb, const std::wstring& name);
void GME_ModsCleanBackup();
bool GME_ModsToggleSel(int action);
bool GME_ModsToggleAll(int action);
bool GME_ModsImport();
bool GME_ModsChkDesc();
void GME_ModsExploreCur();
void GME_ModsDeleteCur();
void GME_ModsListQuickEnable(const std::wstring& name, bool enable);
bool GME_ModsUpdList();
void GME_ModsUninstall();
void GME_ModsMakeArchive(const std::wstring& src_dir, const std::wstring& dst_path, const std::wstring& desc, int vmaj, int vmin, int vrev, int zlevel=MZ_BEST_COMPRESSION);
void GME_ModsMakeArchiveCur(const std::wstring& desc, int vmaj, int vmin, int vrev, int zlevel=MZ_BEST_COMPRESSION);
void GME_ModsMakeCancel();
void GME_ModsProc_PushApply(const std::wstring& name, int type);
void GME_ModsProc_PushRestore(const std::wstring& name);
bool GME_ModsProc_IsReady();
void GME_ModsProc_Launch();
void GME_ModsProcCancel();
bool GME_ModsListIsEmpty();
void GME_ModsClean();

#endif // GME_MODS_H_INCLUDED

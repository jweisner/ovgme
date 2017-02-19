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

#ifndef GME_PROF_H_INCLUDED
#define GME_PROF_H_INCLUDED

void GME_ProfUpdList();
void GME_ProfUpdMenu();
void GME_ProfEnaMenu(bool enable);
unsigned GME_ProfGetCount();
std::wstring GME_ProfGetPath(unsigned id);
std::wstring GME_ProfGetName(unsigned id);
bool GME_ProfCreate(const wchar_t* name);
bool GME_ProfApply(unsigned mid);
bool GME_ProfDelete();

#endif // GME_PROF_H_INCLUDED

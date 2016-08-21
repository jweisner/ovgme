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
#ifndef GME_LOGS_H_INCLUDED
#define GME_LOGS_H_INCLUDED

#define GME_LOG_FATAL 0
#define GME_LOG_ERROR 1
#define GME_LOG_WARNING 2
#define GME_LOG_NOTICE 3

void GME_LogsInit();
void GME_Logs(int level, const char* scope, const char* msg, const char* item);

#endif // GME_LOGS_H_INCLUDED

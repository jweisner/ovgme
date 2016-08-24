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

#ifndef GME_NETW_H_INCLUDED
#define GME_NETW_H_INCLUDED

#include <gme.h>

#define GME_HTTPGET_ERR_DNS 1
#define GME_HTTPGET_ERR_CNX 2
#define GME_HTTPGET_ERR_ENC 3
#define GME_HTTPGET_ERR_BAL 4
#define GME_HTTPGET_ERR_REC 5
#define GME_HTTPGET_ERR_FOP 6
#define GME_HTTPGET_ERR_FWR 7

bool GME_NetwIsUrl(const char* str);
std::string GME_NetwEncodeUrl(const char* url);
std::string GME_NetwEncodeUrl(const std::string& url);

typedef void(*GME_NetwGETOnErr)(const char* url);
typedef bool(*GME_NetwGETOnDnl)(unsigned percent, unsigned rate);
typedef void(*GME_NetwGETOnEnd)(const char* body, size_t body_size);
typedef void(*GME_NetwGETOnSav)(const wchar_t* path);

int GME_NetwHttpGET(const char* url, const GME_NetwGETOnErr, const GME_NetwGETOnDnl, const GME_NetwGETOnEnd);
int GME_NetwHttpGET(const char* url, const GME_NetwGETOnErr, const GME_NetwGETOnDnl, const GME_NetwGETOnSav, const std::wstring& path);

#endif // GME_NETW_H_INCLUDED

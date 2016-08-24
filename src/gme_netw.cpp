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
#include "gme_netw.h"
#include "gme_logs.h"
#include "pugixml.hpp"

/* structure for common url */
struct GME_Url_Struct
{
  char prot[8];
  char host[256];
  char port[64];
  char path[256];
  char file[256];
};

/* http response header structure */
struct GME_Http_Head_Struct
{
  size_t size;
  short code;
  char location[256];
  char content_type[64];
  size_t content_length;
  char transfer_encoding[64];
};


/*
  function to encode url from string
*/
std::string GME_NetwEncodeUrl(const char* url)
{
  /*
    adapted from that: http://www.geekhideout.com/urlcode.shtml
    thanks to the author....
  */

  char hex[] = "0123456789abcdef";

  const char* pstr = url;
  char* buf = new char[strlen(url) * 3 + 1];
  char* pbuf = buf;

  while(*pstr) {

    if(isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~' || *pstr == '/' || *pstr == ':' || *pstr == '?' || *pstr == '%') {
      *pbuf++ = *pstr;
    } else {
      *pbuf++ = '%';
      *pbuf++ = hex[(*pstr >> 4) & 15];
      *pbuf++ = hex[*pstr & 15];
    }

    pstr++;
  }
  *pbuf = '\0';

  std::string encoded = buf;
  delete[] buf;

  return encoded;
}


/*
  function to encode url from string
*/
std::string GME_NetwEncodeUrl(const std::string& url)
{
  /*
    adapted from that: http://www.geekhideout.com/urlcode.shtml
    thanks to the author....
  */

  char hex[] = "0123456789abcdef";

  const char* pstr = url.c_str();
  char* buf = new char[url.size() * 3 + 1];
  char* pbuf = buf;

  while(*pstr) {

    if(isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~' || *pstr == '/' || *pstr == ':' || *pstr == '?' || *pstr == '%') {
      *pbuf++ = *pstr;
    } else {
      *pbuf++ = '%';
      *pbuf++ = hex[(*pstr >> 4) & 15];
      *pbuf++ = hex[*pstr & 15];
    }

    pstr++;
  }
  *pbuf = '\0';

  std::string encoded = buf;
  delete[] buf;

  return encoded;
}


/*
  function to decode url to string
*/
std::string GME_NetwDecodeUrl(const char* url)
{
  /*
    adapted from that: http://www.geekhideout.com/urlcode.shtml
    thanks to the author....
  */

  const char *pstr = url;
  char* buf = new char[strlen(url) + 1];
  char* pbuf = buf;
  char t1, t2;

  while(*pstr) {

    if(*pstr == '%') {
      if(pstr[1] && pstr[2]) {
        t1 = isdigit(pstr[1]) ? pstr[1]-'0' : tolower(pstr[1])-'a'+10;
        t2 = isdigit(pstr[2]) ? pstr[2]-'0' : tolower(pstr[2])-'a'+10;
        *pbuf++ = t1 << 4 | t2;
        pstr += 2;
      }
    } else {
      *pbuf++ = *pstr;
    }
    pstr++;
  }
  *pbuf = '\0';

  std::string decoded = buf;
  delete[] buf;

  return decoded;
}


/*
  function to decode url to string
*/
std::string GME_NetwDecodeUrl(const std::string& url)
{
  /*
    adapted from that: http://www.geekhideout.com/urlcode.shtml
    thanks to the author....
  */

  const char *pstr = url.c_str();
  char* buf = new char[url.size() + 1];
  char* pbuf = buf;
  char t1, t2;

  while(*pstr) {

    if(*pstr == '%') {
      if(pstr[1] && pstr[2]) {
        t1 = isdigit(pstr[1]) ? pstr[1]-'0' : tolower(pstr[1])-'a'+10;
        t2 = isdigit(pstr[2]) ? pstr[2]-'0' : tolower(pstr[2])-'a'+10;
        *pbuf++ = t1 << 4 | t2;
        pstr += 2;
      }
    } else {
      *pbuf++ = *pstr;
    }
    pstr++;
  }
  *pbuf = '\0';

  std::string decoded = buf;
  delete[] buf;

  return decoded;
}

/*
  function to parse an url string
*/
GME_Url_Struct GME_NetwParseUrl(const char* url_str)
{
   GME_Url_Struct url;

  std::string str = GME_NetwEncodeUrl(url_str);

  size_t prot_p, host_p, port_p, path_p, file_p;
  std::string prot, host, path, port, file;

  prot_p = str.find("://", 0);
  if(prot_p == std::string::npos) {
    host_p = 0;
    port_p = str.find_first_of(":", host_p);
    path_p = str.find_first_of("/", host_p);
    file_p = str.find_last_of("/", -1);
  } else {
    host_p = prot_p + 3;
    port_p = str.find_first_of(":", host_p);
    path_p = str.find_first_of("/", host_p);
    file_p = str.find_last_of("/", -1);
  }

  memset(&url, 0, sizeof(url));

  if(prot_p != std::string::npos) {
    prot = str.substr(0, prot_p);
    GME_StrToLower(prot);
  }

  if(port_p != std::string::npos) {
    host = str.substr(host_p, port_p-host_p);
    port = str.substr(port_p+1, (path_p-port_p)-1);
  } else {
    host = str.substr(host_p, path_p-host_p);
  }

  if(path_p != std::string::npos) {
    path = str.substr(path_p, str.size());
  } else {
    path = "/";
  }

  if(file_p != std::string::npos) {
    file = str.substr(file_p+1, str.size());
  }

  /* try to deduct port from protocol */
  if(port.size() == 0 && prot.size()) {
    if(prot == "http") port = "80";
    if(prot == "ftp") port = "22";
  }

  /* try to deduct protocol from port */
  if(prot.size() == 0 && port.size()) {
    if(port == "80") prot = "http";
    if(port == "22") prot = "ftp";
  }

  /* set to default if nothing was deducted */
  if(prot.size() == 0) prot = "http";
  if(port.size() == 0) port = "80";

  strcpy(url.prot, prot.c_str());
  strcpy(url.host, host.c_str());
  strcpy(url.port, port.c_str());
  strcpy(url.path, path.c_str());
  if(file.size()) strcpy(url.file, file.c_str());

  return url;
}


/*
  function to check if string appear as valid url
*/
bool GME_NetwIsUrl(const char* str)
{
  GME_Url_Struct url = GME_NetwParseUrl(str);
  if(!strlen(url.host)) return false;
  if(!strchr(url.host, '.')) return false;
  return true;
}


/*
  function to get sockaddr from host and port string
*/
bool GME_NetwGetIp4(const char* host, const char* port, sockaddr* saddr)
{
  /* check if we got an fqdn name or an ip string */
  addrinfo* result = NULL;
  int error  = getaddrinfo(host, port, NULL, &result);
  if(0 != error) {
    GME_Logs(GME_LOG_WARNING, "GME_NetwGetIp4", "Unable to resolve host", host);
    return false;
  }
  memcpy(saddr, result->ai_addr, sizeof(sockaddr));
  freeaddrinfo(result);
  return true;
}

/*
  function to open a new socket and connection
*/
SOCKET GME_NetwConnect(sockaddr* saddr)
{
  SOCKET sock;

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if(sock == INVALID_SOCKET) {
    GME_Logs(GME_LOG_ERROR, "GME_NetwConnect", "Unable to open new socket", "== INVALID_SOCKET");
    return INVALID_SOCKET;
  }

  if(0 != connect(sock, saddr, sizeof(sockaddr))) {
    GME_Logs(GME_LOG_ERROR, "GME_NetwConnect", "Unable to connect to host", "");
    closesocket(sock);
    return INVALID_SOCKET;
  }

  return sock;
}


/*
  function to parse HTTP response header
*/
GME_Http_Head_Struct GME_NetwHttpParseHead(const char* recv_buff, size_t recv_size)
{
  GME_Http_Head_Struct header;
  std::vector<std::string> head_entry;

  /* copy in a new buffer to make a string */
  char *cstr;
  try {
    cstr = new char[recv_size+1];
  } catch (const std::bad_alloc&) {
    GME_Logs(GME_LOG_ERROR, "GME_NetwHttpParseHead", "Bad alloc", std::to_string(recv_size+1).c_str());
    return header;
  }
  memcpy(cstr, recv_buff, recv_size);
  cstr[recv_size] = 0;
  std::string recv_str = cstr;
  delete[] cstr;

  memset(&header, 0, sizeof(GME_Http_Head_Struct));

  /* get the status code... it is after "HTTP/1.1 " */
  header.code = strtol(recv_str.substr(9, 3).c_str(), NULL, 10);
  if(header.code == 0L) {
    /* we got a problem... */
    GME_Logs(GME_LOG_ERROR, "GME_NetwHttpParseHead", "Invalid response code", "0L");
    return header;
  }

  /* find the size of the header, we check the double CRLF */
  header.size = recv_str.find("\r\n\r\n");
  if(header.size == std::string::npos) {
    /* we got a problem... */
    GME_Logs(GME_LOG_ERROR, "GME_NetwHttpParseHead", "Unable to get header size", "CRLF missing ?");
    header.size = 0;
    return header;
  }

  /* split header into substrings */
  GME_StrSplit(recv_str.substr(0, header.size), &head_entry, "\n");

  /* search for "Content-Length:" header and parse */
  for(unsigned i = 0; i < head_entry.size(); i++) {
    if(head_entry[i].find("Content-Length") != std::string::npos) {
      size_t p = head_entry[i].find_first_of(':', 0) + 2;
      size_t s = head_entry[i].find_first_of('\r', 0) - p;
      header.content_length = strtol(head_entry[i].substr(p, s).c_str(), NULL, 10);
    }
  }

  /* check if we got a redirection */
  if(header.code == 301 || header.code == 302) {

    /* search for "Location:" header and parse */
    for(unsigned i = 0; i < head_entry.size(); i++) {
      if(head_entry[i].find("Location") != std::string::npos) {
        size_t p = head_entry[i].find_first_of(':', 0) + 2;
        size_t s = head_entry[i].find_first_of('\r', 0) - p;
        strcpy(header.location, head_entry[i].substr(p, s).c_str());
      }
    }
    return header;
  }

  /* search for "Content-Type:" header and parse */
  for(unsigned i = 0; i < head_entry.size(); i++) {
    if(head_entry[i].find("Content-Type") != std::string::npos) {
      size_t p = head_entry[i].find_first_of(':', 0) + 2;
      size_t s = head_entry[i].find_first_of('\r', 0) - p;
      strcpy(header.content_type, head_entry[i].substr(p, s).c_str());
    }
  }

  /* search for "Transfer-Encoding:" header and parse */
  for(unsigned i = 0; i < head_entry.size(); i++) {
    if(head_entry[i].find("Transfer-Encoding") != std::string::npos) {
      size_t p = head_entry[i].find_first_of(':', 0) + 2;
      size_t s = head_entry[i].find_first_of('\r', 0) - p;
      strcpy(header.transfer_encoding, head_entry[i].substr(p, s).c_str());
    }
  }

  return header;
}

/*
  function to send GET Http request to a server.
*/
int GME_NetwHttpGET(const char* url_str, const GME_NetwGETOnErr on_err, const GME_NetwGETOnDnl on_dnl, const GME_NetwGETOnEnd on_end)
{
  WSADATA wsa;
  WSAStartup(MAKEWORD(2,2),&wsa);

  /* parse url */
  GME_Url_Struct url = GME_NetwParseUrl(url_str);

  /* get host address for connection infos */
  sockaddr saddr;
  if(!GME_NetwGetIp4(url.host, url.port, &saddr)) {
    WSACleanup();
    if(on_err) on_err(url_str);
    GME_Logs(GME_LOG_ERROR, "GME_NetwHttpGET", "GME_NetwGetIp4 failed", url_str);
    return GME_HTTPGET_ERR_DNS;
  }

  /* open a new connection */
  SOCKET sock;
  if((sock = GME_NetwConnect(&saddr)) == INVALID_SOCKET) {
    WSACleanup();
    if(on_err) on_err(url_str);
    GME_Logs(GME_LOG_ERROR, "GME_NetwHttpGET", "GME_NetwConnect failed", url_str);
    return GME_HTTPGET_ERR_CNX;
  }

  /* create HTTP request */
  char http_req[256];
  if(strlen(url.path)) {
    sprintf(http_req, "GET %s HTTP/1.1\r\nHost: %s \r\n\r\n", url.path, url.host);
  } else {
    sprintf(http_req, "GET / HTTP/1.1\r\nHost: %s \r\n\r\n", url.host);
  }

  /* send request to server */
  send(sock, http_req, strlen(http_req), 0);

  /* stuff to receive data */
  char recv_buff[4096];
  int recv_size;

  /* receiving HTTP response (or not) */
  recv_size = recv(sock, recv_buff, sizeof(recv_buff), 0);
  if(recv_size == SOCKET_ERROR || recv_size == 0) {
    /* stream error... */
    closesocket(sock); WSACleanup();
    if(on_err) on_err(url_str);
    GME_Logs(GME_LOG_ERROR, "GME_NetwHttpGET", "recv failed", url_str);
    return GME_HTTPGET_ERR_REC;
  }

  /* parse HTTP response header */
  GME_Http_Head_Struct header = GME_NetwHttpParseHead(recv_buff, recv_size);

  if(header.code > 302) {
    /* this is a 404 or any HTTP server error... */
    closesocket(sock); WSACleanup();
    if(on_err) on_err(url_str);
    GME_Logs(GME_LOG_ERROR, "GME_NetwHttpGET", "HTTP code greater than 302", url_str);
    return header.code;
  }

  if(header.code == 301 || header.code == 302) {
    /* this is a redirection, we must send a new request */
    closesocket(sock); WSACleanup();
    GME_Logs(GME_LOG_WARNING, "GME_NetwHttpGET", "redirection", url_str);
    return GME_NetwHttpGET(header.location, on_err, on_dnl, on_end);
  }

  if(!header.content_length) {
    /* check if we get a transfer encoding chunked */
    if(strstr(header.transfer_encoding, "chunked")) {

      clock_t t = clock(); /* start clock for download speed */

      int pct, bps;
      unsigned p;
      char chunk_chrs[128];
      size_t chunk_size;
      size_t chunk_recv;
      char* data_p = &recv_buff[header.size+4];
      recv_size -= (header.size+4);

      /* the buffer */
      std::vector<char> body_data;

      // get first chunk size
      for(p = 0; *data_p != '\r'; p++, data_p++, recv_size--) {
        chunk_chrs[p] = *data_p;
      }
      data_p+=2; recv_size-=2; // CRLF
      chunk_chrs[p] = 0;
      chunk_size = strtol(chunk_chrs, NULL, 16);
      chunk_recv = 0;

      do { /* for each chunk */

        if(on_dnl) {
          pct = 0; /* percentage can't be quantified in chunked transfer */
          bps = float(body_data.size())/(float(clock()-t)/CLOCKS_PER_SEC);
          if(!on_dnl(pct, bps)) {
            closesocket(sock); WSACleanup();
            GME_Logs(GME_LOG_NOTICE, "GME_NetwHttpGET", "Cancelled by user", url_str);
            return 0; // cancelled
          }
        }

        do { /* copy chunk data */
          do { /* copy while data available in current buffer */
            body_data.push_back(*data_p);
            chunk_recv++; recv_size--; data_p++;
          } while(recv_size && chunk_recv < chunk_size);

          if(!recv_size) { /* get new data if needed */
            recv_size = recv(sock, recv_buff, sizeof(recv_buff), 0);
            if(recv_size == SOCKET_ERROR || recv_size == 0) {
              /* stream error... */
              closesocket(sock); WSACleanup();
              if(on_err) on_err(url_str);
              GME_Logs(GME_LOG_ERROR, "GME_NetwHttpGET", "recv failed", url_str);
              return GME_HTTPGET_ERR_REC;
            }
            data_p = recv_buff;
          }

        } while(chunk_recv < chunk_size);

        /* begin a new chunk */
        data_p+=2; recv_size-=2; // CRLF

        if(!recv_size) { /* get new data if needed */
          recv_size = recv(sock, recv_buff, sizeof(recv_buff), 0);
          if(recv_size == SOCKET_ERROR || recv_size == 0) {
            /* stream error... */
            closesocket(sock); WSACleanup();
            if(on_err) on_err(url_str);
            GME_Logs(GME_LOG_ERROR, "GME_NetwHttpGET", "recv failed", url_str);
            return GME_HTTPGET_ERR_REC;
          }
          data_p = recv_buff;
        }

        // get chunk size
        for(p = 0; *data_p != '\r'; p++, data_p++, recv_size--) {
          chunk_chrs[p] = *data_p;
        }
        data_p+=2; recv_size-=2; // CRLF
        chunk_chrs[p] = 0;
        chunk_size = strtol(chunk_chrs, NULL, 16);
        chunk_recv = 0;

      } while(chunk_size);

      closesocket(sock); WSACleanup();
      if(on_end) on_end((char*)body_data.data(), body_data.size());
      return 0; // success

    }
    closesocket(sock); WSACleanup();
    if(on_err) on_err(url_str);
    GME_Logs(GME_LOG_ERROR, "GME_NetwHttpGET", "Unsupported transfer encoding", url_str);
    return GME_HTTPGET_ERR_ENC;
  }

  /* we begin download */
  int pct, bps;
  size_t body_size = 0;
  char* body_data;
  clock_t t = clock(); /* start clock for download speed */

  /* alloc memory safely... */
  try {
    body_data = new char[header.content_length];
  } catch(const std::bad_alloc&) {
    closesocket(sock); WSACleanup();
    if(on_err) on_err(url_str);
    GME_Logs(GME_LOG_ERROR, "GME_NetwHttpGET", "Bad alloc", std::to_string(header.content_length).c_str());
    return GME_HTTPGET_ERR_BAL;
  }

  /* first part of body recieved with header */
  recv_size -= (header.size+4);
  if(recv_size) memcpy(body_data, &recv_buff[header.size+4], recv_size);
  body_size += recv_size;

  if(on_dnl) {
    pct = (float(body_size)/float(header.content_length))*100;
    bps = float(body_size)/(float(clock()-t)/CLOCKS_PER_SEC);
    if(!on_dnl(pct, bps)) {
      closesocket(sock); WSACleanup();
      delete[] body_data;
      GME_Logs(GME_LOG_NOTICE, "GME_NetwHttpGET", "Cancelled by user", url_str);
      return 0; // cancelled
    }
  }

  /* do we have more data to receive ? */
  if(header.content_length > body_size) {

    do {
      /* receive bunch of data */
      recv_size = recv(sock, recv_buff, sizeof(recv_buff), 0);
      if(recv_size == SOCKET_ERROR || recv_size == 0) {
        /* stream error... */
        closesocket(sock); WSACleanup();
        delete[] body_data;
        if(on_err) on_err(url_str);
        GME_Logs(GME_LOG_ERROR, "GME_NetwHttpGET", "recv failed", url_str);
        return GME_HTTPGET_ERR_REC;
      }
      memcpy(body_data+body_size, recv_buff, recv_size);
      body_size += recv_size;

      if(on_dnl) {
        pct = (float(body_size)/float(header.content_length))*100;
        bps = float(body_size)/(float(clock()-t)/CLOCKS_PER_SEC);
        if(!on_dnl(pct, bps)) {
          closesocket(sock); WSACleanup();
          delete[] body_data;
          GME_Logs(GME_LOG_NOTICE, "GME_NetwHttpGET", "Cancelled by user", url_str);
          return 0; // cancelled
        }
      }
    } while(body_size < header.content_length);
  }

  /* download successful */
  closesocket(sock); WSACleanup();

  /* send data to callback */
  if(on_end) on_end(body_data, body_size);
  delete[] body_data;

  return 0; // success
}



/*
  function to send GET Http request to a server.
*/
int GME_NetwHttpGET(const char* url_str, const GME_NetwGETOnErr on_err, const GME_NetwGETOnDnl on_dnl, const GME_NetwGETOnSav on_sav, const std::wstring& path)
{
  WSADATA wsa;
  WSAStartup(MAKEWORD(2,2),&wsa);

  /* parse url */
  GME_Url_Struct url = GME_NetwParseUrl(url_str);

  /* get host address for connection infos */
  sockaddr saddr;
  if(!GME_NetwGetIp4(url.host, url.port, &saddr)) {
    WSACleanup();
    if(on_err) on_err(url_str);
    GME_Logs(GME_LOG_ERROR, "GME_NetwHttpGET", "GME_NetwGetIp4 failed", url_str);
    return GME_HTTPGET_ERR_DNS;
  }

  /* open a new connection */
  SOCKET sock;
  if((sock = GME_NetwConnect(&saddr)) == INVALID_SOCKET) {
    WSACleanup();
    if(on_err) on_err(url_str);
    GME_Logs(GME_LOG_ERROR, "GME_NetwHttpGET", "GME_NetwConnect failed", url_str);
    return GME_HTTPGET_ERR_CNX;
  }

  /* create HTTP request */
  char http_req[256];
  if(strlen(url.path)) {
    sprintf(http_req, "GET %s HTTP/1.1\r\nHost: %s \r\n\r\n", url.path, url.host);
  } else {
    sprintf(http_req, "GET / HTTP/1.1\r\nHost: %s \r\n\r\n", url.host);
  }

  /* send request to server */
  send(sock, http_req, strlen(http_req), 0);

  /* stuff to receive data */
  char recv_buff[4096];
  int recv_size;

  /* receiving HTTP response (or not) */
  recv_size = recv(sock, recv_buff, sizeof(recv_buff), 0);
  if(recv_size == SOCKET_ERROR || recv_size == 0) {
    /* stream error... */
    closesocket(sock); WSACleanup();
    if(on_err) on_err(url_str);
    GME_Logs(GME_LOG_ERROR, "GME_NetwHttpGET", "recv failed", url_str);
    return GME_HTTPGET_ERR_REC;
  }

  /* parse HTTP response header */
  GME_Http_Head_Struct header = GME_NetwHttpParseHead(recv_buff, recv_size);

  if(header.code > 302) {
    /* this is a 404 or any HTTP server error... */
    closesocket(sock); WSACleanup();
    if(on_err) on_err(url_str);
    GME_Logs(GME_LOG_ERROR, "GME_NetwHttpGET", "HTTP code greater than 302", url_str);
    return header.code;
  }

  if(header.code == 301 || header.code == 302) {
    /* this is a redirection, we must send a new request */
    closesocket(sock); WSACleanup();
    GME_Logs(GME_LOG_WARNING, "GME_NetwHttpGET", "redirection", url_str);
    return GME_NetwHttpGET(header.location, on_err, on_dnl, on_sav, path);
  }

  if(!header.content_length) {
    /* check if we get a transfer encoding chunked */
    if(strstr(header.transfer_encoding, "chunked")) {
      /* this is some PHP page with chuck encoding, not supported */
    }
    closesocket(sock); WSACleanup();
    if(on_err) on_err(url_str);
    GME_Logs(GME_LOG_ERROR, "GME_NetwHttpGET", "Unsuported transfer encoding", url_str);
    return GME_HTTPGET_ERR_ENC;
  }

  /* we begin download */
  int pct, bps;
  size_t body_size = 0;
  clock_t t = clock(); /* start clock for download speed */

  /* open temporary file for writing */
  std::wstring file_path = path + L"\\";
  file_path += GME_StrToWcs(GME_NetwDecodeUrl(url.file));
  file_path += L".down";

  FILE* fp = _wfopen(file_path.c_str(), L"wb");
  if(!fp) {
    closesocket(sock); WSACleanup();
    if(on_err) on_err(url_str);
    GME_Logs(GME_LOG_ERROR, "GME_NetwHttpGET", "Unable to open file", GME_StrToMbs(file_path).c_str());
    return GME_HTTPGET_ERR_FOP;
  }

  /* first part of body received with header */
  recv_size -= (header.size+4);
  if(recv_size) {
    if(!fwrite(&recv_buff[header.size+4], recv_size, 1, fp)) {
      closesocket(sock); WSACleanup();
      fclose(fp); DeleteFileW(file_path.c_str());
      if(on_err) on_err(url_str);
      GME_Logs(GME_LOG_ERROR, "GME_NetwHttpGET", "Error on write file", GME_StrToMbs(file_path).c_str());
      return GME_HTTPGET_ERR_FWR;
    }
  }
  body_size += recv_size;

  if(on_dnl) {
    pct = (float(body_size)/float(header.content_length))*100;
    bps = float(body_size)/(float(clock()-t)/CLOCKS_PER_SEC);
    if(!on_dnl(pct, bps)) {
      closesocket(sock); WSACleanup();
      fclose(fp); DeleteFileW(file_path.c_str());
      GME_Logs(GME_LOG_NOTICE, "GME_NetwHttpGET", "Cancelled by user", url_str);
      return 0; // cancelled
    }
  }

  /* do we have more data to receive ? */
  if(header.content_length > body_size) {

    do {
      /* receive bunch of data */
      recv_size = recv(sock, recv_buff, sizeof(recv_buff), 0);
      if(recv_size == SOCKET_ERROR || recv_size == 0) {
        /* stream error... */
        closesocket(sock); WSACleanup();
        fclose(fp); DeleteFileW(file_path.c_str());
        if(on_err) on_err(url_str);
        GME_Logs(GME_LOG_ERROR, "GME_NetwHttpGET", "recv failed", url_str);
        return GME_HTTPGET_ERR_REC;
      }
      if(!fwrite(&recv_buff, recv_size, 1, fp)) {
        closesocket(sock); WSACleanup();
        fclose(fp); DeleteFileW(file_path.c_str());
        if(on_err) on_err(url_str);
        GME_Logs(GME_LOG_ERROR, "GME_NetwHttpGET", "Error on write file", GME_StrToMbs(file_path).c_str());
        return GME_HTTPGET_ERR_FWR;
      }
      body_size += recv_size;

      if(on_dnl) {
        pct = (float(body_size)/float(header.content_length))*100;
        bps = float(body_size)/(float(clock()-t)/CLOCKS_PER_SEC);
        if(!on_dnl(pct, bps)) {
          closesocket(sock); WSACleanup(); fclose(fp);
          fclose(fp); DeleteFileW(file_path.c_str());
          GME_Logs(GME_LOG_NOTICE, "GME_NetwHttpGET", "Cancelled by user", url_str);
          return 0; // cancelled
        }
      }

    } while(body_size < header.content_length);
  }

  /* download successful */
  closesocket(sock); WSACleanup();
  fclose(fp);

  /* send downloaded file path to callback */
  if(on_sav) on_sav(file_path.c_str());

  return 0; // success
}


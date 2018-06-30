/*
  MIT License

  Copyright (c) 2016-2018 Martijn Heil

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#ifndef STRONK_CONNECTION_H
#define STRONK_CONNECTION_H

#include <stdint.h>

#include <netinet/in.h>

#include <mcpr/connection.h>
#include <ninio/bstream.h>

#include "player.h"


struct connection
{
  int fd;
  struct sockaddr_storage client_address;
  struct bstream *iostream;
  uint16_t port_used; // Will only be set after having switched to a different state from HANDSHAKE
  char *server_address_used; // Will only be set after having switched to a different state from HANDSHAKE, else it will be NULL
  mcpr_connection *conn;
  struct player *player; // may be NULL
  bool auth_required;

  bool tmp_present;
  struct {
    int32_t verify_token_length;
    void *verify_token;
    RSA *rsa;
    char *username;
  } tmp;
};
void connection_close(struct connection *conn, const char *disconnect_message);

#endif

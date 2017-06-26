/*
    MIT License

    Copyright (c) 2016 Martijn Heil

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

#ifndef MCPR_CONNECTION_H
#define MCPR_CONNECTION_H

#include <stdbool.h>
#include <openssl/evp.h>
#include <ninio/bstream.h>
#include <mcpr/mcpr.h>
#include <mcpr/abstract_packet.h>

typedef void mcpr_connection;

mcpr_connection *mcpr_connection_new(int sockfd);
void mcpr_connection_incref(mcpr_connection *conn);
void mcpr_connection_decref(mcpr_connection *conn);

bool mcpr_connection_update(void);
void mcpr_connection_set_packet_handler(void (*on_packet)(const struct mcpr_abstract_packet *pkt));
void mcpr_connection_write_packet(const struct mcpr_abstract_packet *pkt);

#endif

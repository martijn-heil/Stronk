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
#include <mcpr/packet.h>

typedef void mcpr_connection;

// Calling any function on a closed connection is undefined behaviour.
mcpr_connection *mcpr_connection_new(struct bstream *stream);
void mcpr_connection_incref(mcpr_connection *conn);
void mcpr_connection_decref(mcpr_connection *conn);
bool mcpr_connection_update(mcpr_connection *conn);
void mcpr_connection_set_packet_handler(mcpr_connection *conn, bool (*on_packet)(const struct mcpr_packet *pkt, mcpr_connection *conn));
bool mcpr_connection_is_closed(mcpr_connection *conn);
bool mcpr_connection_write_packet(mcpr_connection *conn, const struct mcpr_packet *pkt);
void mcpr_connection_set_crypto(mcpr_connection *conn, EVP_CIPHER_CTX *ctx_encrypt, EVP_CIPHER_CTX *ctx_decrypt);
void mcpr_connection_set_use_encryption(mcpr_connection *conn, bool value);
void mcpr_connection_set_compression(mcpr_connection *tmpconn, bool compression);
enum mcpr_state mcpr_connection_get_state(mcpr_connection *conn);
void mcpr_connection_set_state(mcpr_connection *conn, enum mcpr_state state);
void mcpr_connection_close(mcpr_connection *conn, const char *reason);

#endif

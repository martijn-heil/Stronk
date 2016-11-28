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



    mcpr.c - Minecraft Protocol functions
*/

#ifndef MCPR_H
#define MCPR_H

#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>

#include <arpa/inet.h>
#include <uuid/uuid.h>

#include <jansson/jansson.h>
#include <nbt/nbt.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

#include "codec.h"

/*
    Minecraft Protocol (http://wiki.vg/Protocol)
    MCPR = MineCraft PRotocol.
*/

#define MCPR_PROTOCOL_VERSION 315


/*
 * out should be at least the size of max_out_size
 * max_out_size, obviously, has to be known by the caller, these functions do not handle that.
 *
 * Returns the new size of out or -1 upon error.
 */
int mcpr_uncompress(void *out, const void *in, size_t max_out_size, size_t in_size);

/*
 * Out should be at least the size of mcr_compress_bounds(n)
 *
 * Returns the new size of out or -1 upon error.
 */
int mcpr_compress(void *out, const void *in, size_t n);

/*
 * Will return the maximum compressed size for len amount of bytes.
 */
size_t mcpr_compress_bounds(size_t len);


enum mcpr_state {
    MCPR_STATE_HANDSHAKE = 0,
    MCPR_STATE_STATUS = 1,
    MCPR_STATE_LOGIN = 2,
    MCPR_STATE_PLAY = 3
};

enum mcpr_connection_type {
    MCPR_CONNECTION_TYPE_SERVERBOUND,
    MCPR_CONNECTION_TYPE_CLIENTBOUND
};

struct mcpr_connection {
    int sockfd;
    enum mcpr_state state;
    bool use_compression;
    unsigned int compression_treshold; // Not guaranteed to be initialized if compression is set to false.

    bool use_encryption;
    EVP_CIPHER_CTX ctx_encrypt;  // Not guaranteed to be initialized if use_encryption is set to false
    EVP_CIPHER_CTX ctx_decrypt;  // Not guaranteed to be initialized if use_encryption is set to false
    unsigned int encryption_block_size;  // Not guaranteed to be initialized if use_encryption is set to false

    enum mcpr_connection_type type;
    bool is_online_mode;
};

struct mcpr_packet {
    uint8_t id;
    void *data;
    size_t data_len;
};

struct mcpr_client_player {
    struct mcpr_connection conn;
};


struct mcpr_server {
    struct mcpr_client_player **client_players;
};

struct mcpr_client {
    struct mcpr_connection conn;


    char *client_token; // Should be free'd using the free function specified with mcpr_set_free_func()
    size_t client_token_len; // It's NUL terminated anyway, but you can use this to prevent wasting performance with strlen.
    char *access_token; // Should be free'd using the free function specified with mcpr_set_free_func()
    size_t access_token_len; // It's NUL terminated anyway, but you can use this to prevent wasting performance with strlen.
    const char *username;
    const char *account_name; // either email or username.
};

/*
 * out should be at least of size (len + mcpr_client->encryption_block_size - 1)
 */
int mcpr_encrypt(void *out, const void *data, EVP_CIPHER_CTX ctx_encrypt, size_t len);

/*
 * out should be at least of size (len + mcpr_client->encryption_block_size)
 */
int mcpr_decrypt(void *out, const void *data, EVP_CIPHER_CTX ctx_decrypt, size_t len);


//struct mcpr_server_sess mcpr_init_server_sess(const char *host, int port);

// use_encryption is only temporarily.. will be removed before this library goes into real usage.
int mcpr_init_client(struct mcpr_client *sess, const char *host, int port, int timeout, const char *account_name, bool use_encryption);


int mcpr_write_packet(struct mcpr_connection *conn, struct mcpr_packet *pkt, bool force_no_compression);

struct mcpr_packet *mcpr_read_packet(struct mcpr_connection *conn); // returns NULL on error. returned packet should be free'd using the free function specified with mcpr_set_free_func()

/*
 * Writes the contents of data to the connection.
 * Does encryption if encryption is enabled for the specified connection.
 * Returns the amount of bytes written or < 0 upon error.
 */
int mcpr_write_raw(const struct mcpr_connection *conn, const void *data, size_t len);



#endif

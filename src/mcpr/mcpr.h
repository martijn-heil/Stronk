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
 * Will malloc *out for you. Don't forget to free it.
 */
int mcpr_compress(void **out, void *in);

/*
 * Will malloc *out for you. Don't forget to free it.
 */
int mcpr_decompress(void **out, void *in); //


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

    const enum mcpr_connection_type type;
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


int mcpr_write_packet(struct mcpr_connection *conn, struct mcpr_packet *pkt);
struct mcpr_packet *mcpr_read_packet(struct mcpr_connection *conn); // returns NULL on error. returned packet should be free'd using the free function specified with mcpr_set_free_func()

#endif

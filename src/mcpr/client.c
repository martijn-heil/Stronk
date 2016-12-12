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



    client.c - Client specific functions
*/

#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <stdbool.h>

#include <arpa/inet.h>
#include <uuid/uuid.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>

#include <jansson/jansson.h>
#include <safe_math.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/sha.h>
#include <curl/curl.h>

#include "mcpr.h"
#include "client.h"
#include "../util.h"
#include "mapi.h"

#ifdef MCPR_DO_LOGGING
    #include "../stronk.h"
#include "codec.h"

#endif

static struct _mcpr_client_connection {
    enum mcpr_state state;
    bool use_compression;
    unsigned int compression_treshold; // Not guaranteed to be initialized if compression is set to false.

    bool use_encryption;
    EVP_CIPHER_CTX ctx_encrypt;  // Not guaranteed to be initialized if use_encryption is set to false
    EVP_CIPHER_CTX ctx_decrypt;  // Not guaranteed to be initialized if use_encryption is set to false
    int encryption_block_size;  // Not guaranteed to be initialized if use_encryption is set to false

    enum mcpr_connection_type type;
    bool is_online_mode;

    int sockfd;
};

static struct _mcpr_client {
    bool is_connected;
    struct _mcpr_client_connection conn; // Should be destroyed upon disconnection.


    bool is_legacy;
    bool is_online_mode;
    char *client_token; // Should be free'd using the free function specified with mcpr_set_free_func()
    char *access_token; // Should be free'd using the free function specified with mcpr_set_free_func()
    char *player_name; // Should be free'd.
    char *account_name; // Should be free'd.
    uuid_t uuid;
};


// Private helper functions
static int                  init_socket                                 (int *returned_sockfd, const char *host, short port);
static int                  minecraft_stringify_sha1                    (char *stringified_hash, const unsigned char *hash1);
static size_t               mcpr_curl_write_callback                    (void *contents, size_t size, size_t nmemb, void *arg);
static int                  send_server_hash_to_mojang                  (char *restrict server_id, unsigned char *restrict shared_secret, size_t shared_secret_len, int8_t server_pubkey[], size_t server_pubkey_len);
static int                  handle_compression_and_login_success        (struct _mcpr_client *client);
static int                  handle_encryption                           (struct _mcpr_client *client);
static int                  send_login_state_2_packet                   (struct _mcpr_client *client, int port, const char *host);
static int                  send_login_start_packet                     (struct _mcpr_client *client);
struct mcpr_auth_response  *json_to_auth_response                        (json_t *json); // Returns NULL on error or a malloc'ed mcpr_auth_response, which should be free'd
struct                      mcpr_curl_buffer;


void mcpr_client_destroy(mcpr_client client) {
    struct _mcpr_client *client2 = client;

    free(client2->client_token);
    free(client2->access_token);
    free(client2->player_name);
    free(client2->account_name);
    free(client2);
}

mcpr_client mcpr_client_create(const char *client_token, const char *access_token, const char *account_name, const char *player_name, bool online_mode, bool legacy) {
    struct _mcpr_client *client = malloc(sizeof(struct _mcpr_client));
    if (unlikely(client == NULL)) {
        return NULL;
    }

    client->is_connected = false;
    client->is_online_mode = online_mode;
    client->is_legacy = legacy;


    client->client_token = malloc(strlen(client_token) + 1);
    if (unlikely(client->client_token == NULL)) {
        return NULL;
    }
    strcpy(client->client_token, client_token);

    client->player_name = malloc(strlen(player_name) + 1);
    if (unlikely(client->player_name == NULL)) {
        free(client->client_token);
        free(client);
        return NULL;
    }

    if (online_mode) {
        client->access_token = malloc(strlen(access_token) + 1);
        if (unlikely(client->access_token == NULL)) {
            free(client->client_token);
            free(client->player_name);
            free(client);
            return NULL;
        }
        strcpy(client->access_token, access_token);

        if(!legacy) {
            client->account_name = malloc(strlen(account_name) + 1);
            if (unlikely(client->account_name == NULL)) {
                free(client->client_token);
                free(client->player_name);
                free(client->access_token);
                free(client);
                return NULL;
            } else {
                client->account_name = NULL;
            }
        }
    } else {
        client->access_token = NULL;
        client->account_name = NULL;
    }

    return client;
}

int mcpr_client_connect(mcpr_client client1, const char *host, short port, unsigned int sock_timeout) {
    struct _mcpr_client *client = client1;

    int init_socket_status = init_socket(&(client->conn.sockfd), host, port);
    if(init_socket_status < 0) { return -1; }

    // Set socket timeout.
    struct timeval timeoutval;
    timeoutval.tv_sec = sock_timeout;
    timeoutval.tv_usec = 0;

    int setsockopt_status1 = setsockopt(client->conn.sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeoutval, sizeof(timeoutval));
    if(setsockopt_status1 == -1) {
        close(client->conn.sockfd);
        return -1;
    }

    int setsockopt_status2 = setsockopt(client->conn.sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeoutval, sizeof(timeoutval));
    if(setsockopt_status2 == -1) {
        close(client->conn.sockfd);
        return -1;
    }

    client->is_connected = true;
    client->conn.use_compression = false; // Will be updated later to the appropriate value.
    client->conn.is_online_mode = false; // Will be updated later to the appropriate value.
    client->conn.state = MCPR_STATE_HANDSHAKE;
    client->conn.type = MCPR_CONNECTION_TYPE_SERVERBOUND;

    // Send Handshake state 2 packet.
    if(unlikely(send_login_state_2_packet(client, port, host) < 0)) {
        return -1;
    }

    client->conn.state = MCPR_STATE_LOGIN;

    // Send login start packet.
    if(unlikely(send_login_start_packet(client) < 0)) {
        return -1;
    }

    // Check if server is in online mode or offline mode, based on if we receive a Encryption Request or not.
    // Thus this also checks if we use encryption or not.
    uint8_t *recbuf = malloc(MCPR_VARINT_SIZE_MAX + MCPR_VARINT_SIZE_MAX);
    if(unlikely(recbuf == NULL)) {
        return -1;
    }
    ssize_t bytes_received = recv(client->conn.sockfd, recbuf, MCPR_VARINT_SIZE_MAX + MCPR_VARINT_SIZE_MAX, MSG_PEEK); // TODO
    if(unlikely(bytes_received == -1 || bytes_received == 0)) {
        free(recbuf);
        return -1;
    }
    int32_t pktlen;
    int bytes_read_1 = mcpr_decode_varint(&pktlen, recbuf, bytes_received);
    if(bytes_read_1 < 0) {
        return -1;
    }
    if(pktlen < bytes_received) {
        return -1;
    }
    bytes_received -= bytes_read_1;
    if(bytes_received <= 0) {
        return -1;
    }
    int32_t pkt_id;
    int bytes_read_2 = mcpr_decode_varint(&pkt_id, recbuf + bytes_read_1, bytes_received);
    if(bytes_read_2 < 0) {
        return -1;
    }
    client->conn.is_online_mode = pkt_id == 0x03; // Encryption Request packet ID.
    client->conn.use_encryption = pkt_id == 0x03;
    free(recbuf);




    // Encryption is not used for loopback connections... right? TODO figure out! Also, does authentication happen for loopback?!
    if(client->conn.use_encryption) {
        if(unlikely(handle_encryption(client) < 0)) {
            return -1;
        }
    }


    // Handle Set Compression and Login Success packets.
    if(unlikely(handle_compression_and_login_success(client) < 0)) {
        return -1;
    }


    client->conn.state = MCPR_STATE_PLAY;


    return 0;
}


























static int init_socket(int *returned_sockfd, const char *host, short port) {
    int sockfd;
    int portno = port;
    struct sockaddr_in serveraddr;
    struct hostent *server;


    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (unlikely(sockfd < 0)) {
        #ifdef MCPR_DO_LOGGING
            nlog_error("Could not create socket. (%s ?)", strerror(errno));
        #endif
        return -1;
    }

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(host);
    if (unlikely(server == NULL))
    {
        #ifdef MCPR_DO_LOGGING
            nlog_error("Could not get by name. (host: %s).", host);
        #endif
        return -2;
    }

    /* build the server's Internet address */
    //bzero((char *) &serveraddr, sizeof(serveraddr));
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    //bcopy((char *) server->h_addr, (char *) &serveraddr.sin_addr.s_addr, server->h_length);
    memcpy(server->h_addr_list[0], &(serveraddr.sin_addr.s_addr), server->h_length);

    if(port > INT16_MAX) { return -1; }
    serveraddr.sin_port = htons((uint16_t) portno);

    /* connect: create a connection with the server */
    if (unlikely(connect(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)) {
        #ifdef MCPR_DO_LOGGING
            nlog_error("Could not connect to server. (%s ?)", strerror(errno));
        #endif
        return -3;
    }

    *returned_sockfd = sockfd;
    return 0;
}

static int minecraft_stringify_sha1(char *stringified_hash, const unsigned char *hash1) {
    unsigned char uhash[SHA_DIGEST_LENGTH];
    for(int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        uhash[i] = hash1[i];
    }

    char *hash = (char *) uhash;
    char *stringified_hashp = stringified_hash + 1;

    bool is_negative = *hash < 0;
    if(is_negative) {
        bool carry = true;
        int i;
        unsigned char new_byte;
        unsigned char value;

        for(i = SHA_DIGEST_LENGTH - 1; i >= 0; --i) {
            value = uhash[i];
            new_byte = ~value & 0xFF;
            if(carry) {
                carry = new_byte == 0xFF;
                uhash[i] = new_byte + 1;
            } else {
                uhash[i] = new_byte;
            }
        }
    }

    // Write it as a hex string.
    for(int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        sprintf(stringified_hashp, "%02x", uhash[i]);
        stringified_hashp += 2;
    }
    *stringified_hashp = '\0';

    // Trim leading zeros
    stringified_hashp = stringified_hash + 1;
    for(int i = 0; i < SHA_DIGEST_LENGTH * 2; i++) {
        if(*stringified_hashp != '0') { break; }
        stringified_hashp++;
    }


    if(is_negative) { // Hash is negative.
        *stringified_hash = '-';
    }


    return 0;
}



struct mcpr_curl_buffer {
    char *content; // Should be NUL terminated.
    size_t size;
};




static size_t mcpr_curl_write_callback(void *contents, size_t size, size_t nmemb, void *arg) {
    struct mcpr_curl_buffer *buf = arg;
    size_t realsize = size * nmemb;
    buf->content = realloc(buf->content, buf->size + realsize + 1);
    if(buf->content == NULL) {
        return 0;
    }

    memcpy(&(buf->content[buf->size]), contents, realsize);
    buf->size += realsize;
    buf->content[buf->size] = '\0';

    return realsize;
}




static int send_server_hash_to_mojang(char *restrict server_id, unsigned char *restrict shared_secret, size_t shared_secret_len, int8_t server_pubkey[], size_t server_pubkey_len) {
    unsigned char *server_id_hash = malloc(SHA_DIGEST_LENGTH);
    if(unlikely(server_id_hash == NULL)) {
        #ifdef MCPR_DO_LOGGING
            nlog_error("Could not allocate memory. (%s ?)", strerror(errno));
        #endif
        return -1;
    }

    SHA_CTX sha_ctx;
    if(unlikely(SHA1_Init(&sha_ctx) == 0)) {
        #ifdef MCPR_DO_LOGGING
            nlog_error("Could not initialize Sha1 hash.");
        #endif
        return -1;
    }

    if(server_id != '\0') { // Make sure the string isn't empty.
        if(unlikely(SHA1_Update(&sha_ctx, server_id, strlen(server_id)) == 0)) {
            #ifdef MCPR_DO_LOGGING
                nlog_error("Could not update Sha1 hash.");
            #endif
            return -1;
        }
    }
    if(unlikely(SHA1_Update(&sha_ctx, shared_secret, shared_secret_len) == 0)) {
        #ifdef MCPR_DO_LOGGING
            nlog_error("Could not update Sha1 hash.");
        #endif
        return -1;
    }

    if(unlikely(SHA1_Update(&sha_ctx, server_pubkey, server_pubkey_len) == 0)) {
        #ifdef MCPR_DO_LOGGING
            nlog_error("Could not update Sha1 hash.");
        #endif
        return -1;
    }

    if(unlikely(SHA1_Final(server_id_hash, &sha_ctx) == 0)) {
        #ifdef MCPR_DO_LOGGING
            nlog_error("Could not finalize Sha1 hash.");
        #endif
        return -1;
    }


    char *stringified_server_id_hash = malloc(SHA_DIGEST_LENGTH * 2 + 2);
    if(unlikely(stringified_server_id_hash == NULL)) {
        #ifdef MCPR_DO_LOGGING
            nlog_error("Could not allocate memory. (%s ?)", strerror(errno));
        #endif
        free(server_id_hash);
        return -1;
    }
    int mc_stringify_status = minecraft_stringify_sha1(stringified_server_id_hash, server_id_hash);
    if(unlikely(mc_stringify_status < 0)) {
        #ifdef MCPR_DO_LOGGING
            nlog_error("Error stringifying SHA1 hash for client authentication.");
        #endif
        free(server_id_hash);
        free(stringified_server_id_hash);
        return -1;
    }

    free(server_id_hash);
    free(stringified_server_id_hash);

    return 0;
}


static int handle_encryption(struct _mcpr_client *client) {
    // We will get this data from the Encryption Request packet, we need it after that.
    int shared_secret_len = 16;
    unsigned char shared_secret[shared_secret_len];

    unsigned char* encrypted_shared_secret_buf;
    int encrypted_shared_secret_len;

    int32_t verify_token_len;
    int8_t *verify_token;
    unsigned char *encrypted_verify_token;
    int encrypted_verify_token_len;

    char *server_id;
    int8_t *server_pubkey;
    size_t server_pubkey_len;


    //
    // Read encryption request packet
    //
    {
        uint8_t pkt_len_raw[5];

        ssize_t bytes_read = read(client->conn.sockfd, &pkt_len_raw, 1);
        if(unlikely(bytes_read == -1)) {
            #ifdef MCPR_DO_LOGGING
                nlog_error("Error reading from socket. (%s ?)", strerror(errno));
            #endif
            return -1;
        }

        int32_t pktlen;
        int bytes_read_2 = mcpr_decode_varint(&pktlen, &pkt_len_raw, 5);
        if(bytes_read_2 < 0) { return -1; }
        if(pktlen < 0) { return -1; }
        DO_GCC_PRAGMA(GCC diagnostic push)
        DO_GCC_PRAGMA(GCC diagnostic ignored "-Wtype-limits")
            if(unlikely((uint32_t) pktlen > SIZE_MAX)) { return -1; }
        DO_GCC_PRAGMA(GCC diagnostic pop)
        if(unlikely(pktlen < 1)) { return -1; }

        uint8_t *buf = malloc((size_t) pktlen);
        if(unlikely(buf == NULL)) {
            #ifdef MCPR_DO_LOGGING
                nlog_error("Could not allocate memory. (%s ?)", strerror(errno));
            #endif
            return -1;
        }
        ssize_t bytes_read_3 = read(client->conn.sockfd, buf, (size_t) pktlen);
        if(bytes_read_3 == -1) { free(buf); return -1; }
        if(bytes_read_3 != pktlen) { free(buf); return -1; }
        size_t len_left = pktlen;


        uint8_t *bufpointer = buf;


        // Read packet ID
        int32_t pkt_id;
        int bytes_read_4 = mcpr_decode_varint(&pkt_id, bufpointer, len_left);
        if(unlikely(bytes_read_4 < 0)) { free(buf); return -1; }
        if(unlikely(pkt_id != 0x01)) { free(buf); return -1; } // Ensure that this packet is actually an encryption request packet..
        bufpointer += bytes_read_4;
        len_left -= bytes_read_4;

        // Read server ID string length
        if(unlikely(len_left < 1)) { free(buf); return -1; }
        int32_t str_len;
        int bytes_read_5 = mcpr_decode_varint(&str_len, bufpointer, len_left);
        if(unlikely(bytes_read_5 < 0)) { free(buf); return -1; }
        if(unlikely(str_len < 0)) { free(buf); return -1; }
        DO_GCC_PRAGMA(GCC diagnostic push)
        DO_GCC_PRAGMA(GCC diagnostic ignored "-Wtype-limits")
            if(unlikely(((uint32_t) str_len) > SIZE_MAX)) { free(buf); return -1; }
        DO_GCC_PRAGMA(GCC diagnostic pop)
        bufpointer += bytes_read_5;
        len_left -= bytes_read_5;

        // Read server ID
        if(len_left < (uint32_t) str_len) { free(buf); return -1; }
        server_id = malloc((str_len + 1) * sizeof(char));
        if(unlikely(server_id == NULL)) { free(buf); return -1; }
        int bytes_read_6 = mcpr_decode_string(server_id, bufpointer, len_left);
        if(unlikely(bytes_read_6 < 0)) { free(server_id); free(buf); return -1; }
        bufpointer += bytes_read_6;
        len_left -= bytes_read_6;

        // Read public key length
        if(len_left < 1) { free(server_id); free(buf); return -1; }
        int32_t pubkeylen;
        int bytes_read_7 = mcpr_decode_varint(&pubkeylen, bufpointer, len_left);
        if(unlikely(bytes_read_7 < 0)) { free(server_id); free(buf); return -1; }
        if(unlikely(pubkeylen < 0)) { free(server_id); free(buf); return -1; }
        DO_GCC_PRAGMA(GCC diagnostic push)
        DO_GCC_PRAGMA(GCC diagnostic ignored "-Wtype-limits")
            if(unlikely(((uint32_t) pubkeylen) > SIZE_MAX)) { free(server_id); free(buf); return -1; }
        DO_GCC_PRAGMA(GCC diagnostic pop)
        bufpointer += bytes_read_7;
        len_left -= bytes_read_6;

        // Read public key
        if(len_left < ((uint32_t) pubkeylen)) { free(server_id); free(buf); return -1; }
        int8_t *pubkey = malloc((size_t) pubkeylen);
        if(unlikely(pubkey == NULL)) {
            #ifdef MCPR_DO_LOGGING
                nlog_error("Could not allocate memory. (%s ?)", strerror(errno));
            #endif
            return -1;
        }
        int8_t *pubkeyp = pubkey;
        for(int32_t i = 0; i < pubkeylen; i++) {
            int bytes_read_8 = mcpr_decode_byte(pubkeyp, bufpointer);
            if(unlikely(bytes_read_8 < 0)) { free(server_id); free(buf); free(pubkey); return -1; }
            pubkey++;
            bufpointer++;
            len_left--;
        }

        // Read verify token length
        if(unlikely(len_left < 1)) { free(server_id); free(buf); free(pubkey); return -1; }
        int bytes_read_9 = mcpr_decode_varint(&verify_token_len, bufpointer, len_left);
        if(unlikely(bytes_read_9 < 0)) { free(server_id); free(buf); free(pubkey); return -1; }
        if(unlikely(verify_token_len < 0)) { free(server_id); free(buf); free(pubkey); return -1; }
        DO_GCC_PRAGMA(GCC diagnostic push)
        DO_GCC_PRAGMA(GCC diagnostic ignored "-Wtype-limits")
            if(unlikely(((uint32_t) verify_token_len) > SIZE_MAX)) { free(server_id); free(buf); free(pubkey); return -1; }
        DO_GCC_PRAGMA(GCC diagnostic pop)
        bufpointer += bytes_read_9;
        len_left -= bytes_read_9;

        if(len_left < ((uint32_t) verify_token_len)) { free(server_id); free(buf); free(pubkey); return -1; }
        verify_token = malloc((size_t) verify_token_len);
        int8_t *verify_tokenp = verify_token;
        for(int32_t i = 0; i < verify_token_len; i++) {
            int bytes_read_10 = mcpr_decode_byte(verify_tokenp, bufpointer);
            if(unlikely(bytes_read_10 < 0)) { free(server_id); free(buf); free(pubkey); free(verify_token); return -1; }
            verify_tokenp++;
            bufpointer++;
        }


        RSA *rsa = d2i_RSA_PUBKEY(NULL, (const unsigned char **) &pubkey, pubkeylen);

        int secure_random_status = secure_random(&(shared_secret[0]), 16);
        if(secure_random_status < 0) { return -1; }


        // Encrypt shared secret.
        encrypted_shared_secret_len = RSA_size(rsa);
        encrypted_shared_secret_buf = malloc(encrypted_shared_secret_len);
        if(unlikely(encrypted_shared_secret_buf == NULL)) {
            RSA_free(rsa);
            free(buf);
            free(pubkey);
            free(verify_token);
            free(server_id);
            return -1;
        }
        RSA_public_encrypt(shared_secret_len, shared_secret, encrypted_shared_secret_buf, rsa, RSA_PKCS1_PADDING);

        encrypted_verify_token_len = RSA_size(rsa);
        encrypted_verify_token = malloc(encrypted_verify_token_len);
        if(unlikely(encrypted_verify_token == NULL)) {
            RSA_free(rsa);
            free(buf);
            free(pubkey);
            free(verify_token);
            free(server_id);
            return -1;
        }
        RSA_public_encrypt(verify_token_len, (unsigned char *) verify_token, encrypted_verify_token, rsa, RSA_PKCS1_PADDING);
        RSA_free(rsa);
        free(buf);
        free(verify_token);
        server_pubkey = pubkey;
        server_pubkey_len = pubkeylen;
    }


    // Generate and send server hash to Mojang servers.
    send_server_hash_to_mojang(server_id, shared_secret, shared_secret_len, server_pubkey, server_pubkey_len);



    // Send Encryption response packet.
    {
        uint8_t *buf = malloc(15 + encrypted_shared_secret_len + encrypted_verify_token_len);
        uint8_t *bufpointer = buf + 5; // Skip the first 5 bytes to leave space for the packet length varint.

        // Write packet ID.
        int bytes_written_1 = mcpr_encode_varint(bufpointer, 0x01);
        if(unlikely(bytes_written_1 < 0)) { free(buf); return -1; }
        bufpointer += bytes_written_1;

        // Write shared secret length.
        int bytes_written_2 = mcpr_encode_varint(bufpointer, (int32_t) shared_secret_len);
        if(unlikely(bytes_written_2 < 0)) { free(buf); return -1; }
        bufpointer += bytes_written_1;

        // Write shared secret. (Encrypted using the server's public key)
        unsigned char *encrypted_shared_secret_pointer = encrypted_shared_secret_buf;
        for(int i = 0; i < encrypted_shared_secret_len; i++) {
            int bytes_written_3 = mcpr_encode_byte(bufpointer, (int8_t)(*encrypted_shared_secret_pointer));
            if(unlikely(bytes_written_3 < 0)) { free(buf); return -1; }
            bufpointer++;
            encrypted_shared_secret_pointer++;
        }

        // Write verify token length.
        int bytes_written_4 = mcpr_encode_varint(bufpointer, verify_token_len);
        if(unlikely(bytes_written_4 < 0)) { free(buf); return -1; }
        bufpointer += bytes_written_4;

        // Write verify token. (Encrypted using the server's public key)
        uint8_t *encrypted_verify_token_pointer = encrypted_verify_token;
        for(int i = 0; i < encrypted_verify_token_len; i++) {
            int bytes_written_5 = mcpr_encode_byte(bufpointer, *((int8_t *) encrypted_verify_token_pointer));
            if(unlikely(bytes_written_5 < 0)) { free(buf); return -1; }
            bufpointer++;
            encrypted_verify_token_pointer++;
        }

        // Prefix the packet by the length of it all.
        int32_t pktlen = bufpointer - (buf + 5);
        int8_t tmppktlen[5];
        int bytes_written_6 = mcpr_encode_varint(&tmppktlen, pktlen);
        if(unlikely(bytes_written_6 < 0)) { free(buf); return -1; }
        memcpy(buf + (5 - bytes_written_6), &tmppktlen, bytes_written_6);

        ssize_t status = write(client->conn.sockfd, buf + (5 - bytes_written_6), pktlen);
        free(buf);
        if(status == -1) {
            #ifdef MCPR_DO_LOGGING
                nlog_error("Error writing to socket. (%s ?)", strerror(errno));
            #endif
            return -1;
        }
    }



    //
    //  Enable & initialize encryption
    //  See https://gist.github.com/Dav1dde/3900517
    //
    //unsigned char *key = shared_secret;
    //unsigned char *iv = shared_secret; // TODO why this weirdness in the example?

    // Initialize this stuff..
    EVP_CIPHER_CTX_init(&(client->conn.ctx_encrypt));
    if(unlikely(EVP_EncryptInit_ex(&(client->conn.ctx_encrypt), EVP_aes_128_cfb8(), NULL, shared_secret, shared_secret) == 0)) { // Should engine be NULL?? I don't know
        #ifdef MCPR_DO_LOGGING
            nlog_error("Error upon EVP_EncryptInit_ex().");
        #endif
        return -1;
    }

    EVP_CIPHER_CTX_init(&(client->conn.ctx_decrypt));
    if(unlikely(EVP_DecryptInit_ex(&(client->conn.ctx_decrypt), EVP_aes_128_cfb8(), NULL, shared_secret, shared_secret) == 0)) { // Should engine be NULL?? I don't know
        #ifdef MCPR_DO_LOGGING
            nlog_error("Error upon EVP_DecryptInit_ex().");
        #endif
        return -1;
    }

    client->conn.encryption_block_size = EVP_CIPHER_block_size(EVP_aes_128_cfb8());

    free(encrypted_shared_secret_buf);
    free(encrypted_verify_token);
    free(server_pubkey);
    free(server_id);

    return 0;
}



static int send_login_state_2_packet(struct _mcpr_client *client, int port, const char *host) {
    uint8_t *buf = malloc(strlen(host) + 27);
    if(unlikely(buf == NULL)) return -1;
    size_t offset = 5; // Skip the first 5 bytes to leave room for the length varint.

    // Writes 5 bytes at most
    // write Packet ID
    int bytes_written_1 = mcpr_encode_varint(buf + offset, 0x00);
    if(unlikely(bytes_written_1 < 0)) { free(buf); return -1; }
    offset += bytes_written_1;

    // Writes 5 bytes at most
    // write Protocol version
    int bytes_written_2 = mcpr_encode_varint(buf + offset, MCPR_PROTOCOL_VERSION);
    if(unlikely(bytes_written_2 < 0)) { free(buf); return -1; }
    offset += bytes_written_2;

    // Writes strlen(host) + 5 bytes at most
    // write server host used to connect
    int bytes_written_3 = mcpr_encode_string(buf + offset, host);
    if(unlikely(bytes_written_3 < 0)) { free(buf); return -1; }
    offset += bytes_written_3;

    // writes 2 bytes
    // write server port used to connect
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wtype-limits"
        if(port > UINT16_MAX) { free(buf); return -1; } // Integer overflow protection.
    #pragma GCC diagnostic pop
    int bytes_written_4 = mcpr_encode_ushort(buf + offset, (uint16_t) port);
    if(unlikely(bytes_written_4 < 0)) { free(buf); return -1; }
    offset += bytes_written_4;

    // writes 5 bytes at most
    // write next state
    int bytes_written_5 = mcpr_encode_varint(buf + offset, MCPR_STATE_LOGIN);
    if(unlikely(bytes_written_5 < 0)) { free(buf); return -1; }
    offset += bytes_written_5;


    // We've written all the data and packet id, now prefix it all with the length.
    uint8_t tmplenbuf[5];
    size_t data_len = offset - 5;
    int bytes_written_6 = mcpr_encode_varint(&tmplenbuf, data_len); // -5 because of the initial offset.
    if(unlikely(bytes_written_6 < 0)) { free(buf); return -1; }
    uint8_t *new_start_of_pkt = buf + (5 - bytes_written_6);
    memcpy(new_start_of_pkt, &tmplenbuf, bytes_written_6);

    // Send the packet.
    ssize_t status = write(client->conn.sockfd, new_start_of_pkt, data_len + bytes_written_6);
    if(unlikely(status == -1)) {
        #ifdef MCPR_DO_LOGGING
            nlog_error("Error writing to socket. (%s ?)", strerror(errno));
        #endif
        return -1;
    }
    free(buf);

    return 0;
}



static int send_login_start_packet(struct _mcpr_client *client) {
    uint8_t *buf = malloc(strlen(client->player_name) + 15);
    if(unlikely(buf == NULL)) return -1;
    size_t offset = 5; // Skip the first 5 bytes to leave room for the length varint.

    // Writes 5 bytes at most
    // write Packet ID
    int bytes_written_1 = mcpr_encode_varint(buf + offset, 0x00);
    if(unlikely(bytes_written_1 < 0)) { free(buf); return -1; }
    offset += bytes_written_1;

    // Writes strlen(username) + 5 bytes at most.
    int bytes_written_2 = mcpr_encode_string(buf + offset, client->player_name); // TODO: We need username information.
    if(unlikely(bytes_written_2 < 0)) { free(buf); return -1; }
    offset += bytes_written_2;

    // Prefix data with length of packet.
    size_t data_len = offset - 5;
    uint8_t tmplenbuf[5];
    int bytes_written_3 = mcpr_encode_varint(&tmplenbuf, data_len);
    if(unlikely(bytes_written_3 < 0)) { free(buf); return -1; }
    uint8_t *pkt_start = buf + (5 - bytes_written_3);
    memcpy(pkt_start, &tmplenbuf, bytes_written_3);

    // Send the packet.
    ssize_t write_status = write(client->conn.sockfd, pkt_start, data_len + bytes_written_3);
    if(unlikely(write_status == -1)) {
        #ifdef MCPR_DO_LOGGING
            nlog_error("Error writing to socket. (%s ?)", strerror(errno));
        #endif
        return -1;
        free(buf);
    }
    free(buf);

    return 0;
}



static int handle_compression_and_login_success(struct _mcpr_client *client) {
    bool set_compression_received = false;

    for(int i = 0; i < 2; i++) {
        uint8_t pkt_len_raw[MCPR_VARINT_SIZE_MAX];
        ssize_t read_status_1 = read(client->conn.sockfd, &pkt_len_raw, 5);
        if(unlikely(read_status_1 == -1)) {
            #ifdef MCPR_DO_LOGGING
                nlog_error("Error reading from socket. (%s ?)", strerror(errno));
            #endif
            return -1;
        }

        int32_t pktleni;
        int bytes_read_1 = mcpr_decode_varint(&pktleni, pkt_len_raw, MCPR_VARINT_SIZE_MAX);
        if(unlikely(bytes_read_1 < 0)) {
            return -1;
        }

        if(unlikely(pktleni < 0)) {
            #ifdef MCPR_DO_LOGGING
                nlog_error("Read packet length is negative!");
            #endif
            return -1;
        }

        DO_GCC_PRAGMA(GCC diagnostic push)
        DO_GCC_PRAGMA(GCC diagnostic ignored "-Wtype-limits")
            if(unlikely((uint32_t) pktleni > SIZE_MAX)) {
                #ifdef MCPR_DO_LOGGING
                    nlog_error("Read packet length is greater than SIZE_MAX! Arithmetic overflow error.");
                #endif
                return -1;
            }
        DO_GCC_PRAGMA(GCC diagnostic pop)

        // pktleni is guaranteed to be non-negative and <= SIZE_MAX at this point.
        size_t pktlen = pktleni;

        uint8_t *encrypted_pktbuf = malloc(pktlen);
        if(unlikely(encrypted_pktbuf == NULL)) {
            #ifdef MCPR_DO_LOGGING
                nlog_error("Could not allocate memory. (%s ?)", strerror(errno));
            #endif
            return -1;
        }

        ssize_t read_status_2 = read(client->conn.sockfd, encrypted_pktbuf, pktlen);
        if(unlikely(read_status_2 == -1)) {
            #ifdef MCPR_DO_LOGGING
                nlog_error("Error reading from socket. (%s ?)", strerror(errno));
            #endif
            free(encrypted_pktbuf);
            return -1;
        }

        if(unlikely((unsigned int) read_status_2 < pktlen)) {
            #ifdef MCPR_DO_LOGGING
                nlog_error("Was not able to read expected packet length! Only read %zd bytes.", read_status_2);
            #endif
            free(encrypted_pktbuf);
            return -1;
        }

        // Decrypt the packet, as the encryption response packet has been sent now.
        uint8_t *pktbuf = malloc(pktlen + client->conn.encryption_block_size);
        int decrypt_status = mcpr_decrypt(pktbuf, encrypted_pktbuf, client->conn.ctx_decrypt, pktlen);
        if(unlikely(decrypt_status < 0)) {
            #ifdef MCPR_DO_LOGGING
                nlog_error("Error decrypting packet.");
            #endif
            free(encrypted_pktbuf);
            return -1;
        }
        free(encrypted_pktbuf);


        size_t len_left = decrypt_status;


        int32_t pktid;
        int bytes_read_4 = mcpr_decode_varint(&pktid, pktbuf, len_left);
        if(unlikely(bytes_read_4 < 0)) {
            free(pktbuf);
            return -1;
        }

        if(set_compression_received && pktid == 0x02) { // It's the login success packet.

        } else if(pktid == 0x03) { // It's the set compression packet.
            len_left -= bytes_read_4;
            set_compression_received = true;

            // Handle set compression packet.
            int32_t treshold;
            int bytes_read_5 = mcpr_decode_varint(&treshold, pktbuf, len_left);
            if(unlikely(bytes_read_5 < 0)) {
                free(pktbuf);
                return -1;
            }

            // Enable compression..
            if(treshold >= 0) {
                if(treshold > UINT_MAX) { return -1; }
                client->conn.compression_treshold = (unsigned int) treshold;
                client->conn.use_compression = true;
            } else {
                client->conn.use_compression = false;
            }
        } else {
            free(pktbuf);
            return -1;
        }

        free(pktbuf);
    }

    return 0;
}

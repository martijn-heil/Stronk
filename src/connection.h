#ifndef STRONK_CONNECTION_H
#define STRONK_CONNECTION_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include <sys/types.h>

#include <openssl/evp.h>

#include <mcpr/abstract_packet.h>


struct connection
{
    int fd;
    bool use_encryption;

    enum mcpr_state state;
    bool use_compression;
    unsigned long compression_threshold; // Not guaranteed to be initialized if use_compression is set to false.

    bool use_encryption;
    EVP_CIPHER_CTX *ctx_encrypt;  // Will be initialized to NULL. Should be free'd
    EVP_CIPHER_CTX *ctx_decrypt;  // Will be initialized to NULL. Should be free'd
    int encryption_block_size;  // Not guaranteed to be initialized if use_encryption is set to false
};

static inline ssize_t connection_write_abstract_packet(struct connection *conn, const struct mcpr_abstract_packet *pkt)
{
    return mcpr_fd_write_abstract_packet(conn->fd, pkt, conn->use_compression, false, conn->use_compression ? conn->compression_treshold : 0,
        conn->use_encryption, conn->use_encryption ? conn->encryption_block_size : 0, conn->use_encryption ? conn->ctx_encrypt : NULL);
}

static inline struct mcpr_abstract_packet *connection_read_abstract_packet(struct connection *conn)
{
    return mcpr_fd_read_abstract_packet(conn->fd, conn->use_compression, conn->use_compression ? conn->compression_treshold : 0,
         conn->use_encryption, conn->encryption_block_size, conn->ctx_decrypt);
}

#endif

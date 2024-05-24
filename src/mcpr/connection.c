/*
    MIT License

    Copyright (c) 2017 Martijn Heil

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

/*
    mcpr connection handles easy reading & writing of packets, mainly for the encryption and compression.
    Due to how the Minecraft protocol works, we need some buffering for this,
    which would be annoying to have to implement yourself everytime.
    A mcpr connection either be client side or server side.
*/

#ifndef MCPR_CONNECTION_H
#define MCPR_CONNECTION_H

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <openssl/evp.h>
#include <ninio/bstream.h>
#include <ninio/ninio.h>
#include <ninerr/ninerr.h>
#include <mcpr/mcpr.h>
#include <mcpr/connection.h>
#include <mcpr/crypto.h>
#include <mcpr/packet.h>
#include <mcpr/codec.h>
#include <util.h>

#include <valgrind/memcheck.h>

#include "internal.h"

#define BLOCK_SIZE EVP_CIPHER_block_size(EVP_aes_128_cfb8())

typedef void mcpr_connection;

// TODO COMPRESSION!!!
struct conn
{
    bool is_closed;
    struct bstream *io_stream;
    enum mcpr_state state;
    bool use_compression;
    unsigned long compression_threshold; // Not guaranteed to be initialized if use_compression is set to false.
    bool use_encryption;
    EVP_CIPHER_CTX *ctx_encrypt;
    EVP_CIPHER_CTX *ctx_decrypt;
    unsigned int reference_count;
    struct ninio_buffer receiving_buf;
    bool (*packet_handler)(const struct mcpr_packet *pkt, mcpr_connection *conn);

    bool tmp_encryption_state_available;
    RSA *rsa;
    int32_t verify_token_length;
    uint8_t *verify_token;
};

bool mcpr_connection_write_packet(mcpr_connection *conn, const struct mcpr_packet *pkt);

enum mcpr_state mcpr_connection_get_state(mcpr_connection *conn)
{
    return ((struct conn *) conn)->state;
}

void mcpr_connection_close(mcpr_connection *tmpconn, const char *reason)
{
    struct conn *conn = (struct conn *) tmpconn;
    if(!conn->is_closed)
    {
        if(conn->state == MCPR_STATE_LOGIN || conn->state == MCPR_STATE_PLAY) {
            struct mcpr_packet pkt;
            pkt.id = MCPR_PKT_PL_CB_DISCONNECT;
            pkt.state = (conn->state == MCPR_STATE_PLAY) ? MCPR_STATE_PLAY : MCPR_STATE_LOGIN;
            IGNORE("-Wdiscarded-qualifiers")
            pkt.data.play.clientbound.disconnect.reason = (reason != NULL) ? reason : "{\"text\":\"Disconnected by server.\"}";
            END_IGNORE()
            mcpr_connection_write_packet(conn, &pkt);
        }

        conn->is_closed = true;
    }
}

bool mcpr_connection_flush(mcpr_connection *tmpconn)
{
    struct conn *conn = (struct conn *) tmpconn;
    return bstream_flush(conn->io_stream) != -1;
}

mcpr_connection *mcpr_connection_new(struct bstream *stream)
{
    struct conn *conn = malloc(sizeof(struct conn));
    if(conn == NULL) { ninerr_set_err(ninerr_from_errno()); return NULL; }
    bstream_incref(stream);

    conn->io_stream = stream;
    conn->state = MCPR_STATE_HANDSHAKE;
    conn->use_compression = false;
    conn->use_encryption = false;
    conn->reference_count = 1;

    conn->ctx_encrypt = NULL;
    conn->ctx_decrypt = NULL;

    conn->receiving_buf.content = malloc(256 * BLOCK_SIZE);
    if(conn->receiving_buf.content == NULL) { free(conn); ninerr_set_err(ninerr_from_errno()); return NULL; }
    conn->receiving_buf.max_size = 256 * BLOCK_SIZE;
    conn->receiving_buf.size = 0;
    conn->packet_handler = NULL;

    conn->is_closed = false;

    return conn;
}

void mcpr_connection_incref(mcpr_connection *conn)
{
    ((struct conn *) conn)->reference_count++;
}

void mcpr_connection_decref(mcpr_connection *tmpconn)
{
    struct conn *conn = (struct conn *) tmpconn;
    conn->reference_count--;
    if(conn->reference_count <= 0)
    {
        bstream_decref(conn->io_stream);
        mcpr_connection_close(conn, NULL);
        EVP_CIPHER_CTX_free(conn->ctx_encrypt);
        EVP_CIPHER_CTX_free(conn->ctx_decrypt);
        free(conn->receiving_buf.content);
        free(conn);
    }
}

void mcpr_connection_set_use_encryption(mcpr_connection *conn, bool value)
{
    ((struct conn *) conn)->use_encryption = value;
}

bool update_receiving_buffer(mcpr_connection *tmpconn)
{
    struct conn *conn = (struct conn *) tmpconn;

    again:
        // Ensure the buffer is large enough.
        if(conn->receiving_buf.max_size < conn->receiving_buf.size + BLOCK_SIZE)
        {
            void *tmp = realloc(conn->receiving_buf.content, conn->receiving_buf.max_size + BLOCK_SIZE * 256);
            if(tmp == NULL) { ninerr_set_err(ninerr_from_errno()); return false; }
            conn->receiving_buf.max_size += BLOCK_SIZE * 256;
            conn->receiving_buf.content = tmp;
        }
        usize size_available = conn->receiving_buf.max_size - conn->receiving_buf.size;

        // Read a single block.

        if(conn->use_encryption)
        {
            usize tmpbuf_size = (size_available >= 256) ? 256 : size_available;
            u8 tmpbuf[tmpbuf_size];
            isize bytes_read = bstream_read_max(conn->io_stream, tmpbuf, tmpbuf_size);
            if(bytes_read < 0)
            {
                if(ninerr != NULL && strcmp(ninerr->type, "ninerr_closed") == 0) mcpr_connection_close(tmpconn, NULL);
                return false;
            }
            if (bytes_read == 0) return true;

            isize bytes_decrypted = mcpr_crypto_decrypt(conn->receiving_buf.content + conn->receiving_buf.size, tmpbuf, conn->ctx_decrypt, bytes_read);
            if(bytes_decrypted == -1) { return false; }
            conn->receiving_buf.size += bytes_decrypted;
            if (bytes_read > 0) goto again;
        }
        else
        {
            isize bytes_read = bstream_read_max(conn->io_stream, conn->receiving_buf.content + conn->receiving_buf.size, size_available);
            if(bytes_read < 0)
            {
                if(ninerr != NULL && strcmp(ninerr->type, "ninerr_closed") == 0) mcpr_connection_close(tmpconn, NULL);
                return false;
            }
            conn->receiving_buf.size += bytes_read;
            if(bytes_read > 0) goto again;
        }

    return true;
}

bool mcpr_connection_update(mcpr_connection *tmpconn)
{
    struct conn *conn = (struct conn *) tmpconn;
    if(!update_receiving_buffer(tmpconn))
    {
        if (ninerr_is(ninerr, "ninerr_broken_pipe"))
        {
            mcpr_connection_close(tmpconn, NULL);
        }
        return false;
    }

    while(conn->receiving_buf.size > 0)
    {
        int32_t pktlen;
        ssize_t result = mcpr_decode_varint(&pktlen, conn->receiving_buf.content, conn->receiving_buf.size);
        if(result == -1) return true;
        if(pktlen <= 0) { ninerr_set_err(ninerr_new("Received invalid packet length")); return false; }
        if((conn->receiving_buf.size - result) >= (uint32_t) pktlen)
        {
            DEBUG_PRINT("Received packet in mcpr_connection_update\n");
            struct mcpr_packet *pkt;
            ssize_t bytes_read = mcpr_decode_packet(&pkt, conn->receiving_buf.content + result, conn->state, (size_t) pktlen);
            if(bytes_read == -1) { mcpr_connection_close(tmpconn, NULL); return false; }
            memmove(conn->receiving_buf.content, conn->receiving_buf.content + bytes_read + result, conn->receiving_buf.size - bytes_read - result);
            conn->receiving_buf.size -= bytes_read + result;
            bool conn_still_valid = conn->packet_handler(pkt, conn);
            free(pkt);
            if(!conn_still_valid) return true;
        }
        else
        {
            break;
        }
    }

    if(!mcpr_connection_flush(tmpconn))
    {
        if (ninerr_is(ninerr, "ninerr_broken_pipe"))
        {
            mcpr_connection_close(tmpconn, NULL);
        }
        return false;
    }

    return true;
}

bool mcpr_connection_write(mcpr_connection *tmpconn, const void *in, size_t bytes)
{
    DEBUG_PRINT("Writing %zu bytes to mcpr_connection at address %p", bytes, (void *) tmpconn)
    struct conn *conn = (struct conn *) tmpconn;

    //VALGRIND_CHECK_VALUE_IS_DEFINED(((u8 *) in)[0]);
    //VALGRIND_CHECK_VALUE_IS_DEFINED(((u8 *) in)[bytes-1]);
    
    if(conn->use_encryption)
    {
        DEBUG_PRINT("Writing those bytes encrypted.");
        u8 encrypted_data[bytes + BLOCK_SIZE - 1];
        isize encrypted_data_length = mcpr_crypto_encrypt(encrypted_data, in, conn->ctx_encrypt, bytes);
        if(encrypted_data_length == -1) return false;

        //VALGRIND_CHECK_VALUE_IS_DEFINED(encrypted_data[0]);
        //VALGRIND_CHECK_VALUE_IS_DEFINED(encrypted_data[encrypted_data_length-1]);

        if(conn->use_compression) // TODO compression treshold
        {
            DEBUG_PRINT("Writing those bytes compressed.");
            u8 compressed_data[mcpr_compress_bounds(encrypted_data_length)];
            isize compression_result = mcpr_compress(compressed_data, encrypted_data, encrypted_data_length);
            if(compression_result == -1) return false;

            //VALGRIND_CHECK_VALUE_IS_DEFINED(compressed_data[0]);
            //VALGRIND_CHECK_VALUE_IS_DEFINED(encrypted_data[compression_result-1]);

            isize write_result = bstream_write(conn->io_stream, compressed_data, compression_result);
            return write_result != -1;
        }
        else
        {
            DEBUG_PRINT("Writing those bytes uncompressed.");
            isize write_result = bstream_write(conn->io_stream, encrypted_data, encrypted_data_length);
            return write_result != -1;
        }
    }
    else
    {
        DEBUG_PRINT("Writing those bytes unencrypted.");
        if(conn->use_compression)
        {
            DEBUG_PRINT("Writing those bytes compressed.");
            void *compressed_data = malloc(mcpr_compress_bounds(bytes));
            if(compressed_data == NULL) { ninerr_set_err(ninerr_from_errno()); return false; }

            ssize_t compression_result = mcpr_compress(compressed_data, in, bytes);
            if(compression_result == -1) { free(compressed_data); return false; }
            isize write_result = bstream_write(conn->io_stream, compressed_data, compression_result);
            free(compressed_data);
            return write_result != -1;
        }
        else
        {
            DEBUG_PRINT("Writing those bytes uncompressed.");
            return bstream_write(conn->io_stream, in, bytes);
        }
    }
}

bool mcpr_connection_write_packet(mcpr_connection *tmpconn, const struct mcpr_packet *pkt)
{
    DEBUG_PRINT("Writing packet (numerical ID: 0x%02x, state: %s) to mcpr_connection at address %p\n", mcpr_packet_type_to_byte(pkt->id), mcpr_state_to_string(pkt->state), tmpconn);
    void *buf;
    size_t pktlen;
    if(!mcpr_encode_packet(&buf, &pktlen, pkt)) return false;
    if(pktlen > INT32_MAX) { free(buf); ninerr_set_err(ninerr_arithmetic_new()); return false; }
    void *tmp = realloc(buf, pktlen + MCPR_VARINT_SIZE_MAX); // TODO this is inefficient
    if(tmp == NULL) { ninerr_set_err(ninerr_from_errno()); free(buf); return false; }
    buf = tmp;
    memmove(buf + mcpr_varint_bounds((int32_t) pktlen), buf, pktlen);
    size_t bytes_written_1 = mcpr_encode_varint(buf, (int32_t) pktlen);
    DEBUG_PRINT("Writing packet, total size: %zu, size before prefixed length: %zu\n", (size_t) (pktlen + bytes_written_1), pktlen);

    if(!mcpr_connection_write(tmpconn, buf, pktlen + bytes_written_1)) { free(buf); return false; }
    free(buf);
    return true;
}

void mcpr_connection_set_packet_handler(mcpr_connection *tmpconn, bool (*on_packet)(const struct mcpr_packet *pkt, mcpr_connection *conn))
{
    struct conn *conn = (struct conn *) tmpconn;
    conn->packet_handler = on_packet;
}

void mcpr_connection_set_crypto(mcpr_connection *tmpconn, EVP_CIPHER_CTX *ctx_encrypt, EVP_CIPHER_CTX *ctx_decrypt)
{
    struct conn *conn = (struct conn *) tmpconn;
    conn->ctx_encrypt = ctx_encrypt;
    conn->ctx_decrypt = ctx_decrypt;
}

void mcpr_connection_set_state(mcpr_connection *conn, enum mcpr_state state)
{
    ((struct conn *) conn)->state = state;
}

void mcpr_connection_set_compression(mcpr_connection *tmpconn, bool compression)
{
    struct conn *conn = (struct conn *) tmpconn;
    conn->use_compression = compression;
}

bool mcpr_connection_is_closed(mcpr_connection *conn)
{
    return ((struct conn *) conn)->is_closed;
}


#endif

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

#include <stdlib.h>
#include <stdint.h>

#ifdef MCPR_DO_LOGGING
    #include "../stronk.h"
#endif

#include <zlib.h>

#include "codec.h"
#include "mcpr.h"
#include "../util.h"
#include "../stack.h"



int mcpr_encrypt(void *out, const void *data, EVP_CIPHER_CTX ctx_encrypt, size_t len) {
    int writtenlen;
    if(unlikely(EVP_EncryptUpdate(&ctx_encrypt, (unsigned char *) out, &writtenlen, (unsigned char *) data, len) == 0)) {
        return -1;
    }
    return writtenlen;
}

int mcpr_decrypt(void *out, const void *data, EVP_CIPHER_CTX ctx_decrypt, size_t len) {
    int writtenlen;
    if(unlikely(EVP_DecryptUpdate(&ctx_decrypt, (unsigned char *) out, &writtenlen, (unsigned char *) data, len) == 0)) {
        return -1;
    }
    return writtenlen;
}



size_t mcpr_compress_bounds(size_t len) {
    return compressBound(len);
}

int mcpr_compress(void *out, const void *in, size_t n) {
    uLongf dest_len = compressBound(n);
    int result = compress((Bytef *) out, &dest_len, (Bytef *) in, n);


    if(result == Z_MEM_ERROR) {
        return -1;
    } else if(result == Z_BUF_ERROR) {
        return -1;
    } else {
        return dest_len; // Compressed out size.
    }
}

int mcpr_uncompress(void *out, const void *in, size_t max_out_size, size_t in_size) {
    uLongf dest_len = max_out_size;
    int result = uncompress((Bytef *) out, &dest_len, (Bytef *) in, in_size);


    if(result == Z_MEM_ERROR) {
        return -1;
    } else if(result == Z_BUF_ERROR) {
        return -1;
    } else if(result == Z_DATA_ERROR) {
        return -1;
    } else {
        return dest_len;
    }
}




int mcpr_write_packet(struct mcpr_connection *conn, struct mcpr_packet *pkt, bool force_no_compression) {
    size_t buf_len;


    uint8_t tmp[MCPR_VARINT_SIZE_MAX];
    int packet_id_required_len = mcpr_encode_varint(&tmp, pkt->id);
    if(packet_id_required_len < 0) {
        return -1;
    }

    if(conn->use_compression) {
        if(force_no_compression) {
            buf_len = MCPR_VARINT_SIZE_MAX + MCPR_VARINT_SIZE_MAX + MCPR_VARINT_SIZE_MAX + pkt->data_len;
        } else {
            buf_len = MCPR_VARINT_SIZE_MAX + MCPR_VARINT_SIZE_MAX + mcpr_compress_bounds(pkt->data_len + packet_id_required_len);
        }

        uint8_t *buf = malloc(buf_len);
        if(unlikely(buf == NULL)) {
            return -1;
        }
        uint8_t *bufpointer = buf;
        bufpointer += MCPR_VARINT_SIZE_MAX; // Skip first 5 bytes to reserve space for the packet length field, which will be added later.

        // Write uncompressed data length
        if(force_no_compression) {
            int bytes_written = mcpr_encode_varint(bufpointer, 0);
            if(bytes_written < 0) {
                return -1;
            }
            bufpointer += bytes_written;
        } else {

            // Write data length
            int bytes_written = mcpr_encode_varint(bufpointer, packet_id_required_len + pkt->data_len);
            if(bytes_written < 0) {
                return -1;
            }
            bufpointer += bytes_written;
        }



        // Write packet ID.
        int bytes_written_pktid = mcpr_encode_varint(bufpointer, pkt->id);
        if(bytes_written_pktid < 0) {
            return -1;
        }
        bufpointer += bytes_written_pktid;

        // Write data.
        memcpy(bufpointer, pkt->data, pkt->data_len);
        bufpointer += pkt->data_len; // Now points at the first byte after the data. WARNING! WE MAY NOT OWN THIS MEMORY! USE FOR CALCULATION ONLY!


        if(!force_no_compression) {
            // Point the bufpointer at the first byte of the packet ID.
            bufpointer -= bytes_written_pktid + pkt->data_len;


            // Compress data & packet ID. TODO can source and destination overlap?.. It isn't mentioned in the Zlib manuals :/
            // The following assumes they can.
            int compression_status = mcpr_compress(bufpointer, bufpointer, bytes_written_pktid + pkt->data_len);
            if(compression_status == -1) {
                return -1;
            }

            bufpointer += compression_status; // Now points at the first byte after the data. WARNING! WE MAY NOT OWN THIS MEMORY! USE FOR CALCULATION ONLY!
        }

        // Calculate and prefix whole packet with packet length
        int final_len = final_len = bufpointer - (buf + 5) + 1;
        uint8_t pkt_len[MCPR_VARINT_SIZE_MAX];
        int bytes_written_pktlen = mcpr_encode_varint(&pkt_len, final_len);
        if(bytes_written_pktlen < 0) {
            return -1;
        }
        memcpy(buf + (5 - bytes_written_pktlen), &pkt_len, bytes_written_pktlen);


        size_t total_length = bufpointer - (buf + (5 - bytes_written_pktlen));

        bufpointer = buf + (5 - bytes_written_pktlen);

        int write_status = mcpr_write_raw(conn, bufpointer, total_length);
        if(write_status < 0) {
            return -1;
        }

        free(buf);
        return write_status;
    } else {
        uint8_t *buf = malloc(MCPR_VARINT_SIZE_MAX + MCPR_VARINT_SIZE_MAX + pkt->data_len);
        if(unlikely(buf == NULL)) {
            return -1;
        }
        uint8_t *bufpointer = buf + MCPR_VARINT_SIZE_MAX; // Skip the first 5 bytes to reserve space for the Packet Length field.


        // Write packet ID
        int bytes_written_1 = mcpr_encode_varint(bufpointer, pkt->id);
        if(bytes_written_1 < 0) {
            return -1;
        }
        bufpointer += bytes_written_1;

        // Write data.
        memcpy(bufpointer, pkt->data, pkt->data_len);

        size_t total_length = (bufpointer + pkt->data_len) - (buf + MCPR_VARINT_SIZE_MAX);


        // Prefix it all with packet length.
        uint8_t tmppktlen[MCPR_VARINT_SIZE_MAX];
        int bytes_written_2 = mcpr_encode_varint(&tmppktlen, total_length);
        if(bytes_written_2 < 0) {
            return -1;
        }
        memcpy(buf + (MCPR_VARINT_SIZE_MAX - bytes_written_2), &tmppktlen, bytes_written_2);


        int write_status = mcpr_write_raw(conn, buf + (MCPR_VARINT_SIZE_MAX - bytes_written_2), total_length + bytes_written_2);
        if(unlikely(write_status < 0)) {
            return -1;
        }

        free(buf);
        return write_status;
    }

    return 0;
}

/*
 * Writes the contents of data to the socket.
 * Does encryption if encryption is enabled for the specified connection.
 * Returns the amount of bytes written or < 0 upon error.
 */
int mcpr_write_raw(const struct mcpr_connection *conn, const void *data, size_t len) {

    if(conn->use_encryption) {
        uint8_t *buf = malloc(len + conn->encryption_block_size - 1);
        if(unlikely(buf == NULL)) {
            return -1;
        }

        int encrypt_status = mcpr_encrypt(buf, data, conn->ctx_encrypt, len);
        if(encrypt_status < 0) {
            return -1;
        }

        ssize_t write_status = write(conn->sockfd, buf, encrypt_status);
        if(write_status == -1) {
            return -1;
        }
        free(buf);

        return write_status;
    } else {
        ssize_t write_status = write(conn->sockfd, data, len);
        if(write_status == -1) {
            return -1;
        }

        return write_status;
    }

}

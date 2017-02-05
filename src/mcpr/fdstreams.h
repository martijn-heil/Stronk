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

    streams.h - Encoding & decoding functions operating on streams.
*/


#ifndef MCPR_FD_STREAMS_H
#define MCPR_FD_STREAMS_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <unistd.h>

#include <jansson/jansson.h>
#include <uuid/uuid.h>

#include "mcpr.h"

ssize_t mcpr_fd_read_byte          (int8_t *out, int in);
ssize_t mcpr_fd_read_ubyte         (uint8_t *out, int in);
ssize_t mcpr_fd_read_short         (int16_t *out, int in);
ssize_t mcpr_fd_read_ushort        (uint16_t *out, int in);
ssize_t mcpr_fd_read_int           (int32_t *out, int in);
ssize_t mcpr_fd_read_long          (int64_t *out, int in);
ssize_t mcpr_fd_read_float         (float *out, int in);
ssize_t mcpr_fd_read_double        (double *out, int in);
ssize_t mcpr_fd_read_string        (char **out, int in);
ssize_t mcpr_fd_read_chat          (json_t **out, int in);
ssize_t mcpr_fd_read_varint        (int32_t *out, int in);
ssize_t mcpr_fd_read_varlong       (int64_t *out, int in);
ssize_t mcpr_fd_read_position      (struct mcpr_position *out, int in);
ssize_t mcpr_fd_read_angle         (int8_t *out, int in);
ssize_t mcpr_fd_read_uuid          (uuid_t out, int in);
ssize_t mcpr_fd_read_byte_array    (void *out, int in);


ssize_t mcpr_fd_write_byte         (int out, int8_t in);
ssize_t mcpr_fd_write_ubyte        (int out, uint8_t in);
ssize_t mcpr_fd_write_short        (int out, int16_t in);
ssize_t mcpr_fd_write_ushort       (int out, uint16_t in);
ssize_t mcpr_fd_write_int          (int out, int32_t in);
ssize_t mcpr_fd_write_long         (int out, int64_t in);
ssize_t mcpr_fd_write_float        (int out, float in);
ssize_t mcpr_fd_write_double       (int out, double in);
ssize_t mcpr_fd_write_string       (int out, const char *restrict in);
ssize_t mcpr_fd_write_chat         (int out, const json_t *in);
ssize_t mcpr_fd_write_varint       (int out, int32_t in);
ssize_t mcpr_fd_write_varlong      (int out, int64_t in);
ssize_t mcpr_fd_write_position     (int out, const struct mcpr_position *in);
ssize_t mcpr_fd_write_angle        (int out, int8_t in);
ssize_t mcpr_fd_write_uuid         (int out, uuid_t in);
ssize_t mcpr_fd_write_byte_array   (int out, void *in, size_t len);

// result should be free'd. NULL upon error
struct mcpr_packet *mcpr_read_packet(int in, bool use_compression, bool use_encryption, size_t encryption_block_size, EVP_CIPHER_CTX *ctx_decrypt);
ssize_t mcpr_write_packet(int out, struct mcpr_packet *pkt, bool use_compression, bool force_no_compression, bool use_encryption, size_t encryption_block_size, EVP_CIPHER_CTX *ctx_encrypt);


#endif
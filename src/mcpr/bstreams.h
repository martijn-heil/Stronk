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

    bstreams.h - Encoding & decoding functions operating on bstreams.
*/


#ifndef MCPR_BSTREAMS_H
#define MCPR_BSTREAMS_H

#include <stddef.h>
#include <stdint.h>

#include <sys/types.h>

#include <openssl/evp.h>
#include <ninuuid/ninuuid.h>
#include <ninio/bstream.h>

#include "mcpr/mcpr.h"

ssize_t mcpr_bstream_read_byte          (int8_t *out, struct bstream *in);
ssize_t mcpr_bstream_read_ubyte         (uint8_t *out, struct bstream *in);
ssize_t mcpr_bstream_read_short         (int16_t *out, struct bstream *in);
ssize_t mcpr_bstream_read_ushort        (uint16_t *out, struct bstream *in);
ssize_t mcpr_bstream_read_int           (int32_t *out, struct bstream *in);
ssize_t mcpr_bstream_read_long          (int64_t *out, struct bstream *in);
ssize_t mcpr_bstream_read_float         (float *out, struct bstream *in);
ssize_t mcpr_bstream_read_double        (double *out, struct bstream *in);
ssize_t mcpr_bstream_read_string        (char **out, struct bstream *in, size_t maxlen);
ssize_t mcpr_bstream_read_chat          (char **out, struct bstream *in, size_t maxlen);
ssize_t mcpr_bstream_read_varint        (int32_t *out, struct bstream *in);
ssize_t mcpr_bstream_read_varlong       (int64_t *out, struct bstream *in);
ssize_t mcpr_bstream_read_position      (struct mcpr_position *out, struct bstream *in);
ssize_t mcpr_bstream_read_angle         (int8_t *out, struct bstream *in);
ssize_t mcpr_bstream_read_uuid          (struct ninuuid *out, struct bstream *in);
ssize_t mcpr_bstream_read_byte_array    (void *out, struct bstream *in, size_t len);


ssize_t mcpr_bstream_write_byte         (struct bstream *out, int8_t in);
ssize_t mcpr_bstream_write_ubyte        (struct bstream *out, uint8_t in);
ssize_t mcpr_bstream_write_short        (struct bstream *out, int16_t in);
ssize_t mcpr_bstream_write_ushort       (struct bstream *out, uint16_t in);
ssize_t mcpr_bstream_write_int          (struct bstream *out, int32_t in);
ssize_t mcpr_bstream_write_long         (struct bstream *out, int64_t in);
ssize_t mcpr_bstream_write_float        (struct bstream *out, float in);
ssize_t mcpr_bstream_write_double       (struct bstream *out, double in);
ssize_t mcpr_bstream_write_string       (struct bstream *out, const char *in);
ssize_t mcpr_bstream_write_chat         (struct bstream *out, const char *json_in);
ssize_t mcpr_bstream_write_varint       (struct bstream *out, int32_t in);
ssize_t mcpr_bstream_write_varlong      (struct bstream *out, int64_t in);
ssize_t mcpr_bstream_write_position     (struct bstream *out, const struct mcpr_position *in);
ssize_t mcpr_bstream_write_angle        (struct bstream *out, int8_t in);
ssize_t mcpr_bstream_write_uuid         (struct bstream *out, struct ninuuid *in);
ssize_t mcpr_bstream_write_byte_array   (struct bstream *out, void *in, size_t len);

// result should be free'd. NULL upon error
struct mcpr_packet *mcpr_bstream_read_packet(int in, bool use_compression, bool use_encryption, size_t encryption_block_size, EVP_CIPHER_CTX *ctx_decrypt);
ssize_t mcpr_bstream_write_packet(int out, struct mcpr_packet *pkt, bool use_compression, bool force_no_compression, bool use_encryption, size_t encryption_block_size, EVP_CIPHER_CTX *ctx_encrypt);

#endif
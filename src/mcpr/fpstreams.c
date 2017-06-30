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

    streams.c - Encoding & decoding functions operating on streams.
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>

#include <jansson/jansson.h>
#include <ninuuid/ninuuid.h>

#include "mcpr/mcpr.h"
#include "mcpr/fpstreams.h"
#include "mcpr/crypto.h"


#define htonll(x) ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) ((1==ntohl(1)) ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))

#define hton16(x) htons(x)
#define hton32(x) htonl(x)
#define hton64(x) htonll(x)

#define ntoh16(x) ntohs(x)
#define ntoh32(x) ntohl(x)
#define ntoh64(x) ntohll(x)


FILE *mcpr_open_packet(struct mcpr_packet *pkt)
{
    return fmemopen(pkt->data, pkt->data_len, "r");
}


ssize_t mcpr_fpstream_read_byte(int8_t *out, FILE *in)
{
    char c = getc(in);
    if (c == EOF) return -1;
    *out = c;
    return sizeof(int8_t);
}

ssize_t mcpr_fpstream_read_ubyte(uint8_t *out, FILE *in)
{
    unsigned char c = getc(in);
    if (c == EOF) return -1;
    *out = c;
    return sizeof(uint8_t);
}

ssize_t mcpr_fpstream_read_short(int16_t *out, FILE *in)
{
    int16_t tmp;
    size_t status = fread(&tmp, sizeof(int16_t), 1, in);
    if (status != 1) return -1;
    *out = ntohs(tmp);
    return sizeof(int16_t);
}

ssize_t mcpr_fpstream_read_ushort(uint16_t *out, FILE *in)
{
    uint16_t tmp;
    size_t status = fread(&tmp, sizeof(uint16_t), 1, in);
    if (status != 1) return -1;
    *out = ntohs(tmp);
    return sizeof(uint16_t);
}

ssize_t mcpr_fpstream_read_int(int32_t *out, FILE *in)
{
    int32_t tmp;
    size_t status = fread(&tmp, sizeof(int32_t), 1, in);
    if (status != 1) return -1;
    *out = ntohl(tmp);
    return sizeof(int32_t);
}

ssize_t mcpr_fpstream_read_long(int64_t *out, FILE *in)
{
    int64_t tmp;
    size_t status = fread(&tmp, sizeof(int64_t), 1, in);
    if (status != 1) return -1;
    *out = ntohll(tmp);
    return sizeof(int64_t);
}

ssize_t mcpr_fpstream_read_float(float *out, FILE *in)
{
    // TODO
}

ssize_t mcpr_fpstream_read_double(double *out, FILE *in)
{
    // TODO
}

// TODO fix
ssize_t mcpr_fpstream_read_string(char **out, FILE *in)
{
    flockfile(in);
    int32_t len;
    ssize_t bytes_read_1 = mcpr_read_varint(&len, in);
    if (bytes_read_1 < 0) goto err;

    size_t bytes_read_2 = fread(out, len, 1, in);
    if(bytes_read_2 != 1) goto err;
    funlockfile(in);

    return bytes_read_1 + bytes_read_2;


    err:
        funlockfile(in);
        return -1;
}

ssize_t mcpr_fpstream_read_chat(char **out, FILE *in)
{
    return mcpr_read_string(out, in);
}

ssize_t mcpr_fpstream_read_varint(int32_t *out, FILE *in)
{
    unsigned int num_read = 0;
    int32_t result = 0;
    uint8_t read;

    flockfile(in);
    do
    {
        read = getc(in);
        if (read == EOF) goto err;
        int value = (read & 0x7F); // 0x7F == 0b01111111
        result |= (value << (7 * num_read));

        num_read++;
        if (num_read > 5)
        {
            goto err;
        }
    } while ((read & 0x80) != 0); // 0x80 == 0b10000000
    funlockfile(in);

    result = ntoh32(result);
    *out = result;
    return num_read;

    err:
        funlockfile(in);
        return -1;
}

ssize_t mcpr_fpstream_read_varlong(int64_t *out, FILE *in)
{
    unsigned int num_read = 0;
    int64_t result = 0;
    uint8_t read;

    flockfile(in);
    do
    {
        read = getc(in);
        if (read == EOF) return -1;

        int64_t value = (read & 0x7F); // 0x7F == 0b01111111
        result |= (value << (7 * num_read));

        num_read++;
        if (num_read > 10)
        {
            #ifdef MCPR_DO_LOGGING
                nlog_error("VarLong is longer than 10 bytes!");
            #endif
            return -1;
        }
    } while ((read & 0x80) != 0); // 0x80 == 0b10000000
    funlockfile(in);

    *out = result;
    return num_read;
}

ssize_t mcpr_fpstream_read_position(struct mcpr_position *out, FILE *in)
{
    int64_t iin;
    if(fread(&iin, sizeof(int64_t), 1, in) != 1) return -1;
    iin = ntoh64(iin);

    int64_t x = iin >> 38;
    int64_t y = (iin >> 26) & 0xFFF;
    int64_t z = iin << 38 >> 38;

    out->x = x;
    out->y = y;
    out->z = z;
    return sizeof(int64_t);
}


ssize_t mcpr_fpstream_read_angle     (int8_t *out, FILE *in)
{
    char tmp = getc(in);
    if (tmp == EOF) return -1;
    *out = tmp;
    return 1;
}

ssize_t mcpr_fpstream_read_uuid      (struct ninuuid *out, FILE *in)
{
    if (fread(out->bytes, 16, 1, in) != 1) return -1;
    return 16;
}


ssize_t mcpr_fpstream_write_byte     (FILE *out, int8_t in)
{
    if (fwrite(&in, 1, 1, out) != 1) return -1;
    fflush(out);
    return 1;
}

ssize_t mcpr_fpstream_write_ubyte    (FILE *out, uint8_t in)
{
    if (fwrite(&in, 1, 1, out) != 1) return -1;
    fflush(out);
    return 1;
}

ssize_t mcpr_fpstream_write_short    (FILE *out, int16_t in)
{
    in = hton16(in);
    if (fwrite(&in, sizeof(int16_t), 1, out) != 1) return -1;
    fflush(out);
    return sizeof(int16_t);
}

ssize_t mcpr_fpstream_write_ushort   (FILE *out, uint16_t in)
{
    in = hton16(in);
    if (fwrite(&in, sizeof(uint16_t), 1, out) != 1) return -1;
    fflush(out);
    return sizeof(uint16_t);
}

ssize_t mcpr_fpstream_write_int      (FILE *out, int32_t in)
{
    in = hton32(in);
    if (fwrite(&in, sizeof(int32_t), 1, out) != 1) return -1;
    fflush(out);
    return sizeof(int32_t);
}

ssize_t mcpr_fpstream_write_long     (FILE *out, int64_t in)
{
    in = hton64(in);
    if (fwrite(&in, sizeof(int64_t), 1, out) != 1) return -1;
    fflush(out);
    return sizeof(int64_t);
}

ssize_t mcpr_fpstream_write_float    (FILE *out, float in);
ssize_t mcpr_fpstream_write_double   (FILE *out, double in);
ssize_t mcpr_fpstream_write_string   (FILE *out, const char *restrict in);
ssize_t mcpr_fpstream_write_chat     (FILE *out, const char *in);
ssize_t mcpr_fpstream_write_varint   (FILE *out, int32_t in);
ssize_t mcpr_fpstream_write_varlong  (FILE *out, int64_t in);
ssize_t mcpr_fpstream_write_position (FILE *out, const struct mcpr_position *in);
ssize_t mcpr_fpstream_write_angle    (FILE *out, int8_t in);
ssize_t mcpr_fpstream_write_uuid     (FILE *out, struct ninuuid *in);

// TODO handle legacy server list ping
// TODO fix all
// struct mcpr_packet *mcpr_fpstream_read_packet(FILE *in, bool use_compression, bool force_no_compression,
//     bool use_encryption, size_t encryption_block_size, EVP_CIPHER_CTX *ctx_decrypt)
// {
//     flockfile(in);
//
//     // Read packet length.
//     // TODO encryption
//     int32_t pkt_len;
//
//     // Read encrypted varint.
//     {
//         unsigned int num_read = 0;
//         int32_t result = 0;
//         uint8_t read;
//         do {
//             mcpr_fread(in, &read, 1, use_compression, ctx_decrypt);
//             if (tmp == EOF) return -1;
//             int value = (read & 0x7F); // 0x7F == 0b01111111
//             result |= (value << (7 * num_read));
//
//             num_read++;
//             if (unlikely(num_read > 5))
//             {
//                 return -1;
//             }
//             else if (unlikely((num_read - 1) >= max_len))
//             {
//                 return -1;
//             }
//         } while ((read & 0x80) != 0); // 0x80 == 0b10000000
//
//         mcpr_fflush(in);
//         result = ntoh32(result);
//         pkt_len = result;
//     }
//
//
//     // Read raw packet after packet length field.
//     if (pkt_len < 0) return NULL;
//     if (pkt_len > SIZE_MAX) return NULL;
//     uint8_t *raw_packet = malloc((size_t) pkt_len);
//     if (raw_packet == NULL) return NULL;
//     if (fread(raw_packet, pkt_len, 1, in) != 1) { free(raw_packet); return NULL; }
//     funlockfile(in);
//
//     size_t decrypted_packet_len;
//     uint8_t *decrypted_packet;
//     if (use_encryption)
//     {
//         safe_add(&decrypted_packet_len, (size_t) pkt_len, (size_t) encryption_block_size)
//         decrypted_packet = malloc(decrypted_raw_packet_len);
//         if (decrypted_raw_packet == NULL) { free(raw_packet); return NULL; }
//
//         ssize_t bytes_written = mcpr_decrypt(decrypted_packet, raw_packet, ctx_decrypt, (size_t) pkt_len);
//         if (bytes_written < 0) { free(raw_packet); free(decrypted_packet); return NULL; }
//         decrypted_packet_len = (size_t) bytes_written;
//         free(raw_packet);
//     }
//     else
//     {
//         decrypted_packet = raw_packet;
//         decrypted_packet_len = (size_t) pkt_len;
//     }
//
//     struct mcpr_packet *pkt = mcpr_decode_packet(decrypted_packet, decrypted_packet_len, use_compression, force_no_compression);
//     if (pkt == NULL) { free(decrypted_packet); return NULL; }
//     free(decrypted_packet);
//
//     return pkt;
// }
//
// ssize_t mcpr_fpstream_write_packet(FILE *out, struct mcpr_packet *pkt, bool use_compression, bool force_no_compression, bool use_encryption, size_t encryption_block_size, EVP_CIPHER_CTX *ctx_encrypt)
// {
//
// }
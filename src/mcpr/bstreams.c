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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <fcntl.h>

#include <safe_math.h>
#include <openssl/evp.h>
#include <ninuuid/ninuuid.h>
#include <ninio/bstream.h>

#include "mcpr/codec.h"
#include "mcpr/mcpr.h"
#include "mcpr/bstreams.h"

#define htonll(x) ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) ((1==ntohl(1)) ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))

#define hton16(x) htons(x)
#define hton32(x) htonl(x)
#define hton64(x) htonll(x)

#define ntoh16(x) ntohs(x)
#define ntoh32(x) ntohl(x)
#define ntoh64(x) ntohll(x)


ssize_t mcpr_bstream_read_byte(int8_t *out, struct bstream *in)
{
    ssize_t tmp = bstream_read(in, out, 1);
    if(tmp == -1) { mcpr_errno = errno; return -1; }
    if(tmp != 1) { mcpr_errno = EIO; return -1; }
    return 1;
}

ssize_t mcpr_bstream_read_ubyte(uint8_t *out, struct bstream *in)
{
    ssize_t tmp = bstream_read(in, out, 1);
    if(tmp == -1) { mcpr_errno = errno; return -1; }
    if(tmp != 1) { mcpr_errno = EIO; return -1; }
    return 1;
}

ssize_t mcpr_bstream_read_short(int16_t *out, struct bstream *in)
{
    int16_t tmp;
    ssize_t bytes_read = bstream_read(in, &tmp, 2);
    if(bytes_read == -1) { mcpr_errno = errno; return -1; }
    if(bytes_read != 2) { mcpr_errno = EIO; return -1; }
    *out = ntoh16(tmp);
    return 2;
}

ssize_t mcpr_bstream_read_ushort(uint16_t *out, struct bstream *in)
{
    uint16_t tmp;
    ssize_t bytes_read = bstream_read(in, &tmp, 2);
    if(bytes_read == -1) { mcpr_errno = errno; return -1; }
    if(bytes_read != 2) { mcpr_errno = EIO; return -1; }

    *out = ntoh16(tmp);
    return 2;
}

ssize_t mcpr_bstream_read_int(int32_t *out, struct bstream *in)
{
    int32_t tmp;
    ssize_t bytes_read = bstream_read(in, &tmp, 4);
    if(bytes_read == -1) { mcpr_errno = errno; return -1; }
    if(bytes_read != 4) { mcpr_errno = EIO; return -1; }

    *out = ntoh32(tmp);
    return 4;
}

ssize_t mcpr_bstream_read_long(int64_t *out, struct bstream *in)
{
    int64_t tmp;
    if(bstream_read(in, &tmp, 8) == -1)
    {
        mcpr_errno = errno;
        return -1;
    }
    ssize_t bytes_read = bstream_read(in, &tmp, 8);
    if(bytes_read != 8)
    {
        mcpr_errno = errno;
        return -1;
    }

    *out = ntoh64(tmp);
    return 8;
}

ssize_t mcpr_bstream_read_float(float *out, struct bstream *in)
{
    ssize_t tmp = bstream_read(in, out, sizeof(float));
    if(tmp == -1) { mcpr_errno = errno; return -1; }
    if(tmp != sizeof(float)) { mcpr_errno = EIO; return -1; }
    return sizeof(float);
}

ssize_t mcpr_bstream_read_double(double *out, struct bstream *in)
{
    ssize_t tmp = bstream_read(in, out, sizeof(double));
    if(tmp == -1) { mcpr_errno = errno; return -1; }
    if(tmp != sizeof(double)) { mcpr_errno = EIO; return -1; }
    return sizeof(double);
}

ssize_t mcpr_bstream_read_string(char **out, struct bstream *in, size_t maxlen)
{
    int32_t len;
    ssize_t tmp_result = mcpr_bstream_read_varint(&len, in);
    if(tmp_result < 0) return -1;
    if(len < 0) { mcpr_errno = MCPR_EDECODE; return -1; }
    if(len > SIZE_MAX) { mcpr_errno = MCPR_EINTERNAL; return -1; }
    if(len > maxlen) { mcpr_errno = MCPR_EINTERNAL; return -1; }

    *out = malloc(len);
    if(*out == NULL) { return -1; }

    size_t read_bytes = bstream_read(in, *out, len);
    if(read_bytes == -1) { free(*out); mcpr_errno = errno; return -1; }
    if(read_bytes != len) { free(*out); mcpr_errno = EIO; return -1; }

    ssize_t result;
    if(!safe_add(&result, len, tmp_result)) { free(*out); mcpr_errno = MCPR_EINTERNAL; return -1; }
    return result;
}

ssize_t mcpr_bstream_read_chat(char **out, struct bstream *in, size_t maxlen)
{
    return mcpr_bstream_read_string(out, in, maxlen);
}

ssize_t mcpr_bstream_read_varint(int32_t *out, struct bstream *in)
{
    size_t i = 0;
    int32_t result = 0;
    uint8_t tmp;

    do
    {
        ssize_t read_bytes = bstream_read(in, &tmp, 1);
        if(read_bytes == -1) { mcpr_errno = errno; return -1; }
        if(read_bytes != 1) { mcpr_errno = EIO; return -1; }

        uint8_t value = (tmp & 0x7F); // 0x7F == 0b01111111
        result |= (value << (7 * i));

        i++;
        if (i > MCPR_VARINT_SIZE_MAX) // VarInt is longer than 5 bytes!
        {
            mcpr_errno = EIO;
            return -1;
        }
    } while ((tmp & 0x80) != 0); // 0x80 == 0b10000000

    *out = result;
    return i;
}

ssize_t mcpr_bstream_read_varlong(int64_t *out, struct bstream *in)
{
    size_t i = 0;
    int32_t result = 0;
    uint8_t tmp;

    do
    {
        ssize_t read_bytes = bstream_read(in, &tmp, 1);
        if(read_bytes == -1) { mcpr_errno = errno; return -1; }
        if(read_bytes != 1) { mcpr_errno = EIO; return -1; }

        if(read_bytes == -1)
        {
            mcpr_errno = errno;
            return -1;
        }
        else if(read_bytes != 1)
        {
            return -1;
        }

        uint8_t value = (tmp & 0x7F); // 0x7F == 0b01111111
        result |= (value << (7 * i));

        i++;
        if (i > MCPR_VARLONG_SIZE_MAX) // VarInt is longer than 5 bytes!
        {
            mcpr_errno = MCPR_EDECODE;
            return -1;
        }
    } while ((tmp & 0x80) != 0); // 0x80 == 0b10000000

    *out = result;
    return i;
}

ssize_t mcpr_bstream_read_position(struct mcpr_position *out, struct bstream *in)
{
    char buf[MCPR_POSITION_SIZE];
    ssize_t tmp = bstream_read(in, buf, MCPR_POSITION_SIZE);
    if(tmp == -1) { mcpr_errno = errno; return -1; }
    if(tmp != MCPR_POSITION_SIZE) { mcpr_errno = EIO; return -1; }

    if(mcpr_decode_position(out, buf) == -1) return -1;
    return MCPR_POSITION_SIZE;
}

ssize_t mcpr_bstream_read_angle(int8_t *out, struct bstream *in)
{
    ssize_t tmp = bstream_read(in, out, MCPR_ANGLE_SIZE);
    if(tmp == -1) { mcpr_errno = errno; return -1; }
    if(tmp != MCPR_ANGLE_SIZE) { mcpr_errno = EIO; return -1; }
    return tmp;
}

ssize_t mcpr_bstream_read_uuid(struct ninuuid *out, struct bstream *in)
{
    ssize_t tmp = bstream_read(in, out->bytes, MCPR_UUID_SIZE);
    if(tmp == -1) { mcpr_errno = errno; return -1; }
    if(tmp != MCPR_UUID_SIZE) { mcpr_errno = EIO; return -1; }
    return tmp;
}

ssize_t mcpr_bstream_read_byte_array(void *out, struct bstream *in, size_t len)
{
    ssize_t tmp = bstream_read(in, out, len);
    if(tmp == -1) { mcpr_errno = errno; return -1; }
    if(tmp != len) { mcpr_errno = EIO; return -1; }
    return tmp;
}


ssize_t mcpr_bstream_write_byte(struct bstream *out, int8_t in)
{
    ssize_t tmp = bstream_write(out, &in, MCPR_BYTE_SIZE);
    if(tmp == -1) { mcpr_errno = errno; return -1; }
    if(tmp != MCPR_BYTE_SIZE) { mcpr_errno = EIO; return -1; }
    return tmp;
}

ssize_t mcpr_bstream_write_ubyte(struct bstream *out, uint8_t in)
{
    ssize_t tmp = bstream_write(out, &in, MCPR_UBYTE_SIZE);
    if(tmp == -1) { mcpr_errno = errno; return -1; }
    if(tmp != MCPR_UBYTE_SIZE) { mcpr_errno = EIO; return -1; }
    return tmp;
}

ssize_t mcpr_bstream_write_short(struct bstream *out, int16_t in)
{
    in = hton16(in);
    ssize_t tmp = bstream_write(out, &in, MCPR_SHORT_SIZE);
    if(tmp == -1) { mcpr_errno = errno; return -1; }
    if(tmp != MCPR_SHORT_SIZE) { mcpr_errno = EIO; return -1; }
    return tmp;
}

ssize_t mcpr_bstream_write_ushort(struct bstream *out, uint16_t in)
{
    in = hton16(in);
    ssize_t tmp = bstream_write(out, &in, MCPR_USHORT_SIZE);
    if(tmp == -1) { mcpr_errno = errno; return -1; }
    if(tmp != MCPR_USHORT_SIZE) { mcpr_errno = EIO; return -1; }
    return tmp;
}

ssize_t mcpr_bstream_write_int(struct bstream *out, int32_t in)
{
    in = hton32(in);
    ssize_t tmp = bstream_write(out, &in, MCPR_INT_SIZE);
    if(tmp == -1) { mcpr_errno = errno; return -1; }
    if(tmp != MCPR_INT_SIZE) { mcpr_errno = EIO; return -1; }
    return tmp;
}

ssize_t mcpr_bstream_write_long(struct bstream *out, int64_t in)
{
    in = hton64(in);
    ssize_t tmp = bstream_write(out, &in, MCPR_LONG_SIZE);
    if(tmp == -1) { mcpr_errno = errno; return -1; }
    if(tmp != MCPR_LONG_SIZE) { mcpr_errno = EIO; return -1; }
    return tmp;
}

ssize_t mcpr_bstream_write_float(struct bstream *out, float in)
{
    ssize_t tmp = bstream_write(out, &in, sizeof(float));
    if(tmp == -1) { mcpr_errno = errno; return -1; }
    if(tmp != sizeof(float)) { mcpr_errno = EIO; return -1; }
    return tmp;
}

ssize_t mcpr_bstream_write_double(struct bstream *out, double in)
{
    ssize_t tmp = bstream_write(out, &in, sizeof(double));
    if(tmp == -1) { mcpr_errno = errno; return -1; }
    if(tmp != sizeof(double)) { mcpr_errno = EIO; return -1; }
    return tmp;
}

ssize_t mcpr_bstream_write_string(struct bstream *out, const char *in)
{
    size_t len = strlen(in);
    ssize_t tmp = mcpr_bstream_write_varint(out, len);
    if(tmp == -1) { mcpr_errno = errno; return -1; }
    if(tmp != len) { mcpr_errno = EIO; return -1; }

    ssize_t tmp2 = bstream_write(out, in, len);
    if(tmp2 == -1) { mcpr_errno = errno; return -1; }
    if(tmp2 != len) { mcpr_errno = EIO; return -1; }

    return tmp + tmp2;
}

ssize_t mcpr_bstream_write_chat(struct bstream *out, const char *json_in)
{
    return mcpr_bstream_write_string(out, json_in);
}

ssize_t mcpr_bstream_write_varint(struct bstream *out, int32_t in)
{
    size_t i = 0;
    do
    {
        uint8_t tmp = (uint8_t) (in & 0x0000007F);
        in = ((uint32_t) in)>>7;
        if(in != 0)
        {
            tmp |= 0x80;
        }

        ssize_t tmp2 = bstream_write(out, &tmp, 1);
        if(tmp2 == -1) { mcpr_errno = errno; return -1; }
        if(tmp2 != 1) { mcpr_errno = EIO; return -1; }
        i++;
    } while(in != 0);

    return i;
}

ssize_t mcpr_bstream_write_varlong(struct bstream *out, int64_t in)
{
    size_t i = 0;
    do
    {
        uint8_t tmp = (uint8_t) (in & 0x0000007F);
        in = ((uint64_t) in)>>7;
        if(in != 0)
        {
            tmp |= 0x80;
        }

        ssize_t tmp2 = bstream_write(out, &tmp, 1);
        if(tmp2 == -1) { mcpr_errno = errno; return -1; }
        if(tmp2 != 1) { mcpr_errno = EIO; return -1; }
        i++;
    } while(in != 0);

    return i;
}

ssize_t mcpr_bstream_write_position(struct bstream *out, const struct mcpr_position *in)
{
    char buf[MCPR_POSITION_SIZE];
    if(mcpr_encode_position(buf, in) == -1) return -1;
    ssize_t tmp = bstream_write(out, buf, MCPR_POSITION_SIZE);
    if(tmp == -1) { mcpr_errno = errno; return -1; }
    if(tmp != MCPR_POSITION_SIZE) { mcpr_errno = EIO; return -1; }
    return tmp;
}

ssize_t mcpr_bstream_write_angle(struct bstream *out, int8_t in)
{
    ssize_t tmp = bstream_write(out, &in, MCPR_ANGLE_SIZE);
    if(tmp == -1) { mcpr_errno = errno; return -1; }
    if(tmp != MCPR_ANGLE_SIZE) { mcpr_errno = EIO; return -1; }
    return tmp;
}

ssize_t mcpr_bstream_write_uuid(struct bstream *out, struct ninuuid *in)
{
    ssize_t tmp = bstream_write(out, in->bytes, MCPR_UUID_SIZE);
    if(tmp == -1) { mcpr_errno = errno; return -1; }
    if(tmp != MCPR_UUID_SIZE) { mcpr_errno = EIO; return -1; }
    return tmp;
}

ssize_t mcpr_bstream_write_byte_array(struct bstream *out, void *in, size_t len)
{
    ssize_t tmp = bstream_write(out, in, len);
    if(tmp == -1) { mcpr_errno = errno; return -1; }
    if(tmp != len) { mcpr_errno = EIO; return -1; }
    return tmp;
}

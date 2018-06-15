/*
    MIT License

    Copyright (c) 2016-2018 Martijn Heil

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



    codec.c - Encoding & decoding of Minecraft Protocol data.
*/

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <ninerr/ninerr.h>
#include <ninuuid/ninuuid.h>
#include <psnip/safe-math/safe-math.h>

#include "mcpr/codec.h"
#include "mcpr/mcpr.h"
#include "mcpr/util.h"




// TODO better logging

void mcpr_encode_bool(void *out, bool b)
{
    uint8_t tmp = (uint8_t) (b ? 0x01 : 0x00);
    memcpy(out, &tmp, sizeof(uint8_t));
}

void mcpr_encode_byte(void *out, int8_t byte)
{
    memcpy(out, &byte, sizeof(byte));
}

void mcpr_encode_ubyte(void *out, uint8_t byte)
{
    memcpy(out, &byte, sizeof(byte));
}

void mcpr_encode_short(void *out, int16_t i)
{
    i = hton16(i);
    memcpy(out, &i, sizeof(i));
}

void mcpr_encode_ushort(void *out, uint16_t i)
{
    i = hton16(i);
    memcpy(out, &i, sizeof(i));
}

void mcpr_encode_int(void *out, int32_t i)
{
    i = hton32(i);
    memcpy(out, &i, sizeof(i));
}

void mcpr_encode_long(void *out, int64_t i)
{
    i = hton64(i);
    memcpy(out, &i, sizeof(i));
}


ssize_t mcpr_encode_float(void *out, float f)
{
    uint32_t netfloat;
    memcpy(&netfloat, &f, 4);
    netfloat = hton64(netfloat);
    memcpy(out, &netfloat, 4);
    return 4;
}

ssize_t mcpr_encode_double(void *out, double d)
{
    uint64_t netdouble;
    memcpy(&netdouble, &d, 8);
    netdouble = hton64(netdouble);
    memcpy(out, &netdouble, 8);
    return 8;
}


ssize_t mcpr_encode_string(void *out, const char *utf8Str)
{
    size_t len = strlen(utf8Str);
    char *tmp = malloc(len * sizeof(char) + 1);
    if(tmp == NULL) { ninerr_set_err(ninerr_from_errno()); return -1; }


    strcpy(tmp, utf8Str);
    if(len > INT32_MAX) { ninerr_set_err(ninerr_arithmetic_new()); return -1; }
    int bytes_written = mcpr_encode_varint(out, (int32_t) len);

    #ifdef __GNUC__
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wpedantic"
    #endif
        memcpy(out + bytes_written, tmp, len); // doesn't copy the NUL byte, on purpose.
    #ifdef __GNUC__
        #pragma GCC diagnostic pop
    #endif

    free(tmp);
    bytes_written += len;
    return bytes_written;
}

ssize_t mcpr_encode_chat(void *out, const char *chat)
{
    return mcpr_encode_string(out, chat);
}

size_t mcpr_encode_varint(void *output, int32_t value)
{
    unsigned char *out = output;

    size_t i = 0;
    do
    {
        uint8_t tmp = (uint8_t) (value & 0x0000007F);
        value = ((uint32_t) value)>>7;
        if(value != 0)
        {
            tmp |= 0x80;
        }

        out[i] = (unsigned char) tmp;

        i++;
    } while(value != 0);

    return i;
}

size_t mcpr_varint_bounds(int32_t value)
{
    size_t i = 0;
    do
    {
        uint8_t tmp = (uint8_t) (value & 0x0000007F);
        value = ((uint32_t) value)>>7;
        if(value != 0)
        {
            tmp |= 0x80;
        }

        i++;
    } while(value != 0);

    return i;
}

size_t mcpr_encode_varlong(void *output, int64_t value) {
    unsigned char *out = output;

    size_t i = 0;
    do
    {
        uint8_t tmp = (uint8_t) (value & 0x0000007F);
        value = ((uint64_t) value)>>7;
        if(value != 0)
        {
            tmp |= 0x80;
        }

        out[i] = (unsigned char) tmp;

        i++;
    } while(value != 0);

    return i;
}

void mcpr_encode_position(void *out, const struct mcpr_position *in)
{
    int64_t pos = ((((*in).x & 0x3FFFFFF) << 38) | (((*in).y & 0xFFF) << 26) | ((*in).z & 0x3FFFFFF));
    mcpr_encode_long(out, pos);
}

void mcpr_encode_angle(void *out, uint8_t angle)
{
    mcpr_encode_ubyte(out, angle);
}

void mcpr_encode_uuid(void *out, const struct ninuuid *in)
{
    memcpy(out, in->bytes, 16);
}





void mcpr_decode_bool(bool *out, const void *in)
{
    uint8_t b;
    memcpy(&b, in, sizeof(b));
    if(b != 0x01 && b != 0x00) return;
    *out = (b == 0x01 ? true : false);
}

void mcpr_decode_byte(int8_t *out, const void *in)
{
    memcpy(out, in, sizeof(int8_t));
}

void mcpr_decode_ubyte(uint8_t *out, const void *in)
{
    memcpy(out, in, sizeof(uint8_t));
}

void mcpr_decode_short(int16_t *out, const void *in)
{
    memcpy(out, in, sizeof(int16_t));
    *out = ntoh16(*out);
}

void mcpr_decode_ushort(uint16_t *out, const void *in)
{
    memcpy(out, in, sizeof(uint16_t));
    *out = ntoh16(*out);
}

void mcpr_decode_int(int32_t *out, const void *in)
{
    memcpy(out, in, sizeof(int32_t));
    *out = ntoh32(*out);
}

void mcpr_decode_long(int64_t *out, const void *in)
{
    memcpy(out, in, sizeof(int64_t));
    *out = ntoh64(*out);
}

ssize_t mcpr_decode_float(float *out, const void *in)
{
    memcpy(out, in, sizeof(float));
    return sizeof(float);
}

ssize_t mcpr_decode_double(double *out, const void *in)
{
    memcpy(out, in, sizeof(double));
    return sizeof(double);
}

ssize_t mcpr_decode_string(char **out, const void *in, size_t maxlen)
{
    int32_t len;
    ssize_t bytes_read = mcpr_decode_varint(&len, in, maxlen);
    if(bytes_read < 0) { return -1; }
    if(len < 0) { ninerr_set_err(ninerr_new("Invalid string length in mcpr_decode_string().")); return -1; }
    if(len > MCPR_STR_MAX || ((uint32_t) len) >= SIZE_MAX) { ninerr_set_err(ninerr_arithmetic_new()); return -1; }
    *out = malloc(len + 1);
    if(*out == NULL) { ninerr_set_err(ninerr_from_errno()); return -1; }

    memcpy(*out, in + bytes_read, len);
    (*out)[len] = '\0';

    size_t final_bytes_read;
    if(!psnip_safe_add(&final_bytes_read, bytes_read, len)) { ninerr_set_err(ninerr_arithmetic_new()); return -1; }

    return final_bytes_read;
}

ssize_t mcpr_decode_chat(char **out, const void *in, size_t maxsize)
{
    return mcpr_decode_string(out, in, maxsize);
}

ssize_t mcpr_decode_varint(int32_t *out, const void *in, size_t max_len)
{
    size_t i = 0;
    int32_t result = 0;
    uint8_t tmp = 0;

    do
    {
        tmp = *((uint8_t *) (in + i));
        //memcpy(&tmp, in + i, 1);

        uint8_t value = (tmp & 0x7F); // 0x7F == 0b01111111
        result |= (value << (7 * i));

        i++;
        if (i > MCPR_VARINT_SIZE_MAX) // VarInt is longer than 5 bytes!
        {
            ninerr_set_err(ninerr_new("Varint size exceeded 5 bytes."));
            return -1;
        }
        else if (i > max_len) // Max length exceeded whilst decoding VarInt
        {
            ninerr_set_err(ninerr_new("Exceeded given max length whilst decoding varint."));
            return -1;
        }
    } while ((tmp & 0x80) != 0); // 0x80 == 0b10000000

    *out = result;
    return i;
}

ssize_t mcpr_decode_varlong(int64_t *out, const void *in, size_t max_len)
{
    size_t i = 0;
    int64_t result = 0;
    uint8_t tmp;

    do
    {
        memcpy(&tmp, in + i, 1);

        uint8_t value = (tmp & 0x7F); // 0x7F == 0b01111111
        result |= (value << (7 * i));

        i++;
        if (i > MCPR_VARLONG_SIZE_MAX) // VarInt is longer than 10 bytes!
        {
            ninerr_set_err(ninerr_new("Varlong size exceeded 10 bytes."));
            return -1;
        }
        else if (i > max_len) // Max length exceeded whilst decoding VarInt
        {
            ninerr_set_err(ninerr_new("Exceeded given max length whilst decoding varlong."));
            return -1;
        }
    } while ((tmp & 0x80) != 0); // 0x80 == 0b10000000

    *out = result;
    return i;
}

void mcpr_decode_position(struct mcpr_position *out, const void *in)
{
    int64_t iin;
    memcpy(&iin, in, sizeof(int64_t));
    iin = ntoh64(iin);

    int64_t x = iin >> 38;
    int64_t y = (iin >> 26) & 0xFFF;
    int64_t z = iin << 38 >> 38;

    out->x = x;
    out->y = y;
    out->z = z;
}

void mcpr_decode_angle(uint8_t *out, const void *in)
{
    mcpr_decode_ubyte(out, in);
}

void mcpr_decode_uuid(struct ninuuid *out, const void *in)
{
    memcpy(out, in, 16);
}

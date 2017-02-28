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



    codec.c - Encoding & decoding of Minecraft Protocol data.
*/

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>

#include <ninuuid/ninuuid.h>
#include <safe_math.h>

#include "mcpr/codec.h"
#include "util.h"

#ifdef MCPR_DO_LOGGING
    #include "stronk.h"
#endif

#include "mcpr/mcpr.h"

// TODO better logging

ssize_t mcpr_encode_bool(void *out, bool b)
{
    uint8_t tmp = (uint8_t) (b ? 0x01 : 0x00);
    memcpy(out, &tmp, sizeof(uint8_t));
    return sizeof(uint8_t);
}

ssize_t mcpr_encode_byte(void *out, int8_t byte)
{
    memcpy(out, &byte, sizeof(byte));
    return sizeof(byte);
}

ssize_t mcpr_encode_ubyte(void *out, uint8_t byte)
{
    memcpy(out, &byte, sizeof(byte));
    return sizeof(byte);
}

ssize_t mcpr_encode_short(void *out, int16_t i)
{
    hton(&i, sizeof(i));
    memcpy(out, &i, sizeof(i));
    return sizeof(i);
}

ssize_t mcpr_encode_ushort(void *out, uint16_t i)
{
    hton(&i, sizeof(i));
    memcpy(out, &i, sizeof(i));
    return sizeof(i);
}

ssize_t mcpr_encode_int(void *out, int32_t i)
{
    hton(&i, sizeof(i));
    memcpy(out, &i, sizeof(i));
    return sizeof(i);
}

ssize_t mcpr_encode_long(void *out, int64_t i)
{
    hton(&i, sizeof(i));
    memcpy(out, &i, sizeof(i));
    return sizeof(i);
}


ssize_t mcpr_encode_float(void *out, float f)
{
    hton(&f, sizeof(f));
    memcpy(out, &f, sizeof(f));
    return sizeof(f);
}

ssize_t mcpr_encode_double(void *out, double d)
{
    hton(&d, sizeof(d));
    memcpy(out, &d, sizeof(d));


    return sizeof(d);
}




ssize_t mcpr_encode_string(void *out, const char *utf8Str)
{
    size_t len = strlen(utf8Str);
    char *tmp = malloc(len * sizeof(char) + 1);
    if(unlikely(tmp == NULL))
    {
        #ifdef MCPR_DO_LOGGING
            nlog_error("Could not allocate memory (%s ?)", strerror(errno));
        #endif
        return -1;
    }


    strcpy(tmp, utf8Str);
    if(len > INT32_MAX) { return -1; }
    int bytes_written = mcpr_encode_varint(out, (int32_t) len);

    memcpy(out + bytes_written, tmp, len); // doesn't copy the NUL byte, on purpose.

    free(tmp);
    bytes_written += len;
    return bytes_written;
}

ssize_t mcpr_encode_chat(void *out, const json_t *root)
{
    char *chat = json_dumps(root, 0); // this should be free'd
    if(unlikely(chat == NULL))
    {
        #ifdef MCPR_DO_LOGGING
            nlog_error("Could not dump JSON to string (%s ?)", strerror(errno));
        #endif
        return -1;
    }
    int bytes_written = mcpr_encode_string(out, chat);
    free(chat);
    return bytes_written;
}

ssize_t mcpr_encode_varint(void *output, int32_t value)
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

    return (ssize_t) i;
}

ssize_t mcpr_varint_bounds(int32_t value)
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

    return (ssize_t) i;
}

ssize_t mcpr_encode_varlong(void *output, int64_t value) {
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

    return (ssize_t) i;
}

ssize_t mcpr_encode_position(void *out, const struct mcpr_position *in)
{
    int64_t pos = ((((*in).x & 0x3FFFFFF) << 38) | (((*in).y & 0xFFF) << 26) | ((*in).z & 0x3FFFFFF));
    return mcpr_encode_long(out, pos);
}

ssize_t mcpr_encode_angle(void *out, uint8_t angle)
{
    return mcpr_encode_ubyte(out, angle);
}

ssize_t mcpr_encode_uuid(void *out, const struct ninuuid *in)
{
    memcpy(out, in->bytes, 16);
    return 16;
}





ssize_t mcpr_decode_bool(bool *out, const void *in)
{
    uint8_t b;
    memcpy(&b, in, sizeof(b));
    *out = (b == 0x01 ? true : false);
    return sizeof(b);
}

ssize_t mcpr_decode_byte(int8_t *out, const void *in)
{
    memcpy(out, in, sizeof(int8_t));
    return sizeof(int8_t);
}

ssize_t mcpr_decode_ubyte(uint8_t *out, const void *in)
{
    memcpy(out, in, sizeof(uint8_t));
    return sizeof(uint8_t);
}

ssize_t mcpr_decode_short(int16_t *out, const void *in)
{
    memcpy(out, in, sizeof(int16_t));
    ntoh(out, sizeof(int16_t));
    return sizeof(int16_t);
}

ssize_t mcpr_decode_ushort(uint16_t *out, const void *in)
{
    memcpy(out, in, sizeof(uint16_t));
    ntoh(out, sizeof(uint16_t));
    return sizeof(uint16_t);
}

ssize_t mcpr_decode_int(int32_t *out, const void *in)
{
    memcpy(out, in, sizeof(int32_t));
    ntoh(out, sizeof(int32_t));
    return sizeof(int32_t);
}

ssize_t mcpr_decode_long(int64_t *out, const void *in)
{
    memcpy(out, in, sizeof(int64_t));
    ntoh(out, sizeof(int64_t));
    return sizeof(int64_t);
}

ssize_t mcpr_decode_float(float *out, const void *in)
{
    memcpy(out, in, sizeof(float));
    ntoh(out, sizeof(float));
    return sizeof(float);
}

ssize_t mcpr_decode_double(double *out, const void *in)
{
    memcpy(out, in, sizeof(double));
    ntoh(out, sizeof(double));
    return sizeof(double);
}

// TODO detect len automatically
ssize_t mcpr_decode_string(char **out, const void *in, size_t maxlen)
{
    int32_t len;
    ssize_t bytes_read = mcpr_decode_varint(&len, in, maxlen);
    if(bytes_read < 0) return -1;
    if(len < 0) return -1;
    if(len > MCPR_STR_MAX || ((uint32_t) len) >= SIZE_MAX) { return -1; }

    memcpy(out, in, len);
    out[len] = '\0';

    size_t final_bytes_read;
    if(!safe_add(&final_bytes_read, bytes_read, len)) return -1;

    return final_bytes_read;
}

ssize_t mcpr_decode_chat(json_t **out, const void *in, size_t maxsize)
{
    char *buf;
    ssize_t decode_string_result = mcpr_decode_string(&buf, in, maxsize);
    if(decode_string_result < 0) return -1;

    json_error_t err;
    *out = json_loads(buf, 0, &err);
    free(buf); // important we do this exactly here, we don't need buf anymore, but it has to be free'd in case of error return on the next line.
    if(unlikely(*out == NULL))
    {
        #ifdef MCPR_DO_LOGGING
            nlog_error("Could not load JSON string. (%s ?)", strerror(errno));
        #endif

        return -1;
    }

    return decode_string_result;
}

// TODO fix
ssize_t mcpr_decode_varint(int32_t *out, const void *in, size_t max_len)
{ // TODO add max read
    unsigned int num_read = 0;
    int32_t result = 0;
    uint8_t read;
    do
    {
        DO_GCC_PRAGMA(GCC diagnostic push)
        DO_GCC_PRAGMA(GCC diagnostic ignored "-Wpointer-arith")
            memcpy(&read, in + num_read, 1);
        DO_GCC_PRAGMA(GCC diagnostic pop)

        int value = (read & 0x7F); // 0x7F == 0b01111111
        result |= (value << (7 * num_read));

        num_read++;
        if (unlikely(num_read > 5))
        {
            #ifdef MCPR_DO_LOGGING
                nlog_error("VarInt is longer than 5 bytes!");
            #endif
            return -1;
        }
        else if (unlikely((num_read - 1) >= max_len))
        {
            #ifdef MCPR_DO_LOGGING
                nlog_error("Max length (%zu) exceeded whilst decoding VarInt", max_len);
            #endif
            return -1;
        }
    } while ((read & 0x80) != 0); // 0x80 == 0b10000000

    ntoh(&result, sizeof(result));
    *out = result;
    return num_read;
}

// TODO fix
ssize_t mcpr_decode_varlong(int64_t *out, const void *in, size_t max_len)
{
    unsigned int num_read = 0;
    int64_t result = 0;
    uint8_t read;
    do {
        memcpy(&read, in + num_read, 1);

        int64_t value = (read & 0x7F); // 0x7F == 0b01111111
        result |= (value << (7 * num_read));

        num_read++;
        if (unlikely(num_read > 10))
        {
            #ifdef MCPR_DO_LOGGING
                nlog_error("VarLong is longer than 10 bytes!");
            #endif
            return -1;
        }
        else if (unlikely((num_read - 1) >= max_len))
        {
            #ifdef MCPR_DO_LOGGING
                nlog_error("Max length (%zu) exceeded whilst decoding VarLong", max_len);
            #endif
            return -1;
        }
    } while ((read & 0x80) != 0); // 0x80 == 0b10000000

    *out = result;
    return num_read;
}

ssize_t mcpr_decode_position(struct mcpr_position *out, const void *in)
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
    return sizeof(int64_t);
}

ssize_t mcpr_decode_angle(uint8_t *out, const void *in)
{
    return mcpr_decode_ubyte(out, in);
}

ssize_t mcpr_decode_uuid(struct ninuuid *out, const void *in)
{
    memcpy(out, in, 16);
    return 16;
}

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#include <uuid/uuid.h>

#include "codec.h"
#include "../util.h"

#ifdef MCPR_DO_LOGGING
    #include "../stronk.h"
#endif


int mcpr_encode_bool(void *out, bool b) {
    uint8_t tmp = (b ? 0x01 : 0x00);
    memcpy(out, &tmp, sizeof(uint8_t));
    return sizeof(uint8_t);
}

int mcpr_encode_byte(void *out, int8_t byte) {
    memcpy(out, &byte, sizeof(byte));
    return sizeof(byte);
}

int mcpr_encode_ubyte(void *out, uint8_t byte) {
    memcpy(out, &byte, sizeof(byte));
    return sizeof(byte);
}

int mcpr_encode_short(void *out, int16_t i) {
    hton(&i, sizeof(i));
    memcpy(out, &i, sizeof(i));
    return sizeof(i);
}

int mcpr_encode_ushort(void *out, uint16_t i) {
    hton(&i, sizeof(i));
    memcpy(out, &i, sizeof(i));
    return sizeof(i);
}

int mcpr_encode_int(void *out, int32_t i) {
    hton(&i, sizeof(i));
    memcpy(out, &i, sizeof(i));
    return sizeof(i);
}

int mcpr_encode_long(void *out, int64_t i) {
    hton(&i, sizeof(i));
    memcpy(out, &i, sizeof(i));
    return sizeof(i);
}

int mcpr_encode_float(void *out, float f) {
    hton(&f, sizeof(f));
    memcpy(out, &f, sizeof(f));
    return sizeof(f);
}

int mcpr_encode_double(void *out, double d) {
    hton(&d, sizeof(d));
    memcpy(out, &d, sizeof(d));
    return sizeof(d);
}

int mcpr_encode_string(void *out, const char *utf8Str) {
    size_t len = strlen(utf8Str);
    char *tmp = malloc(len * sizeof(char) + 1);
    if(unlikely(tmp == NULL)) {
        #ifdef MCPR_DO_LOGGING
            nlog_error("Could not allocate memory (%s ?)", strerror(errno));
        #endif
        return -1;;
    }
    strcpy(tmp, utf8Str);
    size_t bytes_written = mcpr_encode_varint(out, len);

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpointer-arith"
        memcpy(out + bytes_written, tmp, len); // doesn't copy the NUL byte, on purpose.
    #pragma GCC diagnostic pop

    free(tmp);
    bytes_written += len;
    return bytes_written;
}

int mcpr_encode_chat(void *out, const json_t *root) {
    char *chat = json_dumps(root, 0); // this should be free'd
    if(unlikely(chat == NULL)) {
        #ifdef MCPR_DO_LOGGING
            nlog_error("Could not dump JSON to string (%s ?)", strerror(errno));
        #endif
        return -1;
    }
    size_t bytes_written = mcpr_encode_string(out, chat);
    free(chat);
    return bytes_written;
}

int mcpr_encode_varint(void *out, int32_t value) {
    size_t i = 0;
    hton(&value, sizeof(value));

    do {
        uint8_t temp = (uint8_t)(value & 0x7F); // 0x7F == 0b01111111

        value = ((uint32_t) value)>>7;
        if (value != 0) {
            temp |= 0x80; // 0x80 == 0b10000000
        }

        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wpointer-arith"
            memcpy((out + i), &temp, sizeof(temp));
        #pragma GCC diagnostic pop

        i++;
    } while (value != 0);

    return i;
}

int mcpr_encode_varlong(void *out, int64_t value) {
    size_t i = 0;
    hton(&value, sizeof(value));

    do {
        uint8_t temp = (uint8_t)(value & 0x7F); // 0x7F == 0b01111111

        value = ((uint32_t) value)>>7;
        if (value != 0) {
            temp |= 0x80; // 0x80 == 0b10000000
        }
        ((uint8_t *) out)[i] = temp;
        i++;
    } while (value != 0);

    return i;
}

int mcpr_encode_position(void *out, const struct mcpr_position *in) {

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wshift-count-overflow"
        int64_t pos = ((((*in).x & 0x3FFFFFF) << 38) | (((*in).y & 0xFFF) << 26) | ((*in).z & 0x3FFFFFF));
    #pragma GCC diagnostic pop

    return mcpr_encode_long(out, pos);
}

int mcpr_encode_angle(void *out, uint8_t angle) {
    return mcpr_encode_ubyte(out, angle);
}

int mcpr_encode_uuid(void *out, uuid_t in) {
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wsizeof-array-argument"

        memcpy(out, in, 16 * sizeof(in));
        hton(out, 16 * sizeof(in));
        return 16 * sizeof(in);

    #pragma GCC diagnostic pop
}





int mcpr_decode_bool(bool *out, const void *in) {
    uint8_t b;
    memcpy(&b, in, sizeof(b));
    *out = (b == 0x01 ? true : false);
    return sizeof(b);
}

int mcpr_decode_byte(int8_t *out, const void *in) {
    memcpy(out, in, sizeof(int8_t));
    return sizeof(int8_t);
}

int mcpr_decode_ubyte(uint8_t *out, const void *in) {
    memcpy(out, in, sizeof(uint8_t));
    return sizeof(uint8_t);
}

int mcpr_decode_short(int16_t *out, const void *in) {
    memcpy(out, in, sizeof(int16_t));
    ntoh(out, sizeof(int16_t));
    return sizeof(int16_t);
}

int mcpr_decode_ushort(uint16_t *out, const void *in) {
    memcpy(out, in, sizeof(uint16_t));
    ntoh(out, sizeof(uint16_t));
    return sizeof(uint16_t);
}

int mcpr_decode_int(int32_t *out, const void *in) {
    memcpy(out, in, sizeof(int32_t));
    ntoh(out, sizeof(int32_t));
    return sizeof(int32_t);
}

int mcpr_decode_long(int64_t *out, const void *in) {
    memcpy(out, in, sizeof(int64_t));
    ntoh(out, sizeof(int64_t));
    return sizeof(int64_t);
}

int mcpr_decode_float(float *out, const void *in) {
    memcpy(out, in, sizeof(float));
    ntoh(out, sizeof(float));
    return sizeof(float);
}

int mcpr_decode_double(double *out, const void *in) {
    memcpy(out, in, sizeof(double));
    ntoh(out, sizeof(double));
    return sizeof(double);
}

int mcpr_decode_string(char *out, const void *in, int32_t len) {
    // 2147483652 is the max string size allowed by the Minecraft Protocol.

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wtype-limits"
        if(len > MCPR_STR_MAX || ((uint32_t) len) >= SIZE_MAX) { return -1; }
    #pragma GCC diagnostic pop
    if(len <= 0) {
        #ifdef MCPR_DO_LOGGING
            nlog_error("len for mcpr_decode_string() is less than or equal to 0.");
            return -1;
        #endif
    }

    memcpy(out, in, len);
    out[len] = '\0';

    return len;
}

int mcpr_decode_chat(json_t **out, const void *in) {
    int32_t len;
    int bytes_read = mcpr_decode_varint(&len, in, 5);
    if(bytes_read < 0) {
        #ifdef MCPR_DO_LOGGING
            nlog_error("Error whilst decoding varint.");
        #endif
        return bytes_read;
    }

    // Do lot's of safety checks, arithmetic overflow and such things.
    if(len < 1) {
        #ifdef MCPR_DO_LOGGING
            nlog_error("Length of string is less than 1.");
        #endif
        return -1;
    }
    uint32_t ulen = (uint32_t) len; // len is guaranteed to be positive at this point.
    if(ulen == UINT32_MAX) {
        #ifdef MCPR_DO_LOGGING
            nlog_error("uint32 arithmetic overflow error.");
        #endif
        return -1;;
    } // Adding the extra 1 for a NUL byte would overflow.

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wtype-limits"
        if(unlikely((ulen + 1) > SIZE_MAX)) {
            #ifdef MCPR_DO_LOGGING
                nlog_error("Length of string does not fit iside size_t, arithmetic overflow error.");
            #endif

            return -1;
        } // The result would not fit in size_t, so we can't malloc it.
    #pragma GCC diagnostic pop

    size_t bufsize;
    if(unlikely(safe_mul(&bufsize, (size_t) (ulen + 1), sizeof(char)))) { return -1; }



    char *buf = malloc(bufsize);
    if(unlikely(buf == NULL)) {
        #ifdef MCPR_DO_LOGGING
            nlog_error("Could not allocate memory for buffer. (%s ?)", strerror(errno));
        #endif
        return -1;
    }

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpointer-arith"
        int status = mcpr_decode_string(buf, (in + bytes_read), len);
    #pragma GCC diagnostic pop

    if(unlikely(status < 0)) {
        free(buf);
        return status;
    }

    json_error_t err;
    *out = json_loads(buf, 0, &err);
    free(buf); // important we do this exactly here, we don't need buf anymore, but it has to be free'd in case of error return on the next line.
    if(unlikely(*out == NULL)) {
        #ifdef MCPR_DO_LOGGING
            nlog_error("Could not load JSON string. (%s ?)", strerror(errno));
        #endif

        return -1;
    }

    int result;
    if(unlikely(safe_add(&result, bytes_read, (int) len))) {
        #ifdef MCPR_DO_LOGGING
            nlog_error("Safe add (result = bytes_read + len) failed. Arithmetic overflow error.");
        #endif
        return -1;
    }
    return result;
}

int mcpr_decode_varint(int32_t *out, const void *in, size_t max_len) { // TODO add max read
    unsigned int num_read = 0;
    int32_t result = 0;
    uint8_t read;
    do {
        DO_GCC_PRAGMA(GCC diagnostic push)
        DO_GCC_PRAGMA(GCC diagnostic ignored "-Wpointer-arith")
            memcpy(&read, in + num_read, 1);
        DO_GCC_PRAGMA(GCC diagnostic pop)

        int value = (read & 0x7F); // 0x7F == 0b01111111
        result |= (value << (7 * num_read));

        num_read++;
        if (unlikely(num_read > 5)) {
            #ifdef MCPR_DO_LOGGING
                nlog_error("VarInt is longer than 5 bytes!");
            #endif
            return -1;
        } else if (unlikely((num_read - 1) >= max_len)) {
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

int mcpr_decode_varlong(int64_t *out, const void *in, size_t max_len) {
    unsigned int num_read = 0;
    int64_t result = 0;
    uint8_t read;
    do {
        DO_GCC_PRAGMA(GCC diagnostic push)
        DO_GCC_PRAGMA(GCC diagnostic ignored "-Wpointer-arith")
            memcpy(&read, in + num_read, 1);
        DO_GCC_PRAGMA(GCC diagnostic pop)

        int64_t value = (read & 0x7F); // 0x7F == 0b01111111
        result |= (value << (7 * num_read));

        num_read++;
        if (unlikely(num_read > 10)) {
            #ifdef MCPR_DO_LOGGING
                nlog_error("VarLong is longer than 10 bytes!");
            #endif
            return -1;
        } else if (unlikely((num_read - 1) >= max_len)) {
            #ifdef MCPR_DO_LOGGING
                nlog_error("Max length (%zu) exceeded whilst decoding VarLong", max_len);
            #endif
            return -1;
        }
    } while ((read & 0x80) != 0); // 0x80 == 0b10000000

    *out = result;
    return num_read;
}

int mcpr_decode_position(struct mcpr_position *out, const void *in) {
    int64_t iin;
    memcpy(&iin, in, sizeof(int64_t));
    ntoh(&iin, sizeof(int64_t));

    int64_t x = iin >> 38;
    int64_t y = (iin >> 26) & 0xFFF;
    int64_t z = iin << 38 >> 38;

    out->x = x;
    out->y = y;
    out->z = z;
    return sizeof(int64_t);
}

int mcpr_decode_angle(uint8_t *out, const void *in) {
    return mcpr_decode_ubyte(out, in);
}

int mcpr_decode_uuid(uuid_t out, const void *in) {
    DO_GCC_PRAGMA(GCC diagnostic push)
    DO_GCC_PRAGMA(GCC diagnostic ignored "-Wsizeof-array-argument")

        // This is kinda hacky.. uuid_t is USUALLY a typedef'd unsigned char raw[16]
        // If there is an implementation which does not define uuid_t as unsigned char raw[16] we have a bit of an issue here.
        uuid_t uuid;
        memcpy(uuid, in, 16 * sizeof(uuid_t));
        ntoh(uuid, 16 * sizeof(uuid_t));
        uuid_copy(out, uuid);
        return 16 * sizeof(uuid_t);

    DO_GCC_PRAGMA(GCC diagnostic pop)
}

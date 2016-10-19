#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <arpa/inet.h>

#include <jansson/jansson.h>

#include "mcpr.h"
#include "util.h"

int mcpr_encode_bool(void *out, bool b) {
    *((uint8_t *) out) = (b ? 0x01 : 0x00);
    return 1;
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

    size_t bytes_written = mcpr_encode_varint(out, strlen(utf8Str));
    memcpy(out + bytes_written, utf8Str, len); // doesn't copy the NUL byte, on purpose.
    bytes_written += len;

    return bytes_written;
}

int mcpr_encode_chat(void *out, const json_t *root) {
    char *chat = json_dumps(root, 0); // this should be free'd
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
        ((uint8_t *) out)[i] = temp;
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




int mcpr_decode_bool(bool *out, void *in) {
    *out = (*((uint8_t *) out)) == 0x01 ? true: false;
    return 1;
}

int mcpr_decode_byte(int8_t *out, void *in) {
    memcpy(out, in, sizeof(int8_t));
    return sizeof(int8_t);
}

int mcpr_decode_ubyte(uint8_t *out, void *in) {
    memcpy(out, in, sizeof(uint8_t));
    return sizeof(uint8_t);
}

int mcpr_decode_short(int16_t *out, void *in) {
    memcpy(out, in, sizeof(int16_t));
    ntoh(out, sizeof(int16_t));
    return sizeof(int16_t);
}

int mcpr_decode_ushort(uint16_t *out, void *in) {
    memcpy(out, in, sizeof(uint16_t));
    ntoh(out, sizeof(uint16_t));
    return sizeof(uint16_t);
}

int mcpr_decode_int(int32_t *out, void *in) {
    memcpy(out, in, sizeof(int32_t));
    ntoh(out, sizeof(int32_t));
    return sizeof(int32_t);
}

int mcpr_decode_long(int64_t *out, void *in) {
    memcpy(out, in, sizeof(int64_t));
    ntoh(out, sizeof(int64_t));
    return sizeof(int64_t);
}

int mcpr_decode_float(float *out, void *in) {
    memcpy(out, in, sizeof(float));
    ntoh(out, sizeof(float));
    return sizeof(float);
}

int mcpr_decode_double(double *out, void *in) {
    memcpy(out, in, sizeof(double));
    ntoh(out, sizeof(double));
    return sizeof(double);
}

int mcpr_decode_string(char **out, void *in) {
    int32_t len;
    int32_t bytes_read = mcpr_decode_varint(&len, in);
    // 2147483652 is the max string size allowed by the Minecraft Protocol.
    if(len > MCPR_STR_MAX || ((uint32_t) len) >= SIZE_MAX)
    *out = realloc(*out, len * sizeof(char) + 1);
    if(*out == NULL) { return -1; }
    memcpy(*out, (in + bytes_read), len);
    ntoh(out, len * sizeof(char));
    (*out)[len] = '\0';

    return bytes_read + len + 1;
}

int mcpr_decode_chat(char **out, void *in) {
    return mcpr_decode_string(out, in);
}

int mcpr_decode_varint(int32_t *out, void *in) {
    uint8_t *bytes = (uint8_t *) in;

    unsigned int num_read = 0;
    int32_t result = 0;
    uint8_t read;
    do {
        read = bytes[num_read];
        int value = (read & 0x7F); // 0x7F == 0b01111111
        result |= (value << (7 * num_read));

        num_read++;
        if (num_read > 5) {
            return -1;
        }
    } while ((read & 0x80) != 0); // 0x80 == 0b10000000

    ntoh(&result, sizeof(result));
    *out = result;
    return num_read;
}

int mcpr_decode_varlong(int64_t *out, void *in) {
    uint8_t *bytes = (uint8_t *) in;

    unsigned int num_read = 0;
    int64_t result = 0;
    uint8_t read;
    do {
        read = bytes[num_read];
        int64_t value = (read & 0x7F); // 0x7F == 0b01111111
        result |= (value << (7 * num_read));

        num_read++;
        if (num_read > 10) {
            return -1;
        }
    } while ((read & 0x80) != 0); // 0x80 == 0b10000000

    *out = result;
    return num_read;
}

// data should be free'd by the receiver.
// data is just the raw packet bytes, including the packet id and everything else.
void mcpr_on_pkt(uint8_t pkt_id, void (*on_packet)(uint8_t *data)) {

}

void mcpr_on_any_pkt(void (*on_packet)(uint8_t *data)) {

}

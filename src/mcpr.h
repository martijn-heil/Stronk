#ifndef MCPR_H
#define MCPR_H

#include <stdbool.h>
#include <stdint.h>

#include <arpa/inet.h>

#include <jansson/jansson.h>

/*
    Minecraft Protocol (http://wiki.vg/Protocol)
    MCPR = MineCraft PRotocol.
*/

#define MCPR_PROTOCOL_VERSION 210

#define MCPR_STR_MAX 2147483652


// Encoding/decoding functions return the amount of bytes written for encode, and amount of
// bytes read for decode. On error; they return -1

int mcpr_encode_bool    (void *out, bool b);         // writes 1 byte
int mcpr_encode_byte    (void *out, int8_t byte);    // writes 1 byte
int mcpr_encode_ubyte   (void *out, uint8_t byte);   // writes 1 byte
int mcpr_encode_short   (void *out, int16_t i);      // writes 2 bytes
int mcpr_encode_ushort  (void *out, uint16_t i);     // writes 2 bytes
int mcpr_encode_int     (void *out, int32_t i);      // writes 4 bytes
int mcpr_encode_long    (void *out, int64_t i);      // writes 8 bytes
int mcpr_encode_float   (void *out, float f);        // writes 4 bytes
int mcpr_encode_double  (void *out, double d);       // writes 8 bytes



/*
    ------------------- WARNING -----------------
    THE FOLLOWING FUNCTIONS SHOULD BE USED WITH CARE!
*/

 /*
    Make sure the out buffer is (strlen(utf8Str) + 5)
    Note that it is not guaranteed to write  (strlen(utf8Str) + 5) bytes, it can be less,
    check the returned value for how many bytes exactly were written. (It will always write the whole string)

    returns the amount of bytes written.
 */
int mcpr_encode_string  (void *out, const char *utf8Str);

/*
    out should at least be the size of (strlen(json_dumps(root, 0)) + 5).
    Dont forget to free that returned string from json_dumps..

    Note that this function is not guaranteed to write exactly (strlen(json_dumps(root, 0)) + 5) bytes,
    it can write less bytes, check the return for how many bytes were exactly written.

    returns the amount of bytes written.
*/
int mcpr_encode_chat    (void *out, const json_t *root);


int mcpr_encode_varint  (void *out, int32_t i);
int mcpr_encode_varlong (void *out, int64_t i);

int mcpr_decode_bool    (bool *out, void *in);
int mcpr_decode_byte    (int8_t *out, void *in);
int mcpr_decode_ubyte   (uint8_t *out, void *in);
int mcpr_decode_short   (int16_t *out, void *in);
int mcpr_decode_ushort  (uint16_t *out, void *in);
int mcpr_decode_int     (int32_t *out, void *in);
int mcpr_decode_long    (int64_t *out, void *in);
int mcpr_decode_float   (float *out, void *in);
int mcpr_decode_double  (double *out, void *in);
int mcpr_decode_string  (char **out, void *in); // Will write a NUL terminated UTF-8 string to the buffer. out will be realloc'ed in all cases. Returns -1 on error.
int mcpr_decode_chat    (char **out, void *in); // Should be free'd. out will be realloc'ed in all cases. returns -1 if memory could not be allocated or string is too big.
int mcpr_decode_varint  (int32_t *out, void *in);
int mcpr_decode_varlong (int64_t *out, void *in);

// --- WARNING --- READ COMMENTS BELOW CAREFULLY.
// data should be free'd by the receiver.
// data is just the raw packet bytes, including the packet id and everything else.
// data will be decrypted.
void mcpr_on_pkt(uint8_t pkt_id, void (*on_packet)(uint8_t *data));
void mcpr_on_any_pkt(void (*on_packet)(uint8_t *data));

int mcpr_encrypt(void *buf, void *data, size_t data_len);
int mcpr_decrypt(void *buf, void *data, size_t data_len);





// int mcpr_connect_to_server();
// int mcpr_connect_to_client();
//
// int mcpr_disconnect_from_server();
// int mcpr_disconnect_from_client();

#endif

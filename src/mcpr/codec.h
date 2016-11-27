#ifndef MCPR_CODEC_H
#define MCPR_CODEC_H

#include <stdint.h>
#include <stdbool.h>

#include <uuid/uuid.h>

#include <jansson/jansson.h>
#include <safe_math.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/sha.h>
#include <curl/curl.h>

static const size_t MCPR_BOOL_SIZE      = 1;
static const size_t MCPR_BYTE_SIZE      = 1;
static const size_t MCPR_UBYTE_SIZE     = 1;
static const size_t MCPR_SHORT_SIZE     = 2;
static const size_t MCPR_USHORT_SIZE    = 2;
static const size_t MCPR_INT_SIZE       = 4;
static const size_t MCPR_LONG_SIZE      = 8;
static const size_t MCPR_FLOAT_SIZE     = 4;
static const size_t MCPR_DOUBLE_SIZE    = 8;
static const size_t MCPR_UUID_SIZE      = 16;
static const size_t MCPR_POSITION_SIZE  = 8;

static const size_t MCPR_VARINT_SIZE_MAX  =  5;
static const size_t MCPR_VARLONG_SIZE_MAX = 10;
#define MCPR_STR_MAX 2147483652


struct mcpr_position {
    int x;
    int y;
    int z;
};


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
int mcpr_encode_chat    (void *out, const json_t *in);


int mcpr_encode_varint          (void *out, int32_t i);
int mcpr_encode_varlong         (void *out, int64_t i);
int mcpr_encode_chunk_section   (); // TODO
int mcpr_encode_position        (void *out, const struct mcpr_position *in);
int mcpr_encode_angle           (void *out, uint8_t angle); // Angles start at 0 all the way to 255.
int mcpr_encode_uuid            (void *out, uuid_t in);


/*
 * Will decode a raw sequence of bytes of length len from in.
 *
 * Decodes len bytes.
 * Returns the amount of bytes read, or < 0 upon error.
 */
int mcpr_decode_raw             (void *out, const void *in, size_t len);

/*
 * Will decode a boolean from in.
 * Decodes 1 byte.
 * Returns the amount of bytes read, or < 0 upon error.
 */
int mcpr_decode_bool            (bool *out, const void *in);

/*
 * Will decode a single byte from in.
 * Decodes 1 byte.
 * Returns the amount of bytes read, or < 0 upon error.
 */
int mcpr_decode_byte            (int8_t *out, const void *in);

/*
 * Will decode a single unsigned byte from in.
 * Decodes 1 byte.
 * Returns the amount of bytes read, or < 0 upon error.
 */
int mcpr_decode_ubyte           (uint8_t *out, const void *in);

/*
 * Will decode a short (2 bytes) from in.
 * Decodes 2 bytes.
 * Returns the amount of bytes read, or < 0 upon error.
 */
int mcpr_decode_short           (int16_t *out, const void *in);

/*
 * Will decode an unsigned short from in.
 * Decodes 2 bytes.
 * Returns the amount of bytes read, or < 0 upon error.
 */
int mcpr_decode_ushort          (uint16_t *out, const void *in);

/*
 * Will decode a 32 bit integer from in.
 * Decodes 4 bytes.
 * Returns the amount of bytes read, or < 0 upon error.
 */
int mcpr_decode_int             (int32_t *out, const void *in);

/*
 * Will decode a long from in.
 * Decodes 8 bytes.
 * Returns the amount of bytes read, or < 0 upon error.
 */
int mcpr_decode_long            (int64_t *out, const void *in);

/*
 * Will decode a float from in.
 * Decodes 4 bytes.
  * Returns the amount of bytes read, or < 0 upon error.
 */
int mcpr_decode_float           (float *out, const void *in);

/*
 * Will decode a double from in.
 * Decoes 8 bytes.
 * Returns the amount of bytes read, or < 0 upon error.
 */
int mcpr_decode_double          (double *out, const void *in);

/*
 * Will write a NUL terminated UTF-8 string of length len bytes to out. Make sure that out is big enough!
 * Len may not be <= 0.
 * Decodes len bytes.
 * Returns the amount of bytes written, or < 0 upon error.
 */
int mcpr_decode_string          (char *out, const void *in, int32_t len);

/*
 * Will decode chat from in.
 *
 */
int mcpr_decode_chat            (json_t **out, const void *in);

/*
 * Will decode a Minecraft VarInt from in. Will read no further than maxlen.
 * Note that Minecraft VarInts differ from Protocol Buffer VarInts
 *
 * Decodes 1 to 5 bytes.
 * Returns the amount of bytes read, or < 0 upon error.
 */
int mcpr_decode_varint          (int32_t *out, const void *in, size_t maxlen);

/*
 * Will decode a Minecraft VarLong from in. Will read no further than maxlen.
 * Note that Minecraft VarLongs differ from Protocol Buffer VarLongs
 * Returns the amount of bytes read, or < 0 upon error.
 */
int mcpr_decode_varlong         (int64_t *out, const void *in, size_t maxlen);
//int mcpr_decode_chunk_section   (const void *in);

/*
 * Will decode a position from in.
 *
 * Decodes 8 bytes.
 * Returns the amount of bytes read, or < 0 upon error.
 */
int mcpr_decode_position        (struct mcpr_position *out, const void *in);

/*
 * Will decode an angle from in.
 *
 * Decodes 1 byte.
 * Returns the amount of bytes read, or < 0 upon error.
 */
int mcpr_decode_angle           (uint8_t *out, const void *in);

/*
 * Deoces an UUID from in.
 *
 * Decodes 16 bytes.
 * Returns the amount of bytes read, or < 0 upon error.
 */
int mcpr_decode_uuid            (uuid_t out, const void *in);


#endif

/*
  MIT License

  Copyright (c) 2016-2020 Martijn Heil

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



  codec.h - Encoding & decoding of Minecraft Protocol data.
*/

#ifndef MCPR_CODEC_H
#define MCPR_CODEC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <ninuuid/ninuuid.h>

#include "mcpr/mcpr.h"

static const size_t MCPR_ANGLE_SIZE   = 1;
static const size_t MCPR_BOOL_SIZE    = 1;
static const size_t MCPR_BYTE_SIZE    = 1;
static const size_t MCPR_UBYTE_SIZE   = 1;
static const size_t MCPR_SHORT_SIZE   = 2;
static const size_t MCPR_USHORT_SIZE  = 2;
static const size_t MCPR_INT_SIZE     = 4;
static const size_t MCPR_LONG_SIZE    = 8;
static const size_t MCPR_FLOAT_SIZE   = 4;
static const size_t MCPR_DOUBLE_SIZE  = 8;
static const size_t MCPR_UUID_SIZE    = 16;
static const size_t MCPR_POSITION_SIZE  = 8;

static const size_t MCPR_VARINT_SIZE_MAX  =  5;
static const size_t MCPR_VARLONG_SIZE_MAX = 10;
#define MCPR_STR_MAX 2147483652


size_t mcpr_varint_bounds(int32_t value);


// Encoding/decoding functions return the amount of bytes written for encode, and amount of
// bytes read for decode. On error; they return -1
// When an error occurs, they will set ninerr

void mcpr_encode_bool     (void *out, bool b);     // writes 1 byte
void mcpr_encode_byte     (void *out, int8_t byte);  // writes 1 byte
void mcpr_encode_ubyte    (void *out, uint8_t byte);   // writes 1 byte
void mcpr_encode_short    (void *out, int16_t i);    // writes 2 bytes
void mcpr_encode_ushort   (void *out, uint16_t i);   // writes 2 bytes
void mcpr_encode_int    (void *out, int32_t i);    // writes 4 bytes
void mcpr_encode_long     (void *out, int64_t i);    // writes 8 bytes
ssize_t mcpr_encode_float   (void *out, float f);    // writes 4 bytes
ssize_t mcpr_encode_double  (void *out, double d);     // writes 8 bytes


 /*
  Make sure the out buffer is (strlen(utf8Str) + 5)
  Note that it is not guaranteed to write  (strlen(utf8Str) + 5) bytes, it can be less,
  check the returned value for how many bytes exactly were written. (It will always write the whole string)

  returns the amount of bytes written.
 */
ssize_t mcpr_encode_string  (void *out, const char *utf8Str);

/**
 *  Encode chat.
 *
 *   @param [out] out Output buffer. Should be at least the size of (strlen(in) + 5).
 *   @param [in] in Input buffer.
 *
 *   @note This function is not guaranteed to write exactly (strlen(json_dumps(root, 0)) + 5) bytes,
 *   it can write less bytes, check the return for how many bytes were exactly written.
 *
 *   @note Dont forget to free that returned string from json_dumps..
 *
 *   @returns the amount of bytes written.
 */
ssize_t mcpr_encode_chat  (void *out, const char *in);


size_t mcpr_encode_varint      (void *out, int32_t i);
size_t mcpr_encode_varlong     (void *out, int64_t i);
void mcpr_encode_position    (void *out, const struct mcpr_position *in);
void mcpr_encode_angle       (void *out, uint8_t angle); // Angles start at 0 all the way to 255.
void mcpr_encode_uuid      (void *out, const struct ninuuid *in);

/**
 * Will decode a boolean from in.
 * Decodes 1 byte.
 *
 * @param [out] out Output buffer.
 * @param [in] in Input buffer. Should be at least 1 byte long.
 * @returns The amount of bytes read, or a negative integer upon error.
 */
void mcpr_decode_bool      (bool *out, const void *in);

/**
 * Will decode a single byte from in.
 * Decodes 1 byte.
 *
 * @param [out] out Output buffer.
 * @param [in] in Input buffer. Should be at least 1 byte long.
 * @returns The amount of bytes read, or a negative integer upon error.
 */
void mcpr_decode_byte      (int8_t *out, const void *in);

/**
 * Will decode a single unsigned byte from in.
 * Decodes 1 byte.
 *
 * @param [out] out Output buffer.
 * @param [in] in Input buffer. Should be at least 1 byte long.
 * @returns The amount of bytes read, or a negative integer upon error.
 */
void mcpr_decode_ubyte       (uint8_t *out, const void *in);

/**
 * Will decode a short (2 bytes) from in.
 * Decodes 2 bytes.
 *
 * @param [out] out Output buffer.
 * @param [in] in Input buffer. Should be at least 2 bytes long.
 * @returns The amount of bytes read, or < 0 upon error.
 */
void mcpr_decode_short       (int16_t *out, const void *in);

/**
 * Will decode an unsigned short from in.
 * Decodes 2 bytes.
 *
 *@param [out] out Output buffer.
 * @param [in] in Input buffer. Should be at least 2 bytes long.
 * @returns The amount of bytes read, or < 0 upon error.
 */
void mcpr_decode_ushort      (uint16_t *out, const void *in);

/**
 * Will decode a 32 bit integer from in.
 * Decodes 4 bytes.
 *
 * @param [out] out Output buffer.
 * @param [in] in Input buffer. Should be at least 4 bytes long.
 * @returns The amount of bytes read, or < 0 upon error.
 */
void mcpr_decode_int       (int32_t *out, const void *in);

/**
 * Will decode a long from in.
 * Decodes 8 bytes.
 *
 * @param [out] Output buffer.
 * @param [in] Input buffer. Should be at least 8 bytes long.
 * @returns The amount of bytes read, or < 0 upon error.
 */
void mcpr_decode_long      (int64_t *out, const void *in);

/**
 * Will decode a float from in.
 * Decodes 4 bytes.
 *
 * @param [out] out Output buffer.
 * @param [in] Input buffer. Should be at least 4 bytes long.
 * @returns The amount of bytes read, or < 0 upon error.
 */
ssize_t mcpr_decode_float       (float *out, const void *in);

/**
 * Will decode a double from in.
 * Decodes 8 bytes.
 *
 * @param [out] out Output buffer.
 * @param [in] Input buffer. Should be at least 8 bytes long.
 * @returns The amount of bytes read, or < 0 upon error.
 */
ssize_t mcpr_decode_double      (double *out, const void *in);

/**
 * Will write a NUL terminated UTF-8 string of length len bytes to out.
 * Decodes len bytes.
 *
 * @param [out] out Output buffer. Will be dynamicly allocated using malloc()
 * @param [in] in Input buffer. Should be at least maxlen bytes long.
 * @param [in] len Length of string to decode, may not be equal to or less than 0.
 * @returns The amount of bytes written, or < 0 upon error.
 */
ssize_t mcpr_decode_string      (char **out, const void *in, size_t maxlen);

/**
 * Will decode chat from in.
 *
 * @param [out] out Pointer will be updated to new Jansson reference, note that you should decrement
 * the reference count if you are done with the object.
 * @param [in] in Input buffer, should not be NULL.
 *
 * @returns The amount of bytes read, or < 0 upon error.
 */
ssize_t mcpr_decode_chat      (char **out, const void *in, size_t maxlen);

/*
 * Will decode a Minecraft VarInt from in. Will read no further than maxlen.
 * Note that Minecraft VarInts differ from Protocol Buffer VarInts
 *
 * Decodes 1 to 5 bytes.
 * @returns The amount of bytes read, or < 0 upon error.
 */
ssize_t mcpr_decode_varint      (int32_t *out, const void *in, size_t maxlen);

/*
 * Will decode a Minecraft VarLong from in. Will read no further than maxlen.
 * Note that Minecraft VarLongs differ from Protocol Buffer VarLongs
 * @returns The amount of bytes read, or < 0 upon error.
 */
ssize_t mcpr_decode_varlong     (int64_t *out, const void *in, size_t maxlen);
//int mcpr_decode_chunk_section   (const void *in);

/*
 * Will decode a position from in.
 *
 * Decodes 8 bytes.
 * @returns The amount of bytes read, or < 0 upon error.
 */
void mcpr_decode_position    (struct mcpr_position *out, const void *in);

/*
 * Will decode an angle from in.
 *
 * Decodes 1 byte.
 * @returns The amount of bytes read, or < 0 upon error.
 */
void mcpr_decode_angle       (uint8_t *out, const void *in);

/*
 * Deoces an UUID from in.
 *
 * Decodes 16 bytes.
 * @returns The amount of bytes read, or < 0 upon error.
 */
void mcpr_decode_uuid      (struct ninuuid *out, const void *in);


#endif

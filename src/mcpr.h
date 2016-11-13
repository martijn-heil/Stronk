#ifndef MCPR_H
#define MCPR_H

#include <stdbool.h>
#include <stdint.h>

#include <arpa/inet.h>
#include <uuid/uuid.h>

#include <jansson/jansson.h>
#include <nbt/nbt.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

/*
    Minecraft Protocol (http://wiki.vg/Protocol)
    MCPR = MineCraft PRotocol.
*/

#define MCPR_PROTOCOL_VERSION 210

#define MCPR_STR_MAX 2147483652

struct mcpr_position {
    int x;
    int y;
    int z;
};

// -1 is used for remaining error caes.
#define MCPR_ERR_ARITH_OVERFLOW 2
#define MCPR_ERR_MALLOC_FAILURE 3
#define MCPR_ERR_ARITH          4

char *mcpr_err_to_str(int status); // result return value NOT be free'd or modified, the return value is a NUL terminated string!


void mcpr_set_malloc_func(void *(*new_malloc_func)(size_t size));
void mcpr_set_free_func(void (*new_free_func)(void *ptr));


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
int mcpr_encode_chat    (void *out, const json_t *in);


int mcpr_encode_varint          (void *out, int32_t i);
int mcpr_encode_varlong         (void *out, int64_t i);
int mcpr_encode_chunk_section   ();
int mcpr_encode_position        (void *out, const struct mcpr_position *in);
int mcpr_encode_angle           (void *out, uint8_t angle); // Angles start at 0 all the way to 255.
int mcpr_encode_uuid            (void *out, uuid_t in);
//int mcpr_encode_entity_metadata (void *out, const struct mcpr_entity_metadata *in);




int mcpr_decode_bool            (bool *out, const void *in);
int mcpr_decode_byte            (int8_t *out, const void *in);
int mcpr_decode_ubyte           (uint8_t *out, const void *in);
int mcpr_decode_short           (int16_t *out, const void *in);
int mcpr_decode_ushort          (uint16_t *out, const void *in);
int mcpr_decode_int             (int32_t *out, const void *in);
int mcpr_decode_long            (int64_t *out, const void *in);
int mcpr_decode_float           (float *out, const void *in);
int mcpr_decode_double          (double *out, const void *in);
int mcpr_decode_string          (char *out, const void *in, int32_t len); // Will write a NUL terminated UTF-8 string to the buffer. Beware buffer overflows! Make sure that out is big enough! Len may be 0
int mcpr_decode_chat            (json_t **out, const void *in); // Will write a NUL terminated UTF-8 string to the buffer. Beware buffer overflows! Make sure that out is big enough!
int mcpr_decode_varint          (int32_t *out, const void *in, size_t maxlen);
int mcpr_decode_varlong         (int64_t *out, const void *in, size_t maxlen);
int mcpr_deocde_chunk_section   (const void *in);
int mcpr_decode_position        (struct mcpr_position *out, const void *in);
int mcpr_decode_angle           (uint8_t *out, const void *in); // Angles start at 0 all the way to 255.
int mcpr_decode_uuid            (uuid_t out, const void *in);
//int mcpr_decode_entity_metadata (struct mcpr_entity_metadata *out, const void *in);


int mcpr_compress(void **out, void *in); // will malloc *out for you. Don't forget to free it.
int mcpr_decompress(void **out, void *in); // will malloc *out for you. Don't forget to free it.


// LOW LEVEL API ABOVE, HIGH LEVEL API BELOW. --------------------------------------------------------------------------------------------------------------------------
// In the high level API, performance is not as important, ease of use is a greater concern.
// The mcpr_packet structure, for example, is a little wasteful with memory.
//
enum mcpr_state {
    MCPR_STATE_HANDSHAKE = 0,
    MCPR_STATE_STATUS = 1,
    MCPR_STATE_LOGIN = 2,
    MCPR_STATE_PLAY = 3
};


struct mcpr_server_sess {
    int sockfd;
    enum mcpr_state state;

    void *shared_secret;
};

struct mcpr_client_sess {
    int sockfd;
    enum mcpr_state state;

    EVP_CIPHER_CTX ctx_encrypt;
    EVP_CIPHER_CTX ctx_decrypt;
};

int mcpr_encrypt(EVP_CIPHER_CTX ctx_encrypt, void *data, size_t len);
int mcpr_encrypt(EVP_CIPHER_CTX ctx_decrypt, void *data, size_t len);


//struct mcpr_server_sess mcpr_init_server_sess(const char *host, int port);
int mcpr_init_client_sess(struct mcpr_client_sess *sess, const char *host, int port, int socktimeout, const char *username);



#endif

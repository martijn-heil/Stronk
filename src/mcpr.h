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

// TODO this isnt used much currently
// -1 is used for remaining error caes.
#define MCPR_ERR_ARITH_OVERFLOW 2
#define MCPR_ERR_MALLOC_FAILURE 3
#define MCPR_ERR_ARITH          4

char *mcpr_err_to_str(int status); // result return value NOT be free'd or modified, the return value is a NUL terminated string!


void mcpr_set_malloc_func(void *(*new_malloc_func)(size_t size));
void mcpr_set_free_func(void (*new_free_func)(void *ptr));
void mcpr_set_reallloc_func(void *(*new_realloc_func)(void *ptr, size_t size));


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
int mcpr_encode_chunk_section   ();
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


/*
 * Will malloc *out for you. Don't forget to free it.
 */
int mcpr_compress(void **out, void *in);

/*
 * Will malloc *out for you. Don't forget to free it.
 */
int mcpr_decompress(void **out, void *in); //


enum mcpr_state {
    MCPR_STATE_HANDSHAKE = 0,
    MCPR_STATE_STATUS = 1,
    MCPR_STATE_LOGIN = 2,
    MCPR_STATE_PLAY = 3
};

struct mcpr_connection {
    int sockfd;
    enum mcpr_state state;
    bool use_compression;
    unsigned int compression_treshold; // Not guaranteed to be initialized if compression is set to false.

    bool use_encryption;
    EVP_CIPHER_CTX ctx_encrypt;  // Not guaranteed to be initialized if use_encryption is set to false
    EVP_CIPHER_CTX ctx_decrypt;  // Not guaranteed to be initialized if use_encryption is set to false
    unsigned int encryption_block_size;  // Not guaranteed to be initialized if use_encryption is set to false
};

struct mcpr_packet {
    uint8_t id;
    void *data;
    size_t data_len;
};

struct mcpr_client_player {
    struct mcpr_connection conn;
};


struct mcpr_server {
    struct mcpr_client_player **client_players;
};

struct mcpr_client {
    struct mcpr_connection conn;


    char *client_token; // Should be free'd using the free function specified with mcpr_set_free_func()
    size_t client_token_len; // It's NUL terminated anyway, but you can use this to prevent wasting performance with strlen.
    char *access_token; // Should be free'd using the free function specified with mcpr_set_free_func()
    size_t access_token_len; // It's NUL terminated anyway, but you can use this to prevent wasting performance with strlen.
    const char *username;
    const char *account_name; // either email or username.
};

/*
 * out should be at least of size (len + mcpr_client->encryption_block_size - 1)
 */
int mcpr_encrypt(void *out, const void *data, EVP_CIPHER_CTX ctx_encrypt, size_t len);

/*
 * out should be at least of size (len + mcpr_client->encryption_block_size)
 */
int mcpr_decrypt(void *out, const void *data, EVP_CIPHER_CTX ctx_decrypt, size_t len);


//struct mcpr_server_sess mcpr_init_server_sess(const char *host, int port);

// use_encryption is only temporarily.. will be removed before this library goes into real usage.
int mcpr_init_client(struct mcpr_client *sess, const char *host, int port, int timeout, const char *account_name, bool use_encryption);


int mcpr_write_packet(struct mcpr_connection *conn, struct mcpr_packet *pkt);
struct mcpr_packet *mcpr_read_packet(struct mcpr_connection *conn); // returns NULL on error. returned packet should be free'd using the free function specified with mcpr_set_free_func()


#endif

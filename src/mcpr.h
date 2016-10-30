#ifndef MCPR_H
#define MCPR_H

#include <stdbool.h>
#include <stdint.h>

#include <arpa/inet.h>
#include <uuid/uuid.h>

#include <jansson/jansson.h>

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



// struct mcpr_entity_metadata {
//     uint8_t index;
//     int8_t type;
//
//     union {
//         int8_t byte;
//         int32_t air;
//         char *custom_name;
//         bool is_custom_name_visible;
//         bool is_silent;
//         bool no_gravity;
//
//         struct mcpr_entity_metadata_potion {
//             union {
//                 // TODO: slot
//             };
//         } potion;
//
//         struct mcpr_entity_metadata_falling_block {
//             union {
//                 struct mcpr_position spawn_position;
//             };
//         } falling_block;
//
//         struct mcpr_entity_metadata_area_effect_cloud {
//             union {
//                 float radius;
//                 int32_t color;
//                 bool ignore_radius;
//                 int32_t particle_id;
//                 int32_t particle_parameter_1;
//                 int32_t particle_parameter_2;
//             };
//         } area_effect_cloud;
//     };
// };
//
// struct mcpr_entity_metadata {
//     uint8_t index;
//     int8_t type;
//     void *value; // this should be free'd.
// };

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
int mcpr_decode_string          (char *out, const void *in, int32_t len); // Will write a NUL terminated UTF-8 string to the buffer. Beware buffer overflows! Make sure that out is big enough!
int mcpr_decode_chat            (json_t **out, const void *in); // Will write a NUL terminated UTF-8 string to the buffer. Beware buffer overflows! Make sure that out is big enough!
int mcpr_decode_varint          (int32_t *out, const void *in);
int mcpr_decode_varlong         (int64_t *out, const void *in);
int mcpr_deocde_chunk_section   (const void *in);
int mcpr_decode_position        (struct mcpr_position *out, const void *in);
int mcpr_decode_angle           (uint8_t *out, const void *in); // Angles start at 0 all the way to 255.
int mcpr_decode_uuid            (uuid_t out, const void *in);
//int mcpr_decode_entity_metadata (struct mcpr_entity_metadata *out, const void *in);

// --- WARNING --- READ COMMENTS BELOW CAREFULLY.
// data should be free'd by the receiver.
// data is just the raw packet bytes, including the packet id and everything else.
// data will be decrypted.
// void mcpr_on_pkt(uint8_t pkt_id, void (*on_packet)(uint8_t *data));
// void mcpr_on_any_pkt(void (*on_packet)(uint8_t *data));

int mcpr_encrypt(void *buf, void *data, size_t data_len);
int mcpr_decrypt(void *buf, void *data, size_t data_len);


// LOW LEVEL API ABOVE, HIGH LEVEL API BELOW. --------------------------------------------------------------------------------------------------------------------------

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

    void *shared_secret;
};

//struct mcpr_server_sess mcpr_init_server_sess(const char *host, int port);
int mcpr_init_client_sess(struct mcpr_client_sess *sess, const char *host, int port);










#endif

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
int mcpr_init_client_sess(struct mcpr_client_sess *sess, const char *host, int port);

// enum mcpr_recipient_type {
//     MCPR_RECIPIENT_TYPE_SERVER,
//     MCPR_RECIPIENT_TYPE_CLIENT,
// };
//
// enum mcpr_object_type {
//     MCPR_OBJECT_TYPE_BOAT                   = 1,
//     MCPR_OBJECT_TYPE_ITEM_STACK             = 2,
//     MCPR_OBJECT_TYPE_AREA_EFFECT_CLOUD      = 3,
//     MCPR_OBJECT_TYPE_MINECART               = 10,
//     MCPR_OBJECT_TYPE_ACTIVATED_TNT          = 50,
//     MCPR_OBJECT_TYPE_ENDERCRYSTAL           = 51,
//     MCPR_OBJECT_TYPE_TIPPED_ARROW           = 60,
//     MCPR_OBJECT_TYPE_SNOWBALL               = 61,
//     MCPR_OBJECT_TYPE_EGG                    = 62,
//     MCPR_OBJECT_TYPE_FIREBALL               = 63,
//     MCPR_OBJECT_TYPE_FIRECHARGE             = 64,
//     MCPR_OBJECT_TYPE_THROWN_ENDERPEARL      = 65,
//     MCPR_OBJECT_TYPE_WITHER_SKULL           = 66,
//     MCPR_OBJECT_TYPE_SHULKER_BULLET         = 67,
//     MCPR_OBJECT_TYPE_FALLING_OBJECTS        = 70,
//     MCPR_OBJECT_TYPE_ITEM_FRAMES            = 71,
//     MCPR_OBJECT_TYPE_EYE_OF_ENDER           = 72,
//     MCPR_OBJECT_TYPE_THROWN_POTION          = 73,
//     MCPR_OBJECT_TYPE_THROWN_EXP_BOTTLE      = 75,
//     MCPR_OBJECT_TYPE_FIREWORK_ROCKET        = 76,
//     MCPR_OBJECT_TYPE_LEASH_KNOT             = 77,
//     MCPR_OBJECT_TYPE_ARMOR_STAND            = 78,
//     MCPR_OBJECT_TYPE_FISHING_FLOAT          = 90,
//     MCPR_OBJECT_TYPE_SPECTRAL_ARROW         = 91,
//     MCPR_OBJECT_TYPE_DRAGON_FIREBALL        = 93,
// };
//
// enum mcpr_mob_type {
//     MCPR_MOB_TYPE_CREEPER           = 50,
//     MCPR_MOB_TYPE_SKELETON          = 51,
//     MCPR_MOB_TYPE_SPIDER            = 52,
//     MCPR_MOB_TYPE_GIANT             = 53,
//     MCPR_MOB_TYPE_ZOMBIE            = 54,
//     MCPR_MOB_TYPE_SLIME             = 55,
//     MCPR_MOB_TYPE_GHAST             = 56,
//     MCPR_MOB_TYPE_PIG_ZOMBIE        = 57,
//     MCPR_MOB_TYPE_ENDERMAN          = 58,
//     MCPR_MOB_TYPE_CAVE_SPIDER       = 59,
//     MCPR_MOB_TYPE_SILVERFISH        = 60,
//     MCPR_MOB_TYPE_BLAZE             = 61,
//     MCPR_MOB_TYPE_LAVA_SLIME        = 62,
//     MCPR_MOB_TYPE_ENDER_DRAGON      = 63,
//     MCPR_MOB_TYPE_WITHER_BOSS       = 64,
//     MCPR_MOB_TYPE_BAT               = 65,
//     MCPR_MOB_TYPE_WITCH             = 66,
//     MCPR_MOB_TYPE_ENDERMITE         = 67,
//     MCPR_MOB_TYPE_GUARDIAN          = 68,
//     MCPR_MOB_TYPE_SHULKER           = 69,
//     MCPR_MOB_TYPE_PIG               = 90,
//     MCPR_MOB_TYPE_SHEEP             = 91,
//     MCPR_MOB_TYPE_COW               = 92,
//     MCPR_MOB_TYPE_CHICKEN           = 93,
//     MCPR_MOB_TYPE_SQUID             = 94,
//     MCPR_MOB_TYPE_WOLF              = 95,
//     MCPR_MOB_TYPE_MUSHROOM_COW      = 96,
//     MCPR_MOB_TYPE_SNOW_MAN          = 97,
//  // MCPR_MOB_TYPE_PANTHER           = 7734, add plis Mojang :c
//     MCPR_MOB_TYPE_OCELOT            = 98,
//     MCPR_MOB_TYPE_IRON_GOLEM        = 99,
//     MCPR_MOB_TYPE_HORSE             = 100,
//     MCPR_MOB_TYPE_RABBIT            = 101,
//     MCPR_MOB_TYPE_POLAR_BEAR        = 102,
//     MCPR_MOB_TYPE_VILLAGER          = 120,
// };
//
// enum mcpr_cardinal_direction {
//     // don't ask me why the the numerical ID's are so strange, blame Mojang.
//     MCPR_CARDINAL_DIRECTION_NORTH = 2,
//     MCPR_CARDINAL_DIRECTION_EAST  = 3,
//     MCPR_CARDINAL_DIRECTION_SOUTH = 0,
//     MCPR_CARDINAL_DIRECTION_WEST  = 1,
// };
//
// enum mcpr_animation_type {
//     MCPR_ANIMATION_TYPE_SWING_MAIN_ARM           = 0,
//     MCPR_ANIMATION_TYPE_TAKE_DAMAGE              = 1,
//     MCPR_ANIMATION_TYPE_LEAVE_BED                = 2,
//     MCPR_ANIMATION_TYPE_SWING_OFFHAND            = 3,
//     MCPR_ANIMATION_TYPE_CRITICAL_EFFECT          = 4,
//     MCPR_ANIMATION_TYPE_MAGIC_CRITICAL_EFFECT    = 5,
// };
//
// enum mcpr_boss_bar_color {
//     MCPR_BOSS_BAR_COLOR_PINK    = 0,
//     MCPR_BOSS_BAR_COLOR_BLUE    = 1,
//     MCPR_BOSS_BAR_COLOR_RED     = 2,
//     MCPR_BOSS_BAR_COLOR_GREEN   = 3,
//     MCPR_BOSS_BAR_COLOR_YELLOW  = 4,
//     MCPR_BOSS_BAR_COLOR_PURPLE  = 5,
//     MCPR_BOSS_BAR_COLOR_WHITE   = 6,
// };
//
// enum mcpr_boss_bar_division {
//     MCPR_BOSS_BAR_DIVISION_NONE         = 0,
//     MCPR_BOSS_BAR_DIVISION_6_NOTCHES    = 1,
//     MCPR_BOSS_BAR_DIVISION_10_NOTCHES   = 2,
//     MCPR_BOSS_BAR_DIVISION_12_NOTCHES   = 3,
//     MCPR_BOSS_BAR_DIVISION_20_NOTCHES   = 4,
// };
//
// enum mcpr_boss_bar_action {
//     MCPR_BOSS_BAR_ACTION_ADD            = 0,
//     MCPR_BOSS_BAR_ACTION_REMOVE         = 1,
//     MCPR_BOSS_BAR_ACTION_UPDATE_HEALTH  = 2,
//     MCPR_BOSS_BAR_ACTION_UPDATE_TITLE   = 3,
//     MCPR_BOSS_BAR_ACTION_UPDATE_STYLE   = 4,
//     MCPR_BOSS_BAR_ACTION_UPDATE_FLAGS   = 5,
// };
//
// enum mcpr_server_difficulty {
//     MCPR_SERVER_DIFFICULTY_PEACEFUL = 0,
//     MCPR_SERVER_DIFFICULTY_EASY     = 1,
//     MCPR_SERVER_DIFFICULTY_NORMAL   = 2,
//     MCPR_SERVER_DIFFICULTY_HARD     = 3,
// };
//
// enum mcpr_chat_position {
//     MCPR_CHAT_POSITION_CHAT     = 0,
//     MCPR_CHAT_POSITION_SYSTEM   = 1,
//     MCPR_CHAT_POSITION_HOTBAR   = 2,
// };

// TODO finish entity metadata
// struct mcpr_entity_metadata {
//     uint8_t index;
//     int8_t type;
//     enum mcpr_recipient_type bound_to; // clientbound or serverbound.
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
// struct mcpr_statistic {
//     char *name; // Shall not be a string literal, use strcpy.
//     int32_t value;
// };
//
// struct mcpr_multi_block_change_record {
//     int x, z;
//     uint8_t y;
//     int32_t block_id;
// };
//
// // This is unfortanutely a little wasteful for memory.. oh well, it's only memory.
// // Note: All char*'s should be free'd when destucting this struct.
// // SO DO NOT PUT STRING LITERALS IN IT, USE STRCPY.
// // Struct pointers should be free'd too.
// struct mcpr_packet {
//     uint8_t id;
//     enum mcpr_state state;
//
//     union {
//         struct { // Serverbound - Handshake - Handshake
//             int32_t protocol_version;
//             char *server_adress; // No string literals, use strcpy.
//             uint16_t server_port;
//             enum mcpr_state next_state;
//         } SB_HS_00;
//
//         struct { // Clientbound - Play - Spawn Object
//             int32_t entity_id;
//             uuid_t object_uuid;
//             enum mcpr_object_type type;
//             double x, y, z;
//             uint8_t pitch;
//             uint8_t yaw;
//             int32_t data;
//             int16_t velocity_x, velocity_y, velocity_z;
//         } CB_PL_00;
//
//         struct { // Clientbound - Play - Spawn Experience Orb
//             int32_t entity_id;
//             double x, y, z;
//             int16_t count;
//         } CB_PL_01;
//
//         struct { // Clientbound - Play - Spawn Global Entity (thunderbolt)
//             int32_t entity_id;
//             int8_t type; // The global entity type, currently always 1 for thunderbolt
//             double x, y, z;
//         } CB_PL_02;
//
//         struct { // Clientbound - Play - Spawn Mob
//             int32_t entity_id;
//             uuid_t entity_uuid;
//             enum mcpr_mob_type type;
//             double x, y, z;
//             uint8_t yaw;
//             uint8_t pitch, head_pitch;
//             int16_t velocity_x, velocity_y, velocity_z;
//             struct mcpr_entity_metadata *metadata; // array
//         } CB_PL_03;
//
//         struct { // Clientbound - Play - Spawn Painting
//             int32_t entity_id;
//             uuid_t entity_uuid;
//             char *title; // No string literals, use strcpy.
//             struct mcpr_position location;
//             enum mcpr_cardinal_direction direction;
//         } CB_PL_04;
//
//         struct { // Clientbound - Play - Spawn Player
//             int32_t entity_id;
//             uuid_t player_uuid;
//             double x, y, z;
//             uint8_t yaw, pitch;
//             struct mcpr_entity_metadata *metadata; // array
//         } CB_PL_05;
//
//         struct { // Clientbound - Play - Animation
//             int32_t entity_id;
//             enum mcpr_animation_type animation;
//         } CB_PL_06;
//
//         struct { // Clientbound - Play - Statistics
//             int32_t count;
//             struct mcpr_statistic *statistics; // array
//         } CB_PL_07;
//
//         struct { // Clientbound - Play - Block Break Animation
//             int32_t entity_id;
//             struct mcpr_position location; // block position.
//             int8_t destroy_stage; // 0–9 to set it, any other value to unset it
//         } CB_PL_08;
//
//         struct { // Clientbound - Play - Update Block Entity
//             struct mcpr_position location;
//             uint8_t action; // TODO: maybe make a enum for this.
//             nbt_node *nbt_data;
//         } CB_PL_09;
//
//         struct { // Clientbound - Play - Block Action
//             struct mcpr_position location;
//             uint8_t action_id;
//             uint8_t action_param;
//             int32_t block_type; // TODO enum for block type id?
//         } CB_PL_0A;
//
//         struct { // Clientbound - Play - Block Change
//             struct mcpr_position location;
//             int32_t block_id;
//         } CB_PL_0B;
//
//         struct { // Clientbound - Play - Boss Bar
//             uuid_t uuid;
//             enum mcpr_boss_bar_action action;
//
//             union {
//                 struct { // Action 0: add
//                     json_t *title;
//                     float health;
//                     enum mcpr_boss_bar_color color;
//                     enum mcpr_boss_bar_division division;
//                     uint8_t flags;
//                 } action_add;
//
//                 // Action 1: remove (has no fields, so no struct either)
//
//                 struct { // Action 2: update health
//                     float health;
//                 } action_update_health;
//
//                 struct { // Action 3: update title
//                     json_t *title;
//                 } action_update_title;
//
//                 struct { // Action 4: update style
//                     enum mcpr_boss_bar_color color;
//                     enum mcpr_boss_bar_division division;
//                 } action_update_style;
//
//                 struct { // Action 5: upate flags
//                     uint8_t flags;
//                 } action_update_flags;
//             };
//         } CB_PL_0C;
//
//         struct { // Clientbound - Play - Server difficulty 0x0D
//             enum mcpr_server_difficulty difficulty;
//         } CB_PL_0D;
//
//         struct { // Clientbound - Play - Tab-Complete 0x0E
//             int32_t count;
//             char **matches; // array of strings.
//         } CB_PL_0E;
//
//         struct { // Clientbound - Play - Chat Message 0x0F
//             json_t *json_data;
//             enum mcpr_chat_position position;
//         } CB_PL_0F;
//
//         struct { // Clientbound - Play - Multi Block Change
//             int32_t chunk_x, chunk_z;
//             int32_t record_count;
//
//             struct mcpr_multi_block_change_record *records;
//         } CB_PL_10;
//     };
// };



#endif

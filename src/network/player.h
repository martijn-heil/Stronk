#ifndef PLAYER_H
#define PLAYER_H

#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include <ninuuid/ninuuid.h>
#include <mcpr/mcpr.h>
#include <mcpr/abstract_packet.h>

#include <world/fposition.h>

struct player {
    int32_t entity_id;
    struct ninuuid uuid;
    char *client_brand; // or NULL if unknown. Might be set to a non-NULL value when a MC|BRAND plugin message is received.
    struct fposition pos;
    struct connection conn;
    struct mcpr_position compass_target;

    bool invulnerable;
    bool is_flying;
    float flying_speed;
    bool allow_flying;
    float walking_speed;
    enum mcpr_gamemode gamemode;

    bool client_settings_known;
    struct
    {
        char *locale;
        unsigned char view_distance;
        enum mcpr_chat_mode chat_mode;
        bool chat_colors;
        enum mcpr_side_based_hand main_hand;

        struct
        {
            bool cape_enabled;
            bool jacket_enabled;
            bool left_sleeve_enabled, right_sleeve_enabled;
            bool left_pants_enabled, right_pants_enabled;
            bool hat_enabled;
        } displayed_skin_parts;

    } client_settings;

    long last_teleport_id;
    struct timespec last_keepalive_received; // Based on server_get_internal_clock_time()
    struct timespec last_keepalive_sent; // Based on server_get_internal_clock_time()
};


struct fposition *player_get_position(struct player *p);
struct connection *player_get_connection(struct player *p);
struct ninuuid uuid *player_get_uuid(struct player *p);

#endif

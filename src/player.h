#ifndef PLAYER_H
#define PLAYER_H

#include <stdio.h>
#include <stdbool.h>

#include <ninuuid/ninuuid.h>
#include <mcpr/mcpr.h>

#include "fposition.h"

struct player_client_settings
{
    char *locale;
    int view_distance;
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

};

struct player {
    struct ninuuid uuid;
    struct fposition pos;
    FILE *chat_stream; // Write only, no complex chat
    struct connection conn;

    bool invulnerable;
    bool flying;
    bool allow_flying;
    enum mcpr_gamemode gamemode;

    struct
    {
        char *locale;
        int view_distance;
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
};


struct player_client_settings *player_get_client_settings(player *p);
struct fposition *player_get_position(player *p);

#endif

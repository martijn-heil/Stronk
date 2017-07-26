/*
    MIT License

    Copyright (c) 2016 Martijn Heil

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
*/

#ifndef STRONK_NETWORK_PLAYER_H
#define STRONK_NETWORK_PLAYER_H

#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include <algo/hash-table.h>

#include <ninuuid/ninuuid.h>
#include <mcpr/mcpr.h>
#include <mcpr/connection.h>

#include <world/positions.h>
#include "connection.h"

struct player {
    int32_t entity_id;
    char *username;
    struct ninuuid uuid;
    char *client_brand; // or NULL if unknown. Might be set to a non-NULL value when a MC|BRAND plugin message is received.
    struct entitypos pos;
    struct connection *conn;
    struct mcpr_position compass_target;
    //HashTable *loaded_chunks; TODO maybe

    bool invulnerable;
    bool is_flying;
    float flying_speed;
    bool allow_flying;
    float walking_speed;
    enum mcpr_gamemode gamemode;

    bool client_settings_known; // client settings are not guaranteed to be initialized if this is false
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


static const struct entitypos *player_get_position(const struct player *p)
{
    return &(p->pos);
}

static const struct connection *player_get_connection(const struct player *p)
{
    return p->conn;
}

static const struct ninuuid *player_get_uuid(const struct player *p)
{
    return &(p->uuid);
}

#endif

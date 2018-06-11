/*
    MIT License

    Copyright (c) 2017 Martijn Heil

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

#ifndef STRONK_PACKETHANDLERS_H
#define STRONK_PACKETHANDLERS_H

#include <stdbool.h>
#include <mcpr/packet.h>
#include <network/connection.h>

enum hp_result_type
{
    HP_RESULT_OK,
    HP_RESULT_ERR,
    HP_RESULT_FATAL,
    HP_RESULT_CLOSED,
};

struct hp_result
{
    enum hp_result_type result;
    const char *disconnect_message; // may be NULL.
    bool free_disconnect_message;
};

// All packets are obviously serverbound
struct hp_result handle_hs_handshake                    (const struct mcpr_packet *pkt, struct connection *conn);

struct hp_result handle_lg_login_start                  (const struct mcpr_packet *pkt, struct connection *conn);
struct hp_result handle_lg_encryption_response          (const struct mcpr_packet *pkt, struct connection *conn);
struct hp_result handle_st_request                      (const struct mcpr_packet *pkt, struct connection *conn);
struct hp_result handle_st_ping                         (const struct mcpr_packet *pkt, struct connection *conn);

struct hp_result handle_pl_teleport_confirm             (const struct mcpr_packet *pkt, struct connection *conn);
struct hp_result handle_pl_tab_complete                 (const struct mcpr_packet *pkt, struct connection *conn);
struct hp_result handle_pl_chat_message                 (const struct mcpr_packet *pkt, struct connection *conn);
struct hp_result handle_pl_client_status                (const struct mcpr_packet *pkt, struct connection *conn);
struct hp_result handle_pl_client_settings              (const struct mcpr_packet *pkt, struct connection *conn);
struct hp_result handle_pl_confirm_transaction          (const struct mcpr_packet *pkt, struct connection *conn);
struct hp_result handle_pl_enchant_item                 (const struct mcpr_packet *pkt, struct connection *conn);
struct hp_result handle_pl_click_window                 (const struct mcpr_packet *pkt, struct connection *conn);
struct hp_result handle_pl_close_window                 (const struct mcpr_packet *pkt, struct connection *conn);
struct hp_result handle_pl_plugin_message               (const struct mcpr_packet *pkt, struct connection *conn);
struct hp_result handle_pl_use_entity                   (const struct mcpr_packet *pkt, struct connection *conn);
struct hp_result handle_pl_keep_alive                   (const struct mcpr_packet *pkt, struct connection *conn);
struct hp_result handle_pl_player_position              (const struct mcpr_packet *pkt, struct connection *conn);
struct hp_result handle_pl_player_position_and_look     (const struct mcpr_packet *pkt, struct connection *conn);
struct hp_result handle_pl_player_look                  (const struct mcpr_packet *pkt, struct connection *conn);
struct hp_result handle_pl_player                       (const struct mcpr_packet *pkt, struct connection *conn);
struct hp_result handle_pl_vehicle_move                 (const struct mcpr_packet *pkt, struct connection *conn);
struct hp_result handle_pl_steer_boat                   (const struct mcpr_packet *pkt, struct connection *conn);
struct hp_result handle_pl_player_abilities             (const struct mcpr_packet *pkt, struct connection *conn);
struct hp_result handle_pl_player_digging               (const struct mcpr_packet *pkt, struct connection *conn);
struct hp_result handle_pl_entity_action                (const struct mcpr_packet *pkt, struct connection *conn);
struct hp_result handle_pl_steer_vehicle                (const struct mcpr_packet *pkt, struct connection *conn);
struct hp_result handle_pl_resource_pack_status         (const struct mcpr_packet *pkt, struct connection *conn);
struct hp_result handle_pl_held_item_change             (const struct mcpr_packet *pkt, struct connection *conn);
struct hp_result handle_pl_creative_inventory_action    (const struct mcpr_packet *pkt, struct connection *conn);
struct hp_result handle_pl_update_sign                  (const struct mcpr_packet *pkt, struct connection *conn);
struct hp_result handle_pl_animation                    (const struct mcpr_packet *pkt, struct connection *conn);
struct hp_result handle_pl_spectate                     (const struct mcpr_packet *pkt, struct connection *conn);
struct hp_result handle_pl_player_block_placement       (const struct mcpr_packet *pkt, struct connection *conn);
struct hp_result handle_pl_use_item                     (const struct mcpr_packet *pkt, struct connection *conn);

#endif

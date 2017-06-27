#include <stdbool.h>
#include <mcpr/abstract_packet.h>
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
struct hp_result handle_hs_handshake                    (const struct mcpr_abstract_packet *pkt, struct connection *conn);

struct hp_result handle_lg_login_start                  (const struct mcpr_abstract_packet *pkt, struct connection *conn);
struct hp_result handle_lg_encryption_response          (const struct mcpr_abstract_packet *pkt, struct connection *conn);
struct hp_result handle_st_request                      (const struct mcpr_abstract_packet *pkt, struct connection *conn);
struct hp_result handle_st_ping                         (const struct mcpr_abstract_packet *pkt, struct connection *conn);

struct hp_result handle_pl_teleport_confirm             (const struct mcpr_abstract_packet *pkt, struct connection *conn);
struct hp_result handle_pl_tab_complete                 (const struct mcpr_abstract_packet *pkt, struct connection *conn);
struct hp_result handle_pl_chat_message                 (const struct mcpr_abstract_packet *pkt, struct connection *conn);
struct hp_result handle_pl_client_status                (const struct mcpr_abstract_packet *pkt, struct connection *conn);
struct hp_result handle_pl_client_settings              (const struct mcpr_abstract_packet *pkt, struct connection *conn);
struct hp_result handle_pl_confirm_transaction          (const struct mcpr_abstract_packet *pkt, struct connection *conn);
struct hp_result handle_pl_enchant_item                 (const struct mcpr_abstract_packet *pkt, struct connection *conn);
struct hp_result handle_pl_click_window                 (const struct mcpr_abstract_packet *pkt, struct connection *conn);
struct hp_result handle_pl_close_window                 (const struct mcpr_abstract_packet *pkt, struct connection *conn);
struct hp_result handle_pl_plugin_message               (const struct mcpr_abstract_packet *pkt, struct connection *conn);
struct hp_result handle_pl_use_entity                   (const struct mcpr_abstract_packet *pkt, struct connection *conn);
struct hp_result handle_pl_keep_alive                   (const struct mcpr_abstract_packet *pkt, struct connection *conn);
struct hp_result handle_pl_player_position              (const struct mcpr_abstract_packet *pkt, struct connection *conn);
struct hp_result handle_pl_player_position_and_look     (const struct mcpr_abstract_packet *pkt, struct connection *conn);
struct hp_result handle_pl_player_look                  (const struct mcpr_abstract_packet *pkt, struct connection *conn);
struct hp_result handle_pl_player                       (const struct mcpr_abstract_packet *pkt, struct connection *conn);
struct hp_result handle_pl_vehicle_move                 (const struct mcpr_abstract_packet *pkt, struct connection *conn);
struct hp_result handle_pl_steer_boat                   (const struct mcpr_abstract_packet *pkt, struct connection *conn);
struct hp_result handle_pl_player_abilities             (const struct mcpr_abstract_packet *pkt, struct connection *conn);
struct hp_result handle_pl_player_digging               (const struct mcpr_abstract_packet *pkt, struct connection *conn);
struct hp_result handle_pl_entity_action                (const struct mcpr_abstract_packet *pkt, struct connection *conn);
struct hp_result handle_pl_steer_vehicle                (const struct mcpr_abstract_packet *pkt, struct connection *conn);
struct hp_result handle_pl_resource_pack_status         (const struct mcpr_abstract_packet *pkt, struct connection *conn);
struct hp_result handle_pl_held_item_change             (const struct mcpr_abstract_packet *pkt, struct connection *conn);
struct hp_result handle_pl_creative_inventory_action    (const struct mcpr_abstract_packet *pkt, struct connection *conn);
struct hp_result handle_pl_update_sign                  (const struct mcpr_abstract_packet *pkt, struct connection *conn);
struct hp_result handle_pl_animation                    (const struct mcpr_abstract_packet *pkt, struct connection *conn);
struct hp_result handle_pl_spectate                     (const struct mcpr_abstract_packet *pkt, struct connection *conn);
struct hp_result handle_pl_player_block_placement       (const struct mcpr_abstract_packet *pkt, struct connection *conn);
struct hp_result handle_pl_use_item                     (const struct mcpr_abstract_packet *pkt, struct connection *conn);

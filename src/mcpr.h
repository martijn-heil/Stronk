#ifndef MCPR_H
#define MCPR_H

#include <stdbool.h>
#include <stdint.h>

/*
    Minecraft Protocol (http://wiki.vg/Protocol)
    MCPR = MineCraft PRotocol.
*/

#define PROTOCOL_VERSION 210

struct mcpr_packet {
    uint8_t id;
};

struct mcpr_entity_metadata_entry {
    uint8_t index;
    int8_t type;
    void *value;
};

struct mcpr_slot {
    int16_t block_id;
    int8_t item_count; // optional, not present if Block ID is -1.
    int16_t item_damage; // optional, not present if Block ID is -1.
    char *nbt; // optional, not present if Block ID is -1. TODO: NBT...
};

void mcpr_send_packet(struct mcpr_packet *pkt);
void mcpr_on_packet(void (*on_packet)(mcpr_packet *pkt));
void mcpr_on_specific_packet(uint8_t packet_id, void (*on_packet)(mcpr_packet *pkt));

/*
 Explanation of namings:

 MCPR = MineCraft PRotocol.

 sb = server-bound
 cb = client-bound

 st = status state
 hs = handshake state
 pl = play state
 lo = login state

 Format: (state)_(bound to?)_name

 Examples:
    hs_sb_handshake     (handshake_serverbound_handshake),
    lo_sb_login_start   (login_serverbound_login_start),
    pl_cb_spawn_object  (play_clientbound_spawn_object)
*/


// ----------------- Handshake state ------------------

//              --- Serverbound ---
extern const uint8_t MCPR_PKT_HS_SB_HANDSHAKE;




// ----------------- Status state ------------------

//              --- Clientbound ---
extern const uint8_t MCPR_PKT_ST_CB_RESPONSE;
extern const uint8_t MCPR_PKT_ST_CB_PONG;


//              --- Serverbound ---
extern const uint8_t MCPR_PKT_ST_SB_REQUEST;
extern const uint8_t MCPR_PKT_ST_SB_PING;




// ----------------- Login state ------------------

//              --- Clientbound ---
extern const uint8_t MCPR_PKT_LO_CB_DISCONNECT;
extern const uint8_t MCPR_PKT_LO_CB_ENCRYPTION_REQUEST;
extern const uint8_t MCPR_PKT_LO_CB_LOGIN_SUCCESS;
extern const uint8_t MCPR_PKT_LO_CB_SET_COMPRESSION;


//              --- Serverbound ---
extern const uint8_t MCPR_PKT_LO_SB_LOGIN_START;
extern const uint8_t MCPR_PKT_LO_SB_ENCRYPTION_RESPONSE;



// ------------------- Play state ----------------------

//              --- Clientbound ---
extern const uint8_t MCPR_PKT_PL_CB_SPAWN_OBJECT;
struct pl_cb_spawn_object {
    uint8_t packet_id;

    int32_t entity_id;
    uint64_t object_uuid;
    int8_t type;
    double x;
    double y;
    double z;
};


extern const uint8_t MCPR_PKT_PL_CB_SPAWN_EXPERIENCE_ORB;
struct mcpr_pkt_pl_cb_spawn_experience_orb {
    uint8_t packet_id;

    int32_t entity_id;
    double x;
    double y;
    double z;
    int16_t count;
};


extern const uint8_t MCPR_PKT_PL_CB_SPAWN_GLOBAL_ENTITY;
struct mcpr_pkt_pl_cb_spawn_global_entity {
    uint8_t packet_id;

    int32_t entity_id;
    double x;
    double y;
    double z;
};

extern const uint8_t MCPR_PKT_PL_CB_SPAWN_MOB;
struct mcpr_pkt_pl_cb_spawn_mob {
    uint8_t packet_id;

    int32_t entity_id;
    uint64_t entity_uuid;
    uint8_t type;
    double x;
    double y;
    double z;
    uint8_t yaw;
    uint8_t pitch;
    uint8_t head_pitch;
    int16_t velocity_x;
    int16_t velocity_y;
    int16_t velocity_z;
    struct mcpr_entity_metadata_entry *metadata;
};

extern const uint8_t MCPR_PKT_PL_CB_SPAWN_PAINTING;
struct mcpr_pkt_pl_cb_spawn_painting {
    uint8_t packet_id;

    int32_t entity_id;
    uint64_t entity_uuid;
    char *title;
    int64_t position;
    int8_t direction;
};

extern const uint8_t MCPR_PKT_PL_CB_SPAWN_PLAYER;
struct mcpr_pkt_pl_cb_spawn_player {
    uint8_t packet_id;

    int32_t entity_id;
    uint64_t player_uuid;
    double x;
    double y;
    double z;
    uint8_t yaw;
    uint8_t pitch;
    struct mcpr_entity_metadata_entry *metadata;
};

extern const uint8_t MCPR_PKT_PL_CB_ANIMATON;
struct mcpr_pkt_pl_cb_animation {
    uint8_t packet_id;

    int32_t entity_id;
    uint8_t animation;
};
extern const uint8_t MCPR_PKTENUM_PL_CB_ANIMATION_ANIMATION_SWING_MAIN_ARM;
extern const uint8_t MCPR_PKTENUM_PL_CB_ANIMATION_ANIMATION_TAKE_DAMAGE;
extern const uint8_t MCPR_PKTENUM_PL_CB_ANIMATION_ANIMATION_LEAVE_BED;
extern const uint8_t MCPR_PKTENUM_PL_CB_ANIMATION_ANIMATION_SWING_OFFHAND;
extern const uint8_t MCPR_PKTENUM_PL_CB_ANIMATION_ANIMATION_CRITICAL_EFFECT;
extern const uint8_t MCPR_PKTENUM_PL_CB_ANIMATION_ANIMATION_MAGIC_CRITICAL_EFFECT;

extern const uint8_t MCPR_PKT_PL_CB_STATISTICS; // TODO http://wiki.vg/Protocol#Statistics

extern const uint8_t MCPR_PKT_PL_CB_BLOCK_BREAK_ANIMATON;
struct mcpr_pkt_pl_cb_block_break_animation {
    uint8_t packet_id;

    int32_t entity_id;
    int64_t location;
    int8_t destroy_stage;
};

extern const uint8_t MCPR_PKT_PL_CB_UPDATE_BLOCK_ENTITY;
struct mcpr_pkt_pl_cb_update_block_entity {
    uint8_t packet_id;

    int64_t location;
    uint8_t action;
    int8_t *raw_nbt; // TODO
};

extern const uint8_t MCPR_PKT_PL_CB_BLOCK_ACTION;
struct mcpr_pkt_pl_cb_block_action {
    uint8_t packet_id;

    int64_t location;
    uint8_t byte1;
    uint8_t byte2;
    int32_t block_type;
};

extern const uint8_t MCPR_PKT_PL_CB_BLOCK_CHANGE;
struct mcpr_pkt_pl_cb_block_change {
    uint8_t packet_id;

    int64_t location;
    int32_t blockId;
};

extern const uint8_t MCPR_PKT_PL_CB_BOSS_BAR;
struct mcpr_pkt_pl_cb_boss_bar {
    uint8_t packet_id;

    uint64_t uuid;
    int32_t action;
};
struct mcpr_pkt_pl_cb_boss_bar_action0 {
    uint8_t packet_id;

    uint64_t uuid;
    int32_t action;

    char *title_chat;
    float health;
    int32_t color;
    int32_t division;
    uint8_t flags;

};
// Action 1 doesn't actually have to exist since it has no fields and simply means, remove the bossbar.
struct mcpr_pkt_pl_cb_boss_bar_action2 {
    uint8_t packet_id;

    uint64_t uuid;
    int32_t action;

    float health;
};
struct mcpr_pkt_pl_cb_boss_bar_action3 {
    uint8_t packet_id;

    uint64_t uuid;
    int32_t action;

    char *titleChat;
};
struct mcpr_pkt_pl_cb_boss_bar_action4 {
    uint8_t packet_id;

    uint64_t uuid;
    int32_t action;

    int32_t color;
    int32_t dividers;
};
struct mcpr_pkt_pl_cb_boss_bar_action5 {
    uint8_t packet_id;

    uint64_t uuid;
    int32_t action;

    uint8_t flags;
};
extern const int32_t MCPR_PKTENUM_PL_CB_BOSS_BAR_COLOR_PINK;
extern const int32_t MCPR_PKTENUM_PL_CB_BOSS_BAR_COLOR_BLUE;
extern const int32_t MCPR_PKTENUM_PL_CB_BOSS_BAR_COLOR_RED;
extern const int32_t MCPR_PKTENUM_PL_CB_BOSS_BAR_COLOR_GREEN;
extern const int32_t MCPR_PKTENUM_PL_CB_BOSS_BAR_COLOR_YELLOW;
extern const int32_t MCPR_PKTENUM_PL_CB_BOSS_BAR_COLOR_PURPLE;
extern const int32_t MCPR_PKTENUM_PL_CB_BOSS_BAR_COLOR_WHITE;

extern const int32_t MCPR_PKTENUM_PL_CB_BOSS_BAR_DIVISION_NONE;
extern const int32_t MCPR_PKTENUM_PL_CB_BOSS_BAR_DIVISION_6_NOTCHES;
extern const int32_t MCPR_PKTENUM_PL_CB_BOSS_BAR_DIVISION_10_NOTCHES;
extern const int32_t MCPR_PKTENUM_PL_CB_BOSS_BAR_DIVISION_12_NOTCHES;
extern const int32_t MCPR_PKTENUM_PL_CB_BOSS_BAR_DIVISION_20_NOTCHES;



extern const uint8_t PL_CB_SERVER_DIFFICULTY;
struct mcpr_pkt_pl_cb_server_difficulty {
    uint8_t packet_id;

    uint8_t difficulty;
};
extern const uint8_t MCPR_PKTENUM_PL_CB_SERVER_DIFFICULTY_DIFFICULTY_PEACEFUL;
extern const uint8_t MCPR_PKTENUM_PL_CB_SERVER_DIFFICULTY_DIFFICULTY_EASY;
extern const uint8_t MCPR_PKTENUM_PL_CB_SERVER_DIFFICULTY_DIFFICULTY_NORMAL;
extern const uint8_t MCPR_PKTENUM_PL_CB_SERVER_DIFFICULTY_DIFFICULTY_HARD;

extern const uint8_t PL_CB_TAB_COMPLETE;
struct mcpr_pkt_pl_cb_tab_complete {
    uint8_t packet_id;

    int32_t count;
    char **matches; // an array of strings.
};

extern const uint8_t MCPR_PKT_PL_CB_CHAT_MESSAGE;
struct mcpr_pkt_pl_cb_chat_message {
    uint8_t packet_id;

    char *json_data_chat;
    int8_t position;
};
extern const int8_t MCPR_PKTENUM_PL_CB_CHAT_MESSAGE_POSITION_CHAT;
extern const int8_t MCPR_PKTENUM_PL_CB_CHAT_MESSAGE_POSITION_SYSTEM;
extern const int8_t MCPR_PKTENUM_PL_CB_CHAT_MESSAGE_POSITION_HOTBAR;

extern const uint8_t MCPR_PKT_PL_CB_MULTI_BLOCK_CHANGE;
struct mcpr_pkt_pl_cb_multi_block_change_record {
    uint8_t horizontal_position;
    uint8_t y_coordinate;
    int32_t block_id;
};
struct mcpr_pkt_pl_cb_multi_block_change {
    uint8_t packet_id;

    int32_t chunk_x;
    int32_t chunk_z;
    int32_t record_count;
    struct mcpr_pkt_pl_cb_multi_block_change_record *records;
};

extern const uint8_t MCPR_PKT_PL_CB_CONFIRM_TRANSACTION;
struct mcpr_pkt_pl_cb_confirm_transaction {
    uint8_t packet_id;

    int8_t window_id;
    int16_t action_number;
    bool accepted;
};

extern const uint8_t MCPR_PKT_PL_CB_CLOSE_WINDOW;
struct mcpr_pkt_pl_cb_close_window {
    uint8_t packet_id;

    uint8_t window_id;
};

extern const uint8_t MCPR_PKT_PL_CB_OPEN_WINDOW;
struct mcpr_pkt_pl_cb_close_window {
    uint8_t packet_id;

    uint8_t window_id;
    char *window_type;
    char *window_title; // chat type.
    uint8_t slot_count;
    int32_t *entity_id; // NULLable
};

extern const uint8_t MCPR_PKT_PL_CB_WINDOW_ITEMS;
struct mcpr_pkt_pl_cb_window_items {
    uint8_t packet_id;

    uint8_t window_id;
    int16_t count;
    struct slot *slots;
};

extern const uint8_t MCPR_PKT_PL_CB_WINDOW_PROPERTY;
struct mcpr_pkt_pl_cb_window_property {
    uint8_t packet_id;

    uint8_t window_id;
    int16_t property;
    int16_t value;
};

extern const int16_t MCPR_PKTENUM_PL_CB_WINDOW_PROPERTY_PROPERTY_FURNACE_FUEL_REMAINING;
extern const int16_t MCPR_PKTENUM_PL_CB_WINDOW_PROPERTY_PROPERTY_FURNACE_MAX_FUEL_BURN_TIME;
extern const int16_t MCPR_PKTENUM_PL_CB_WINDOW_PROPERTY_PROPERTY_FURNACE_PROGRESS;
extern const int16_t MCPR_PKTENUM_PL_CB_WINDOW_PROPERTY_PROPERTY_FURNACE_MAX_PROGRESS;
extern const int16_t MCPR_PKTENUM_PL_CB_WINDOW_PROPERTY_PROPERTY_ENCHTABLE_LVL_REQUIRED_TOP;
extern const int16_t MCPR_PKTENUM_PL_CB_WINDOW_PROPERTY_PROPERTY_ENCHTABLE_LVL_REQUIRED_MID;
extern const int16_t MCPR_PKTENUM_PL_CB_WINDOW_PROPERTY_PROPERTY_ENCHTABLE_LVL_REQUIRED_BOTTOM;
extern const int16_t MCPR_PKTENUM_PL_CB_WINDOW_PROPERTY_PROPERTY_ENCHTABLE_SEED;
extern const int16_t MCPR_PKTENUM_PL_CB_WINDOW_PROPERTY_PROPERTY_ENCHTABLE_ENCHHOVER_TOP;
extern const int16_t MCPR_PKTENUM_PL_CB_WINDOW_PROPERTY_PROPERTY_ENCHTABLE_ENCHHOVER_MID;
extern const int16_t MCPR_PKTENUM_PL_CB_WINDOW_PROPERTY_PROPERTY_ENCHTABLE_ENCHHOVER_BOTTOM;
extern const int16_t MCPR_PKTENUM_PL_CB_WINDOW_PROPERTY_PROPERTY_BEACON_POWER_LVL;
extern const int16_t MCPR_PKTENUM_PL_CB_WINDOW_PROPERTY_PROPERTY_BEACON_POTION_EFFECT_FIRST;
extern const int16_t MCPR_PKTENUM_PL_CB_WINDOW_PROPERTY_PROPERTY_BEACON_POTION_EFFECT_SECOND;
extern const int16_t MCPR_PKTENUM_PL_CB_WINDOW_PROPERTY_PROPERTY_ANVIL_ // TODO seriously this gets way too long.


extern const uint8_t MCPR_PKT_PL_CB_SET_SLOT;
extern const uint8_t MCPR_PKT_PL_CB_SET_COOLDOWN;
extern const uint8_t MCPR_PKT_PL_CB_PLUGIN_MESSAGE;
extern const uint8_t MCPR_PKT_PL_CB_NAMED_SOUND_EFFECT;
extern const uint8_t MCPR_PKT_PL_CB_DISCONNECT;
extern const uint8_t MCPR_PKT_PL_CB_ENTITY_STATUS;
extern const uint8_t MCPR_PKT_PL_CB_EXPLOSION;
extern const uint8_t MCPR_PKT_PL_CB_UNLOAD_CHUNK;
extern const uint8_t MCPR_PKT_PL_CB_CHANGE_GAME_STATE;
extern const uint8_t MCPR_PKT_PL_CB_KEEP_ALIVE;
extern const uint8_t MCPR_PKT_PL_CB_CHUNK_DATA;
extern const uint8_t MCPR_PKT_PL_CB_EFFECT;
extern const uint8_t MCPR_PKT_PL_CB_PARTICLE;
extern const uint8_t MCPR_PKT_PL_CB_JOIN_GAME;
extern const uint8_t MCPR_PKT_PL_CB_MAP;
extern const uint8_t MCPR_PKT_PL_CB_ENTITY_RELATIVE_MOVE;
extern const uint8_t MCPR_PKT_PL_CB_ENTITY_LOOK_AND_RELATIVE_MOVE;
extern const uint8_t MCPR_PKT_PL_CB_ENTITY_LOOK;
extern const uint8_t MCPR_PKT_PL_CB_ENTITY;
extern const uint8_t MCPR_PKT_PL_CB_VEHICLE_MOVE;
extern const uint8_t MCPR_PKT_PL_CB_OPEN_SIGN_EDITOR;
extern const uint8_t MCPR_PKT_PL_CB_PLAYER_ABILITIES;
extern const uint8_t MCPR_PKT_PL_CB_COMBAT_EVENT;
extern const uint8_t MCPR_PKT_PL_CB_PLAYER_LIST_ITEM;
extern const uint8_t MCPR_PKT_PL_CB_PLAYER_POSITION_AND_LOOK;
extern const uint8_t MCPR_PKT_PL_CB_USE_BED;
extern const uint8_t MCPR_PKT_PL_CB_DESTROY_ENTITIES;
extern const uint8_t MCPR_PKT_PL_CB_REMOVE_ENTITY_EFFECT;
extern const uint8_t MCPR_PKT_PL_CB_RESOURCE_PACK_SEND;
extern const uint8_t MCPR_PKT_PL_CB_RESPAWN;
extern const uint8_t MCPR_PKT_PL_CB_ENTITY_HEAD_LOOK;
extern const uint8_t MCPR_PKT_PL_CB_WORLD_BORDER;
extern const uint8_t MCPR_PKT_PL_CB_CAMERA;
extern const uint8_t MCPR_PKT_PL_CB_HELD_ITEM_CHANGE;
extern const uint8_t MCPR_PKT_PL_CB_DISPLAY_SCOREBOARD;
extern const uint8_t MCPR_PKT_PL_CB_ENTITY_METADATA;
extern const uint8_t MCPR_PKT_PL_CB_ATTACH_ENTITY;
extern const uint8_t MCPR_PKT_PL_CB_ENTITY_VELOCITY;
extern const uint8_t MCPR_PKT_PL_CB_ENTITY_EQUIPMENT;
extern const uint8_t MCPR_PKT_PL_CB_SET_EXPERIENCE;
extern const uint8_t MCPR_PKT_PL_CB_UPDATE_HEALTH;
extern const uint8_t MCPR_PKT_PL_CB_SCOREBOARD_OBJECTIVE;
extern const uint8_t MCPR_PKT_PL_CB_SET_PASSENGERS;
extern const uint8_t MCPR_PKT_PL_CB_TEAMS;
extern const uint8_t MCPR_PKT_PL_CB_UPDATE_SCORE;
extern const uint8_t MCPR_PKT_PL_CB_SPAWN_POSITION;
extern const uint8_t MCPR_PKT_PL_CB_TIME_UPDATE;
extern const uint8_t MCPR_PKT_PL_CB_TITLE;
extern const uint8_t MCPR_PKT_PL_CB_SOUND_EFFECT;
extern const uint8_t MCPR_PKT_PL_CB_PLAYER_LIST_HEADER_AND_FOOTER;
extern const uint8_t MCPR_PKT_PL_CB_COLLECT_ITEM;
extern const uint8_t MCPR_PKT_PL_CB_ENTITY_TELEPORT;
extern const uint8_t MCPR_PKT_PL_CB_ENTITY_PROPERTIES;
extern const uint8_t MCPR_PKT_PL_CB_ENTITY_EFFECT;

//              --- Serverbound ---
extern const uint8_t MCPR_PKT_PL_SB_TELEPORT_CONFIRM;
extern const uint8_t MCPR_PKT_PL_SB_TAB_COMPLETE;
extern const uint8_t MCPR_PKT_PL_SB_CHAT_MESSAGE;
extern const uint8_t MCPR_PKT_PL_SB_CLIENT_STATUS;
extern const uint8_t MCPR_PKT_PL_SB_CLIENT_SETTINGS;
extern const uint8_t MCPR_PKT_PL_SB_CONFIRM_TRANSACTION;
extern const uint8_t MCPR_PKT_PL_SB_ENCHANT_ITEM;
extern const uint8_t MCPR_PKT_PL_SB_CLICK_WINDOW;
extern const uint8_t MCPR_PKT_PL_SB_CLOSE_WINDOW;
extern const uint8_t MCPR_PKT_PL_SB_PLUGIN_MESSAGE;
extern const uint8_t MCPR_PKT_PL_SB_USE_ENTITY;
extern const uint8_t MCPR_PKT_PL_SB_KEEP_ALIVE;
extern const uint8_t MCPR_PKT_PL_SB_PLAYER_POSITION;
extern const uint8_t MCPR_PKT_PL_SB_PLAYER_POSITION_AND_LOOK;
extern const uint8_t MCPR_PKT_PL_SB_PLAYER_LOOK;
extern const uint8_t MCPR_PKT_PL_SB_PLAYER;
extern const uint8_t MCPR_PKT_PL_SB_VEHICLE_MOVE;
extern const uint8_t MCPR_PKT_PL_SB_STEER_BOAT;
extern const uint8_t MCPR_PKT_PL_SB_PLAYER_ABILITIES;
extern const uint8_t MCPR_PKT_PL_SB_PLAYER_DIGGING;
extern const uint8_t MCPR_PKT_PL_SB_ENTITY_ACTION;
extern const uint8_t MCPR_PKT_PL_SB_STEER_VEHICLE;
extern const uint8_t MCPR_PKT_PL_SB_RESOURCE_PACK_STATUS;
extern const uint8_t MCPR_PKT_PL_SB_HELD_ITEM_CHANGE;
extern const uint8_t MCPR_PKT_PL_SB_CREATIVE_INVENTORY_ACTION;
extern const uint8_t MCPR_PKT_PL_SB_UPDATE_SIGN;
extern const uint8_t MCPR_PKT_PL_SB_ANIMATION;
extern const uint8_t MCPR_PKT_PL_SB_SPECTATE;
extern const uint8_t MCPR_PKT_PL_SB_PLAYER_BLOCK_PLACEMENT;
extern const uint8_t MCPR_PKT_PL_SB_USE_ITEM;


    #ifdef MCPR_SHORT_NAMES

        // TODO: #define short_func_name long_very_long_func_name
        // And all other identifiers, ofcourse.

    #endif
#endif

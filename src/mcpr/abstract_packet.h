#ifndef MCPR_ABSTRACT_PACKET_H
#define MCPR_ABSTRACT_PACKET_H

#include <stdbool.h>
#include <stdint.h>

#include <jansson/jansson.h>

#include "mcpr.h"

#define MCPR_PKT_HS_SB_HANDSHAKE 0x00;

enum mcpr_resource_pack_result
{
    MCPR_RESOURCE_PACK_RESULT_SUCCESS,
    MCPR_RESOURCE_PACK_RESULT_DECLINED,
    MCPR_RESOURCE_PACK_RESULT_FAILED,
    MCPR_RESOURCE_PACK_RESULT_ACCEPTED
};

enum mcpr_entity_action
{
    MCPR_ENTITY_ACTION_START_SNEAKING,
    MCPR_ENTITY_ACTION_STOP_SNEAKING,
    MCPR_ENTITY_ACTION_LEAVE_BED,
    MCPR_ENTITY_ACTION_START_SPRINTING,
    MCPR_ENTITY_ACTION_STOP_SPRINTING,
    MCPR_ENTITY_ACTION_START_HORSE_JUMP,
    MCPR_ENTITY_ACTION_STOP_HORSE_JUMP,
    MCPR_ENTITY_ACTION_OPEN_HORSE_INVENTORY,
    MCPR_ENTITY_ACTION_START_ELYTRA_FLYING
};

enum mcpr_player_digging_status
{
    MCPR_PLAYER_DIGGING_STATUS_STARTED,
    MCPR_PLAYER_DIGGING_STATUS_CANCELLED,
    MCPR_PLAYER_DIGGING_STATUS_FINISHED,
    MCPR_PLAYER_DIGGING_STATUS_DROP_ITEM_STACK,
    MCPR_PLAYER_DIGGING_STATUS_DROP_ITEM,
    MCPR_PLAYER_DIGGING_STATUS_SHOOT_ARROW_FINISH_EATING,
    MCPR_PLAYER_DIGGING_STATUS_SWAP_ITEM_IN_HAND
};

enum mcpr_block_face
{
    MCPR_BLOCK_FACE_TOP,
    MCPR_BLOCK_FACE_BOTTOM,
    MCPR_BLOCK_FACE_NORTH,
    MCPR_BLOCK_FACE_SOUTH,
    MCPR_BLOCK_FACE_WEST,
    MCPR_BLOCK_FACE_EAST,
    MCPR_BLOCK_FACE_SPECIAL
};

enum mcpr_side_based_hand
{
    MCPR_HAND_RIGHT,
    MCPR_HAND_LEFT
};

enum mcpr_hand
{
    MCPR_HAND_MAIN,
    MCPR_HAND_OFFHAND,
};

enum mcpr_chat_mode
{
    MCPR_CHAT_MODE_ENABLED,
    MCPR_CHAT_MODE_COMMANDS_ONLY,
    MCPR_CHAT_MODE_HIDDEN
};

enum mcpr_client_status_action
{
    MCPR_CLIENT_STATUS_ACTION_PERFORM_RESPAWN,
    MCPR_CLIENT_STATUS_ACTION_REQUEST_STATS,
    MCPR_CLIENT_STATUS_ACTION_OPEN_INVENTORY
};

enum mcpr_entity_property_modifier_operation
{
    MCPR_ENTITY_PROPERTY_MODIFIER_OPERATION_ADD_SUBTRACT,
    MCPR_ENTITY_PROPERTY_MODIFIER_OPERATION_ADD_SUBTRACT_PERCENT,
    MCPR_ENTITY_PROPERTY_MODIFIER_OPERATION_MULTIPLY_PERCENT
};

struct mcpr_entity_property_modifier
{
    struct mcpr_uuid uuid;
    double amount;
    enum mcpr_entity_property_modifier_operation operation;
};

struct mcpr_entity_property
{
    enum mcpr_entity_property_key key;
    double value;
    int32_t number_of_modifiers;
    struct mcpr_entity_property_modifier *modifiers;
};

enum mcpr_sound_category
{
    MCPR_SOUND_CATEGORY_MASTER,
    MCPR_SOUND_CATEGORY_MUSIC,
    MCPR_SOUND_CATEGORY_RECORD,
    MCPR_SOUND_CATEGORY_WEATHER,
    MCPR_SOUND_CATEGORY_BLOCK,
    MCPR_SOUND_CATEGORY_HOSTILE,
    MCPR_SOUND_CATEGORY_NEUTRAL,
    MCPR_SOUND_CATEGORY_PLAYER,
    MCPR_SOUND_CATEGORY_AMBIENT,
    MCPR_SOUND_CATEGORY_VOICE
};

enum mcpr_title_action
{
    MCPR_TITLE_ACTION_SET_TITLE,
    MCPR_TITLE_ACTION_SET_SUBTITLE,
    MCPR_TITLE_ACTION_SET_ACTION_BAR,
    MCPR_TITLE_ACTION_SET_TIMES_AND_DISPLAY,
    MCPR_TITLE_ACTION_HIDE,
    MCPR_TITLE_ACTION_RESET
};

enum mcpr_update_score_action
{
    MCPR_UPDATE_SCORE_ACTION_REMOVE,
    MCPR_UPDATE_SCORE_ACTION_UPDATE
};

enum mcpr_teams_action
{
    MCPR_TEAMS_ACTION_CREATE,
    MCPR_TEAMS_ACTION_UPDATE_INFO,
    MCPR_TEAMS_ACTION_ADD_PLAYERS,
    MCPR_TEAMS_ACTION_REMOVE_PLAYERS
};

enum mcpr_scoreboard_objective_mode
{
    MCPR_SCOREBOARD_OBJECTIVE_MODE_CREATE,
    MCPR_SCOREBOARD_OBJECTIVE_MODE_REMOVE,
    MCPR_SCOREBOARD_OBJECTIVE_MODE_UPDATE
};

enum mcpr_scoreboard_position
{
    MCPR_SCOREBOARD_POSITION_LIST,
    MCPR_SCOREBOARD_POSITION_SIDEBAR,
    MCPR_SCOREBOARD_POSITION_BELOW_NAME
};

enum mcpr_world_border_action
{
    MCPR_WORLD_BORDER_ACTION_SET_SIZE,
    MCPR_WORLD_BORDER_ACTION_LERP_SIZE,
    MCPR_WORLD_BORDER_ACTION_SET_CENTER,
    MCPR_WORLD_BORDER_ACTION_INITIALIZE,
    MCPR_WORLD_BORDER_ACTION_SET_WARNING_TIME,
    MCPR_WORLD_BORDER_ACTION_SET_WARNING_BLOCKS,
};


enum mcpr_gamemode
{
    MCPR_GAMEMODE_SURVIVAL,
    MCPR_GAMEMODE_CREATIVE,
    MCPR_GAMEMODE_ADVENTURE,
    MCPR_GAMEMODE_SPECTATOR,
};

enum mcpr_level
{
    MCPR_LEVEL_DEFAULT,
    MCPR_LEVEL_FLAT,
    MCPR_LEVEL_LARGE_BIOMES,
    MCPR_LEVEL_AMPLIFIED,
    MCPR_LEVEL_DEFAULT_1_1
};

enum mcpr_player_list_item_action
{
    MCPR_PLAYER_LIST_ITEM_ACTION_ADD_PLAYER,
    MCPR_PLAYER_LIST_ITEM_ACTION_UPDATE_GAMEMODE,
    MCPR_PLAYER_LIST_ITEM_ACTION_UPDATE_LATENCY,
    MCPR_PLAYER_LIST_ITEM_ACTION_UPDATE_DISPLAY_NAME,
    MCPR_PLAYER_LIST_ITEM_ACTION_REMOVE_PLAYER
};

struct mcpr_player_list_item_property
{
    char *name;
    char *value;
    bool is_signed;
    char *signature; // Optional, only if is_signed is true
};

struct mcpr_player_list_item_player
{
    struct mcpr_uuid uuid;

    struct
    {
        char *name;
        int32_t number_of_properties;
        struct mcpr_player_list_item_property *properties;
    } action_add_player;

    struct
    {
        enum mcpr_gamemode gamemode;
    } action_update_gamemode;

    struct
    {
        int32_t ping;
    } action_update_latency;

    struct
    {
        bool has_display_name;
        char display_name; // Optional, only if has_display_name is true.
    } action_update_display_name;
};

enum mcpr_effect
{
    MCPR_EFFECT_DISPENSER_DISPENSE,
    MCPR_EFFECT_DISPENSER_DISPENSE_FAILED,
    MCPR_EFFECT_DISPENSER_SHOOT,
    MCPR_EFFECT_ENDER_EYE_LAUNCH,
    MCPR_EFFECT_FIREWORK_SHOT,
    MCPR_EFFECT_IRON_DOOR_OPEN,
    MCPR_EFFECT_WOODEN_DOOR_OPEN,
    MCPR_EFFECT_WOODEN_TRAPDOOR_OPEN,
    MCPR_EFFECT_FENCE_GATE_OPEN,
    MCPR_EFFECT_FIRE_EXTINGUISH,
    MCPR_EFFECT_PLAY_RECORD,
    MCPR_EFFECT_IRON_DOOR_CLOSE,
    MCPR_EFFECT_WOODEN_DOOR_CLOSE,
    MCPR_EFFECT_WOODEN_TRAPDOOR_CLOSE,
    MCPR_EFFECT_FENCE_GATE_CLOSE,
    MCPR_EFFECT_GHAST_WARN,
    MCPR_EFFECT_GHAST_SHOOT,
    MCPR_EFFECT_ENDERDRAGON_SHOOT,
    MCPR_EFFECT_BLAZE_SHOOT,
    MCPR_EFFECT_ZOMBIE_ATTACK_WOOD_DOOR,
    MCPR_EFFECT_ZOMBIE_ATTACK_IRON_DOOR,
    MCPR_EFFECT_ZOMBIE_BREAK_WOOD_DOOR,
    MCPR_EFFECT_WITHER_BREAK_BLOCK,
    MCPR_EFFECT_WITHER_SPAWN,
    MCPR_EFFECT_WITHER_SHOOT,
    MCPR_EFFECT_BAT_TAKEOFF,
    MCPR_EFFECT_ZOMBIE_INFECT,
    MCPR_EFFECT_ZOMBIE_VILLAGER_CONVERT,
    MCPR_EFFECT_ENDERDRAGON_DEATH,
    MCPR_EFFECT_ANVIL_DESTROY,
    MCPR_EFFECT_ANVIL_USE,
    MCPR_EFFECT_ANVIL_LAND,
    MCPR_EFFECT_PORTAL_TRAVEL,
    MCPR_EFFECT_CHORUS_FLOWER_GROW,
    MCPR_EFFECT_CHORUS_FLOWER_DIE,
    MCPR_EFFECT_BREWING_STAND_BREW,
    MCPR_EFFECT_IRON_TRAPDOOR_OPEN,
    MCPR_EFFECT_IRON_TRAPDOOR_CLOSE,
    MCPR_EFFECT_SPAWN_10_SMOKE_PARTICLES, // For example for fires.
    MCPR_EFFECT_BLOCK_BREAK,
    MCPR_EFFECT_SPLASH_POTION,
    MCPR_EFFECT_EYE_OF_ENDER_BREAK,
    MCPR_EFFECT_MOB_SPAWN_PARTICLE,
    MCPR_EFFECT_BONEMEAL_PARTICLES,
    MCPR_EFFECT_DRAGON_BREATH,
    MCPR_EFFECT_END_GATEWAY_SPAWN,
    MCPR_EFFECT_ENDERDRAGON_GROWL
};

enum mcpr_particle
{
    MCPR_PARTICLE_EXPLODE,
    MCPR_PARTICLE_LARGE_EXPLOSION,
    MCPR_PARTICLE_HUGE_EXPLOSION,
    MCPR_PARTICLE_FIREWORKS_SPARK,
    MCPR_PARTICLE_BUBBLE,
    MCPR_PARTICLE_SPLASH,
    MCPR_PARTICLE_WAKE,
    MCPR_PARTICLE_SUSPENDED,
    MCPR_PARTICLE_DEPTH_SUSPEND,
    MCPR_PARTICLE_CRIT,
    MCPR_PARTICLE_MAGIC_CRIT,
    MCPR_PARTICLE_SMOKE,
    MCPR_PARTICLE_LARGE_SMOKE,
    MCPR_PARTICLE_SPELL,
    MCPR_PARTICLE_INSTANT_SPELL,
    MCPR_PARTICLE_MOB_SPELL,
    MCPR_PARTICLE_MOB_SPELL_AMBIENT,
    MCPR_PARTICLE_WITCH_MAGIC,
    MCPR_PARTICLE_DRIP_WATER,
    MCPR_PARTICLE_DRIP_LAVA,
    MCPR_PARTICLE_ANGRY_VILLAGER,
    MCPR_PARTICLE_HAPPY_VILLAGER,
    MCPR_PARTICLE_TOWN_AURA,
    MCPR_PARTICLE_NOTE,
    MCPR_PARTICLE_PORTAL,
    MCPR_PARTICLE_ENCHANTMENT_TABLE,
    MCPR_PARTICLE_FLAME,
    MCPR_PARTICLE_LAVA,
    MCPR_PARTICLE_FOOTSTEP,
    MCPR_PARTICLE_CLOUD,
    MCPR_PARTICLE_RED_DUST,
    MCPR_PARTICLE_SNOWBALL_PROOF,
    MCPR_PARTICLE_SNOW_SHOVEL,
    MCPR_PARTICLE_SLIME,
    MCPR_PARTICLE_HEART,
    MCPR_PARTICLE_BARRIER,
    MCPR_PARTICLE_IRON_CRACK,
    MCPR_PARTICLE_BLOCK_CRACK,
    MCPR_PARTICLE_BLOCK_DUST,
    MCPR_PARTICLE_DROPLET,
    MCPR_PARTICLE_TAKE,
    MCPR_PARTICLE_MOB_APPEARANCE,
    MCPR_PARTICLE_DRAGON_BREATH,
    MCPR_PARTICLE_END_ROD,
    MCPR_PARTICLE_DAMAGE_INDICATOR,
    MCPR_PARTICLE_SWEEP_ATTACK,
    MCPR_PARTICLE_FALING_DUST
};

enum mcpr_difficulty
{
    MCPR_DIFFICULTY_PEACEFUL,
    MCPR_DIFFICULTY_EASY,
    MCPR_DIFFICULTY_NORMAL,
    MCPR_DIFFICULTY_HARD
};

enum mcpr_chat_position
{
    MCPR_CHAT_POSITION_CHAT,
    MCPR_CHAT_POSITION_SYSTEM,
    MCPR_CHAT_POSITION_HOTBAR
};

struct mcpr_multi_block_change
{
    uint8_t horizontal_position_x;
    uint8_t horizontal_position_z;
    uint8_t y_coordinate;
    int32_t block_id;
};

struct mcpr_explosion_record
{
    int8_t x, y, z;
};

enum mcpr_map_icon
{
    MCPR_MAP_ICON_WHITE_ARROW,
    MCPR_MAP_ICON_GREEN_ARROW,
    MCPR_MAP_ICON_RED_ARROW,
    MCPR_MAP_ICON_BLUE_ARROW,
    MCPR_MAP_ICON_WHITE_CROSS,
    MCPR_MAP_ICON_RED_POINTER,
    MCPR_MAP_ICON_WHITE_CIRCLE,
    MCPR_MAP_ICON_SMALL_WHITE_CIRCLE,
    MCPR_MAP_ICON_MANSION,
    MCPR_MAP_ICON_TEMPLE,
};

struct mcpr_map_icon
{
    uint8_t direction;
    enum mcpr_map_icon type;
    int8_t x, z;
};

enum mcpr_combat_event
{
    MCPR_COMBAT_EVENT_ENTER_COMBAT,
    MCPR_COMBAT_EVENT_END_COMBAT,
    MCPR_COMBAT_EVENT_ENTITY_DEAD
};

struct mcpr_abstract_packet
{
    int8_t id;
    union
    {
        struct // handshake
        {
            union // handshake - serverbound
            {
                struct
                {
                    int32_t protocol_version;
                    char *server_address;
                    uint16_t server_port;
                    enum mcpr_state next_state;
                } handshake;
            } serverbound;

            union // handshake - clientbound
            {

            } clientbound;
        } handshake;

        struct // login
        {
            union // login - serverbound
            {
                struct
                {
                    char *name;
                } login_start;

                struct
                {
                    int32_t shared_secret_length;
                    void *shared_secret;
                    int32_t verify_token_length;
                    void *verify_token;
                } encryption_response;
            } serverbound;

            union // login - clientbound
            {
                struct
                {
                    struct mcpr_chat_message reason;
                } disconnect;

                struct
                {
                    char *server_id;
                    int32_t public_key_length;
                    void *public_key;
                    int32_t verify_token_length;
                    void *verify_token;
                } encryption_request;

                struct
                {
                    struct mcpr_uuid uuid;
                    char *username;
                } login_success;

                struct
                {
                    int32_t threshold;
                } set_compression;
            } clientbound;
        } login;

        struct // status
        {
            union // status - serverbound
            {
                struct
                {

                } request;

                struct
                {
                    int64_t payload;
                } ping;
            } serverbound;

            union // status - clientbound
            {
                struct
                {
                int64_t payload;
                } pong;

                struct
                {
                    char json_response;
                } response;
            } clientbound;
        } status;

        struct
        { // play
            union // play - serverbound
            {
                struct
                {
                    int32_t teleport_id;
                } teleport_confirm;

                struct
                {
                    char *text;
                    bool assume_command;
                    bool has_position;
                    struct mcpr_position looked_at_block; // Optional, only if has_position is true.
                } tab_complete;

                struct
                {
                    char *message;
                } chat_message;

                struct
                {
                    enum mcpr_client_status_action action;
                } client_status;

                struct
                {
                    char *locale;
                    int8_t view_distance;
                    enum mcpr_chat_mode chat_mode;
                    bool chat_colors;

                    struct
                    {
                        bool cape_enabled;
                        bool jacket_enabled;
                        bool left_sleeve_enabled, right_sleeve_enabled;
                        bool left_pants_enabled, right_pants_enabled;
                        bool hat_enabled;
                    } displayed_skin_parts;

                    enum mcpr_side_based_hand main_hand;
                } client_settings;

                struct
                {
                    int8_t window_id;
                    int16_t action_number;
                    bool accepted;
                } confirm_transaction;

                struct
                {
                    int8_t window_id;
                    int8_t enchantment;
                } enchant_item;

                struct
                {
                    uint8_t window_id;
                    int16_t slot;
                    int8_t button;
                    int16_t action_number;
                    int32_t mode;
                    struct mcpr_slot clicked_item;
                } click_window;

                struct
                {
                    uint8_t window_id;
                } close_window;

                struct
                {
                    char *channel;
                    size_t data_length;
                    void *data;
                } plugin_message;

                struct
                {
                    int32_t target;
                    enum mcpr_use_entity_type type;
                    float target_x, target_y, target_z; // Optional, only if type is MCPR_USE_ENTITY_TYPE_INTERACT_AT
                    enum mcpr_hand hand; // Optional, only if type is MCPR_USE_ENTITY_TYPE_INTERACT_AT
                } use_entity;

                struct
                {
                    int32_t keep_alive_id;
                } keep_alive;

                struct
                {
                    double x, feet_y, z;
                    bool on_ground;
                } player_position;

                struct
                {
                    double x, feet_y, z;
                    float yaw, pitch;
                    bool on_ground;
                } player_position_and_look;

                struct
                {
                    float yaw, pitch;
                    bool on_ground;
                } player_look;

                struct
                {
                    bool on_ground;
                } player;

                struct
                {
                    double x, y ,z;
                    float yaw, pitch;
                } vehicle_move;

                struct
                {
                    bool right_paddle_turning, left_paddle_turning;
                } steer_boat;

                struct
                {
                    bool invulnerable;
                    bool can_fly;
                    bool is_flying;
                    bool is_creative;
                    float flying_speed;
                    float walking_speed;
                } player_abilities;

                struct
                {
                    enum mcpr_player_digging_status status;
                    struct mcpr_position block_position;
                    enum mcpr_block_face face;
                } player_digging;

                struct
                {
                    int32_t entity_id;
                    enum mcpr_entity_action action;
                    int32_t jump_boost;
                } entity_action;

                struct
                {
                    float sideways, forward;
                    bool jump;
                    bool unmount;
                } steer_vehicle;

                struct
                {
                    enum mcpr_resource_pack_result result;
                } resource_pack_status;

                struct
                {
                    int16_t slot;
                } held_item_change;

                struct
                {
                    int16_t slot;
                    struct mcpr_slot clicked_item;
                } creative_inventory_action;

                struct
                {
                    struct mcpr_position sign_location;
                    char *line_1, line_2, line_3, line_4;
                } update_sign;

                struct
                {
                    enum mcpr_hand hand;
                } animation;

                struct
                {
                    struct  mcpr_uuid target_player;
                } spectate;

                struct
                {
                    struct mcpr_position block_position;
                    enum mcpr_block_face face;
                    enum mcpr_hand hand;
                    float cursor_position_x, cursor_position_y, cursor_position_z;
                } player_block_placement;

                struct
                {
                    enum mcpr_hand hand;
                } use_item;
            } serverbound;

            union // play - clientbound
            {
                struct
                {
                    int32_t entity_id;
                    struct mcpr_uuid uuid;
                    enum mcpr_object type;
                    double x, y, z;
                    uint8_t pitch;
                    uint8_t yaw;
                    int32_t data;
                    int16_t velocity_x;
                    int16_t velocity_y;
                    int16_t velocity_z;
                } spawn_object;

                struct
                {
                    int32_t entity_id;
                    double x, y, z;
                    int16_t count;
                } spawn_experience_orb;

                struct
                {
                    int32_t entity_id;
                    int8_t type;
                    double x, y, z;
                } spawn_global_entity;

                struct
                {
                    int32_t entity_id;
                    struct mcpr_uuid entity_uuid;
                    enum mcpr_mob type;
                    double x, y, z;
                    int8_t yaw;
                    int8_t pitch;
                    int8_t head_pitch;
                    int16_t velocity_x, velocity_y, velocity_z;
                    struct mcpr_entity_metadata metadata;
                } spawn_mob;

                struct
                {
                    int32_t entity_id;
                    struct mcpr_uuid entity_uuid;
                    enum mcpr_painting;
                    struct mcpr_position position;
                    int8_t direction;
                } spawn_painting;

                struct
                {
                    int32_t entity_id;
                    struct mcpr_uuid player_uuid;
                    double x, y, z;
                    int8_t yaw;
                    int8_t pitch;
                    struct mcpr_entity_metadata metadata;
                } spawn_player;

                struct
                {
                    int32_t entity_id;
                    enum mcpr_animation animation;
                } animation;

                struct
                {
                    int32_t count;
                    struct mcpr_statistic *statistics;
                } statistics;

                struct
                {
                    int32_t entity_id;
                    struct mcpr_position location;
                    int8_t destroy_stage;
                } block_break_animation;

                struct
                {
                    struct mcpr_position location;
                    enum mcpr_update_block_entity_action action;
                    nbt_t *nbt;
                } update_block_entity;

                struct
                {
                    struct mcpr_position location;
                    enum mcpr_block_action action_param;
                    enum mcpr_block block_type;
                } block_action;

                struct
                {
                    struct mcpr_position location;
                    int32_t block_id;
                } block_change;

                struct
                {
                    struct mcpr_uuid uuid;
                    enum mcpr_boss_bar_action action;

                    union {
                        struct
                        {
                            char *title;
                            float health;
                            enum mcpr_boss_bar_color color;
                            enum mcpr_boss_bar_division division;
                            struct
                            {
                                bool darken_sky;
                                bool dragon_bar;
                            } flags;
                        } action_add;

                        struct
                        {

                        } action_remove;

                        struct
                        {
                            float health;
                        } action_update_health;

                        struct
                        {
                            char *title;
                        } action_update_title;

                        struct
                        {
                            enum mcpr_boss_bar_color color;
                            enum mcpr_boss_bar_division division;
                        } action_update_style;

                        struct
                        {
                            struct
                            {
                                bool darken_sky;
                                bool dragon_bar;
                            } flags;
                        } action_update_flags;
                    };
                } boss_bar;

                struct
                {
                    enum mcpr_difficulty difficulty;
                } server_difficulty;

                struct
                {
                    int32_t count;
                    char **matches;
                } tab_complete;

                struct
                {
                    char *json_data;
                    enum mcpr_chat_position;
                } chat_message;

                struct
                {
                    int32_t chunk_x, chunk_z;
                    int32_t record_count;
                    struct mcpr_multi_block_change_record *records;
                } multi_block_change;

                struct
                {
                    uint8_t window_id;
                    uint16_t action_number;
                    bool accepted;
                } confirm_transaction;

                struct
                {
                    uint8_t window_id;
                } close_window;

                struct
                {
                    uint8_t window_id;
                    enum mcpr_window window_type;
                    char window_title;
                    uint8_t number_of_slots;
                    int32_t entity_id; // Optional, only sent if window type is "EntityHorse"
                } open_window;

                struct
                {
                    uint8_t window_id;
                    int16_t count;
                    struct mcpr_slot *slots;
                } window_items;

                struct
                {
                    uint8_t window_id;
                    enum mcpr_window_property property;
                    int16_t value;
                } window_property;

                struct
                {
                    uint8_t window_id;
                    int16_t slot;
                    struct mcpr_slot slot_data;
                } set_slot;

                struct
                {
                    int32_t item_id;
                    int32_t cooldown_ticks;
                } set_cooldown;

                struct
                {
                    char *channel;
                    size_t data_length;
                    void *data;
                } plugin_message;

                struct
                {
                    enum mcpr_sound sound;
                    enum mcpr_sound_category sound_category;
                    int32_t effect_position_x, effect_position_y, effect_position_z;
                    float volume;
                    float pitch;
                } named_sound_effect;

                struct
                {
                    char *reason;
                } disconnect;

                struct
                {
                    int32_t entity_id;
                    int8_t entity_status;
                } entity_status;

                struct
                {
                    float x, y, z;
                    float radius;
                    int32_t record_count;
                    struct mcpr_explosion_record *records;
                    float player_motion_x, player_motion_y, player_motion_z;
                } explosion;

                struct
                {
                    int32_t chunk_x, chunk_z;
                } unload_chunk;

                struct
                {
                    enum mcpr_change_game_state_effect reason;
                    float value;
                } change_game_state;

                struct
                {
                    int32_t keep_alive_id;
                } keep_alive;

                struct
                {
                    int32_t chunk_x, chunk_z;
                    bool ground_up_continuous;
                    int32_t primary_bit_mask;
                    int32_t size;
                    // TODO
                } chunk_data;

                struct
                {
                    enum mcpr_effect effect;
                    struct mcpr_position location;
                    int32_t data;
                    bool disable_relative_volume;
                } effect;

                struct
                {
                    enum mcpr_particle particle;
                    bool long_distance;
                    float x, y, z;
                    float offset_x, offset_y, offset_z;
                    float particle_data;
                    int32_t particle_count;
                    int32_t *data; // array of int32_t's
                } particle;

                struct
                {
                    int32_t entity_id;
                    enum mcpr_gamemode gamemode;
                    bool hardcore;
                    enum mcpr_dimension dimension;
                    enum mcpr_difficulty difficulty;
                    uint8_t max_players;
                    enum mcpr_level level_type;
                    bool reduced_debug_info;
                } join_game;

                struct
                {
                    int32_t item_damage;
                    int8_t scale;
                    bool tracking_position;
                    int32_t icon_count;
                    struct mcpr_map_icon *icons;
                    int8_t columns;
                    int8_t rows;    // Optional, only if columns is more than 0
                    int8_t x;       // Optional, only if columns is more than 0
                    int8_t z;       // Optional, only if columns is more than 0
                    int32_t length; // Optional, only if columns is more than 0
                    void *data;     // Optional, only if columns is more than 0
                } map;

                struct
                {
                    int32_t entity_id;
                    int16_t delta_x;
                    int16_t delta_y;
                    int16_t delta_z;
                    bool on_ground;
                } entity_relative_move;

                struct
                {
                    int32_t entity_id;
                    int16_t delta_x;
                    int16_t delta_y;
                    int16_t delta_z;
                    int8_t yaw;
                    int8_t pitch;
                    bool on_ground;
                } entity_look_and_relative_move;

                struct
                {
                    int32_t entity_id;
                    int8_t yaw;
                    int8_t pitch;
                    bool on_ground;
                } entity_look;


                struct // http://wiki.vg/Protocol#Entity
                {
                    int32_t entity_id;
                } entity;

                struct
                {
                    double x, y, z;
                    float yaw;
                    float pitch;
                } vehicle_move;

                struct
                {
                    struct mcpr_position location;
                } open_sign_editor;

                struct
                {
                    bool invulnerable;
                    bool flying;
                    bool allow_flying;
                    bool creative_mode;
                    float flying_speed;
                    float field_of_view_modifier;
                } player_abilities;

                struct
                {
                    enum mcpr_combat_event event;
                    union
                    {
                        struct
                        {
                            int32_t duration;
                            int32_t entity_id;
                        } event_end_combat;

                        struct
                        {
                            int32_t player_id;
                            int32_t entity_id;
                            char message;
                        } event_entity_dead;
                    };
                } combat_event;

                struct
                {
                    enum mcpr_player_list_item_action action;
                    int32_t number_of_players;
                    struct mcpr_player_list_item_player *players;
                } player_list_item;

                struct
                {
                    double x, y, z;
                    float yaw, pitch;
                    bool x_is_relative;
                    bool y_is_relative;
                    bool z_is_relative;
                    bool yaw_is_relative;
                    bool pitch_is_relative;
                } player_position_and_look;

                struct
                {
                    int32_t entity_id;
                    struct mcpr_position location;
                } use_bed;

                struct
                {
                    int32_t count;
                    int32_t *entity_ids; // Array of entity IDs, with count elements.
                } destroy_entities;

                struct
                {
                    int32_t entity_id;
                    enum mcpr_potion_effect effect;
                } remove_entity_effect;

                struct
                {
                    char *url;
                    char *hash;
                } resource_pack_send;

                struct
                {
                    enum mcpr_dimension dimension;
                    enum mcpr_difficulty difficulty;
                    enum mcpr_gamemode gamemode;
                    enum mcpr_level level_type;
                } respawn;

                struct
                {
                    int32_t entity_id;
                    int8_t head_yaw;
                } entity_head_look;

                struct
                {
                    enum mcpr_world_border_action action;

                    union {
                        struct
                        {
                            double diameter;
                        } action_set_size;

                        struct
                        {
                            double old_diameter;
                            double new_diameter;
                            int64_t speed;
                        } action_lerp_size;

                        struct
                        {
                            double x, z;
                        } action_set_center;

                        struct
                        {
                            double x, z;
                            double old_diameter;
                            double new_diameter;
                            int64_t speed;
                            int32_t portal_teleport_boundary;
                            int32_t warning_time;
                            int32_t warning_blocks;
                        } action_initialize;

                        struct
                        {
                            int32_t warning_time;
                        } action_set_warning_time;

                        struct
                        {
                            int32_t warning_blocks;
                        } action_set_warning_blocks;
                    };
                } world_border;

                struct
                {
                    int32_t camera_id;
                } camera;

                struct
                {
                    int8_t slot;
                } held_item_change;

                struct
                {
                    enum mcpr_scoreboard_position position;
                    char *score_name;
                } display_scoreboard;

                struct
                {
                    int32_t entity_id;
                    struct mcpr_entity_metadata metadata;
                } entity_metadata;

                struct
                {
                    int32_t attached_entity_id;
                    int32_t holding_entity_id;
                } attach_entity;

                struct
                {
                    int32_t entity_id;
                    int16_t velocity_x, velocity_y, velocity_z;
                } entity_velocity;

                struct
                {
                    int32_t entity_id;
                    enum mcpr_equipment_slot slot;
                    struct mcpr_slot slot;
                } entity_equipment;

                struct
                {
                    float experience_bar;
                    int32_t level;
                    int32_t total_experience;
                } set_experience;

                struct
                {
                    float health;
                    int32_t food;
                    float food_saturation;
                } update_health;

                struct
                {
                    char *objective_name;
                    enum mcpr_scoreboard_objective_mode mode;
                    char *objective_value; // Optional, only if mode is MCPR_SCOREBOARD_OBJECTIVE_MODE_CREATE or MCPR_SCOREBOARD_OBJECTIVE_MODE_MCPR_SCOREBOARD_OBJECTIVE_MODE_UPDATE.
                    char *type; // Optional, only if mode is MCPR_SCOREBOARD_OBJECTIVE_MODE_CREATE or MCPR_SCOREBOARD_OBJECTIVE_MODE_MCPR_SCOREBOARD_OBJECTIVE_MODE_UPDATE.
                } scoreboard_objective;

                struct
                {
                    int32_t entity_id;
                    int32_t passenger_count;
                    int32_t *passengers;
                } set_passengers;

                struct
                {
                    char *team_name;
                    enum mcpr_teams_action action;

                    union
                    {
                        struct
                        {
                            char *team_display_name, *team_prefix, *team_suffix;
                            bool allow_friendly_fire;
                            bool can_see_teamed_invisibles;
                            char *name_tag_visibility;
                            char *collision_rule;
                            int8_t color;
                            int32_t player_count;
                            char **players; // array of strings
                        } action_create

                        struct
                        {
                            char *team_display_name, *team_prefix, *team_suffix;
                            bool allow_friendly_fire;
                            bool can_see_teamed_invisibles;
                            char *name_tag_visibility;
                            char *collision_rule;
                            int8_t color;
                        } action_update_info;

                        struct
                        {
                            int32_t player_count;
                            char **players;
                        } action_add_players;

                        struct
                        {
                            int32_t player_count;
                            char **players;
                        } action_remove_players;
                    };
                } teams;

                struct
                {
                    char *score_name;
                    enum mcpr_update_score_action action;
                    char *objective_name;
                    int32_t value; // Optional, only when action is not MCPR_UPDATE_SCORE_ACTION_REMOVE
                } update_score;

                struct
                {
                    struct mcpr_position location;
                } spawn_position;

                struct
                {
                    int64_t world_age;
                    int64_t time_of_day;
                } time_update;

                struct
                {
                    enum mcpr_title_action action;

                    union
                    {
                        struct
                        {
                            char *title_text;
                        } action_set_title;

                        struct
                        {
                            char *subtitle_text;
                        } action_set_subtitle;

                        struct
                        {
                            char *action_bar_text;
                        } action_set_action_bar;

                        struct
                        {
                            int32_t fade_in;
                            int32_t stay;
                            int32_t fade_out;
                        } action_set_times_and_display;
                    };
                } title;

                struct
                {
                    int32_t sound_id; // TODO enum
                    enum mcpr_sound_category category;
                    int32_t effect_position_x, effect_position_y, effect_position_z;
                    float volume, pitch;
                } sound_effect;

                struct
                {
                    char *header, *footer; // Or NULL to empty.
                } player_list_header_and_footer;

                struct
                {
                    int32_t collected_entity_id;
                    int32_t collector_entity_id;
                    int32_t pickup_item_count;
                } collect_item;

                struct
                {
                    int32_t entity_id;
                    double x, y, z;
                    int8_t angle, yaw;
                    bool on_ground;
                } entity_teleport;

                struct
                {
                    int32_t entity_id;
                    int32_t number_of_properties;
                    struct mcpr_entity_property *properties;
                } entity_properties;

                struct
                {
                    int32_t entity_id;
                    enum mcpr_potion_effect effect;
                    int8_t amplifier;
                    int32_t duration;
                    bool is_ambient;
                    bool show_particles;
                } entity_effect;
            } clientbound;
        } play;
    } data;
};

struct mcpr_abstract_packet *mcpr_read_abstract_packet(FILE *in, bool use_compression, bool use_encryption, size_t encryption_block_size, EVP_CIPHER_CTX *ctx_decrypt);
void mcpr_free_abstract_packet(struct mcpr_abstract_packet);
ssize_t mcpr_write_abstract_packet(FILE *out, struct mcpr_abstract_packet *pkt, bool use_compression, bool force_no_compression, bool use_encryption, size_t encryption_block_size, EVP_CIPHER_CTX *ctx_encrypt);

#endif // MCPR_PACKET_H

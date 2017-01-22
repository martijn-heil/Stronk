#ifndef MCPR_PACKET_H
#define MCPR_PACKET_H

#include <stdbool.h>
#include <stdint.h>

#include <jansson/jansson.h>

#include "mcpr.h"

#define PACKET_HANDSHAKE_SERVERBOUND_HANDSHAKE 0x00;

enum mcpr_level
{
    MCPR_LEVEL_DEFAULT,
    MCPR_LEVEL_FLAT,
    MCPR_LEVEL_LARGE_BIOMES,
    MCPR_LEVEL_AMPLIFIED,
    MCPR_LEVEL_DEFAULT_1_1
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
    int8_t x;
    int8_t y;
    int8_t z;
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
    int8_t x;
    int8_t z;
};

enum mcpr_combat_event
{
    MCPR_COMBAT_EVENT_ENTER_COMBAT,
    MCPR_COMBAT_EVENT_END_COMBAT,
    MCPR_COMBAT_EVENT_ENTITY_DEAD
};

struct mcpr_packet
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
                    json_t json_response;
                } response;
            } clientbound;
        } status;

        struct { // play
            union // play - serverbound
            {

            } serverbound;

            union // play - clientbound
            {
                struct
                {
                    int32_t entity_id;
                    struct mcpr_uuid uuid;
                    enum mcpr_object type;
                    double x;
                    double y;
                    double z;
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
                    double x;
                    double y;
                    double z;
                    int16_t count;
                } spawn_experience_orb;

                struct
                {
                    int32_t entity_id;
                    int8_t type;
                    double x;
                    double y;
                    double z;
                } spawn_global_entity;

                struct
                {
                    int32_t entity_id;
                    struct mcpr_uuid entity_uuid;
                    enum mcpr_mob type;
                    double x;
                    double y;
                    double z;
                    int8_t yaw;
                    int8_t pitch;
                    int8_t head_pitch;
                    int16_t velocity_x;
                    int16_t velocity_y;
                    int16_t velocity_z;
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
                    double x;
                    double y;
                    double z;
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
                    nbt_t nbt;
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
                            json_t title;
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
                            json_t title;
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
                    json_t json_data;
                    enum mcpr_chat_position;
                } chat_message;

                struct
                {
                    int32_t chunk_x;
                    int32_t chunk_z;
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
                    json_t window_title;
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
                    int32_t effect_position_x;
                    int32_t effect_position_y;
                    int32_t effect_position_z;
                    float volume;
                    float pitch;
                } named_sound_effect;

                struct
                {
                    json_t reason;
                } disconnect;

                struct
                {
                    int32_t entity_id;
                    int8_t entity_status;
                } entity_status;

                struct
                {
                    float x;
                    float y;
                    float z;
                    float radius;
                    int32_t record_count;
                    struct mcpr_explosion_record *records;
                    float player_motion_x;
                    float player_motion_y;
                    float player_motion_z;
                } explosion;

                struct
                {
                    int32_t chunk_x;
                    int32_t chunk_z;
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
                    int32_t chunk_x;
                    int32_t chunk_z;
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
                    float x;
                    float y;
                    float z;
                    float offset_x;
                    float offset_y;
                    float offset_z;
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
                    double x;
                    double y;
                    double z;
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
                    union {
                        struct {
                            int32_t duration;
                            int32_t entity_id;
                        } event_end_combat;

                        struct {
                            int32_t player_id;
                            int32_t entity_id;
                            json_t message;
                        } event_entity_dead;
                    };

                } combat_event;
            } clientbound;
        } play;
    } data;
};

#endif // MCPR_PACKET_H

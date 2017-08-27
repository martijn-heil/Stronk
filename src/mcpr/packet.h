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

#ifndef MCPR_PACKET_H
#define MCPR_PACKET_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <openssl/evp.h>

#include <ninuuid/ninuuid.h>
#include <nbt/nbt.h>

#include "mcpr/mcpr.h"
#include "warnings.h"

IGNORE("-Wpedantic")

enum mcpr_packet_type
{
    MCPR_PKT_HS_SB_HANDSHAKE,


    MCPR_PKT_ST_CB_RESPONSE,
    MCPR_PKT_ST_CB_PONG,

    MCPR_PKT_ST_SB_REQUEST,
    MCPR_PKT_ST_SB_PING,


    MCPR_PKT_LG_CB_DISCONNECT,
    MCPR_PKT_LG_CB_ENCRYPTION_REQUEST,
    MCPR_PKT_LG_CB_LOGIN_SUCCESS,
    MCPR_PKT_LG_CB_SET_COMPRESSION,

    MCPR_PKT_LG_SB_LOGIN_START,
    MCPR_PKT_LG_SB_ENCRYPTION_RESPONSE,


    MCPR_PKT_PL_CB_SPAWN_OBJECT,
    MCPR_PKT_PL_CB_SPAWN_EXPERIENCE_ORB,
    MCPR_PKT_PL_CB_SPAWN_GLOBAL_ENTITY,
    MCPR_PKT_PL_CB_SPAWN_MOB,
    MCPR_PKT_PL_CB_SPAWN_PAINTING,
    MCPR_PKT_PL_CB_SPAWN_PLAYER,
    MCPR_PKT_PL_CB_ANIMATION,
    MCPR_PKT_PL_CB_STATISTICS,
    MCPR_PKT_PL_CB_BLOCK_BREAK_ANIMATION,
    MCPR_PKT_PL_CB_UPDATE_BLOCK_ENTITY,
    MCPR_PKT_PL_CB_BLOCK_ACTION,
    MCPR_PKT_PL_CB_BLOCK_CHANGE,
    MCPR_PKT_PL_CB_BOSS_BAR,
    MCPR_PKT_PL_CB_SERVER_DIFFICULTY,
    MCPR_PKT_PL_CB_TAB_COMPLETE,
    MCPR_PKT_PL_CB_CHAT_MESSAGE,
    MCPR_PKT_PL_CB_MULTI_BLOCK_CHANGE,
    MCPR_PKT_PL_CB_CONFIRM_TRANSACTION,
    MCPR_PKT_PL_CB_CLOSE_WINDOW,
    MCPR_PKT_PL_CB_OPEN_WINDOW,
    MCPR_PKT_PL_CB_WINDOW_ITEMS,
    MCPR_PKT_PL_CB_WINDOW_PROPERTY,
    MCPR_PKT_PL_CB_SET_SLOT,
    MCPR_PKT_PL_CB_SET_COOLDOWN,
    MCPR_PKT_PL_CB_PLUGIN_MESSAGE,
    MCPR_PKT_PL_CB_NAMED_SOUND_EFFECT,
    MCPR_PKT_PL_CB_DISCONNECT,
    MCPR_PKT_PL_CB_ENTITY_STATUS,
    MCPR_PKT_PL_CB_EXPLOSION,
    MCPR_PKT_PL_CB_UNLOAD_CHUNK,
    MCPR_PKT_PL_CB_CHANGE_GAME_STATE,
    MCPR_PKT_PL_CB_KEEP_ALIVE,
    MCPR_PKT_PL_CB_CHUNK_DATA,
    MCPR_PKT_PL_CB_EFFECT,
    MCPR_PKT_PL_CB_PARTICLE,
    MCPR_PKT_PL_CB_JOIN_GAME,
    MCPR_PKT_PL_CB_MAP,
    MCPR_PKT_PL_CB_ENTITY_RELATIVE_MOVE,
    MCPR_PKT_PL_CB_ENTITY_LOOK_AND_RELATIVE_MOVE,
    MCPR_PKT_PL_CB_ENTITY_LOOK,
    MCPR_PKT_PL_CB_ENTITY,
    MCPR_PKT_PL_CB_VEHICLE_MOVE,
    MCPR_PKT_PL_CB_OPEN_SIGN_EDITOR,
    MCPR_PKT_PL_CB_PLAYER_ABILITIES,
    MCPR_PKT_PL_CB_COMBAT_EVENT,
    MCPR_PKT_PL_CB_PLAYER_LIST_ITEM,
    MCPR_PKT_PL_CB_PLAYER_POSITION_AND_LOOK,
    MCPR_PKT_PL_CB_USE_BED,
    MCPR_PKT_PL_CB_DESTROY_ENTITIES,
    MCPR_PKT_PL_CB_REMOVE_ENTITY_EFFECT,
    MCPR_PKT_PL_CB_RESOURCE_PACK_SEND,
    MCPR_PKT_PL_CB_RESPAWN,
    MCPR_PKT_PL_CB_ENTITY_HEAD_LOOK,
    MCPR_PKT_PL_CB_WORLD_BORDER,
    MCPR_PKT_PL_CB_CAMERA,
    MCPR_PKT_PL_CB_HELD_ITEM_CHANGE,
    MCPR_PKT_PL_CB_DISPLAY_SCOREBOARD,
    MCPR_PKT_PL_CB_ENTITY_METADATA,
    MCPR_PKT_PL_CB_ATTACH_ENTITY,
    MCPR_PKT_PL_CB_ENTITY_VELOCITY,
    MCPR_PKT_PL_CB_ENTITY_EQUIPMENT,
    MCPR_PKT_PL_CB_SET_EXPERIENCE,
    MCPR_PKT_PL_CB_UPDATE_HEALTH,
    MCPR_PKT_PL_CB_SCOREBOARD_OBJECTIVE,
    MCPR_PKT_PL_CB_SET_PASSENGERS,
    MCPR_PKT_PL_CB_TEAMS,
    MCPR_PKT_PL_CB_UPDATE_SCORE,
    MCPR_PKT_PL_CB_SPAWN_POSITION,
    MCPR_PKT_PL_CB_TIME_UPDATE,
    MCPR_PKT_PL_CB_TITLE,
    MCPR_PKT_PL_CB_SOUND_EFFECT,
    MCPR_PKT_PL_CB_PLAYER_LIST_HEADER_AND_FOOTER,
    MCPR_PKT_PL_CB_COLLECT_ITEM,
    MCPR_PKT_PL_CB_ENTITY_TELEPORT,
    MCPR_PKT_PL_CB_ENTITY_PROPERTIES,
    MCPR_PKT_PL_CB_ENTITY_EFFECT,

    MCPR_PKT_PL_SB_TELEPORT_CONFIRM,
    MCPR_PKT_PL_SB_TAB_COMPLETE,
    MCPR_PKT_PL_SB_CHAT_MESSAGE,
    MCPR_PKT_PL_SB_CLIENT_STATUS,
    MCPR_PKT_PL_SB_CLIENT_SETTINGS,
    MCPR_PKT_PL_SB_CONFIRM_TRANSACTION,
    MCPR_PKT_PL_SB_ENCHANT_ITEM,
    MCPR_PKT_PL_SB_CLICK_WINDOW,
    MCPR_PKT_PL_SB_CLOSE_WINDOW,
    MCPR_PKT_PL_SB_PLUGIN_MESSAGE,
    MCPR_PKT_PL_SB_USE_ENTITY,
    MCPR_PKT_PL_SB_KEEP_ALIVE,
    MCPR_PKT_PL_SB_PLAYER_POSITION,
    MCPR_PKT_PL_SB_PLAYER_POSITION_AND_LOOK,
    MCPR_PKT_PL_SB_PLAYER_LOOK,
    MCPR_PKT_PL_SB_PLAYER,
    MCPR_PKT_PL_SB_VEHICLE_MOVE,
    MCPR_PKT_PL_SB_STEER_BOAT,
    MCPR_PKT_PL_SB_PLAYER_ABILITIES,
    MCPR_PKT_PL_SB_PLAYER_DIGGING,
    MCPR_PKT_PL_SB_ENTITY_ACTION,
    MCPR_PKT_PL_SB_STEER_VEHICLE,
    MCPR_PKT_PL_SB_RESOURCE_PACK_STATUS,
    MCPR_PKT_PL_SB_HELD_ITEM_CHANGE,
    MCPR_PKT_PL_SB_CREATIVE_INVENTORY_ACTION,
    MCPR_PKT_PL_SB_UPDATE_SIGN,
    MCPR_PKT_PL_SB_ANIMATION,
    MCPR_PKT_PL_SB_SPECTATE,
    MCPR_PKT_PL_SB_PLAYER_BLOCK_PLACEMENT,
    MCPR_PKT_PL_SB_USE_ITEM,
    MCPR_PKT_PL_SB_UNLOCK_RECIPES,
    MCPR_PKT_PL_SB_ADVANCEMENT_PROGRESS,
    MCPR_PKT_PL_SB_ADVANCEMENTS,
    MCPR_PKT_PL_SB_CRAFT_RECIPE_REQUEST,
    MCPR_PKT_PL_SB_CRAFTING_BOOK_DATA,
    MCPR_PKT_PL_SB_ADVANCEMENT_TAB,
};
bool mcpr_get_packet_type(enum mcpr_packet_type *out, uint8_t id, enum mcpr_state state);
uint8_t mcpr_packet_type_to_byte(enum mcpr_packet_type id);

enum mcpr_painting
{
    MCPR_PAINTING_ALBAN,
    MCPR_PAINTING_AZTEC,
    MCPR_PAINTING_AZTEC2,
    MCPR_PAINTING_BOMB,
    MCPR_PAINTING_BURNING_SKULL,
    MCPR_PAINTING_BUST,
    MCPR_PAINTING_COURBET,
    MCPR_PAINTING_CREEBET,
    MCPR_PAINTING_DONKEYKONG,
    MCPR_PAINTING_FIGHTERS,
    MCPR_PAINTING_GRAHAM,
    MCPR_PAINTING_KEBAB,
    MCPR_PAINTING_MATCH,
    MCPR_PAINTING_PIG_SCENE,
    MCPR_PAINTING_PLANT,
    MCPR_PAINTING_POINTER,
    MCPR_PAINTING_POOL,
    MCPR_PAINTING_SEA,
    MCPR_PAINTING_SKELETON,
    MCPR_PAINTING_SKULL_AND_ROSES,
    MCPR_PAINTING_STAGE,
    MCPR_PAINTING_SUNSET,
    MCPR_PAINTING_VOID,
    MCPR_PAINTING_WANDERER,
    MCPR_PAINTING_WASTELAND,
    MCPR_PAINTING_WITHER,
};

enum mcpr_equipment_slot
{
    MCPR_EQUIP_MENT_SLOT_HEAD,
    MCPR_EQUIP_MENT_SLOT_CHEST,
    MCPR_EQUIP_MENT_SLOT_LEGS,
    MCPR_EQUIP_MENT_SLOT_FEET,
    MCPR_EQUIP_MENT_SLOT_OFFHAND,
};

enum mcpr_select_advancement_tab_id
{
    MCPR_SELECT_ADVANCEMENT_TAB_ID_STORY_ROOT,
    MCPR_SELECT_ADVANCEMENT_TAB_ID_NETHER_ROOT,
    MCPR_SELECT_ADVANCEMENT_TAB_ID_END_ROOT,
    MCPR_SELECT_ADVANCEMENT_TAB_ID_ADVENTURE_ROOT,
    MCPR_SELECT_ADVANCEMENT_TAB_ID_HUSBANDRY_ROOT,
};

enum mcpr_potion_effect
{
    // TODO
    MCPR_POTION_EFFECT_
};

enum mcpr_creeper_state
{
    MCPR_CREEPER_STATE_IDLE,
    MCPR_CREEPER_STATE_FUSE,
};

enum mcpr_evocation_evoker_spell
{
    MCPR_EVOCATION_EVOKER_SPELL_FANGS,
    MCPR_EVOCATION_EVOKER_SPELL_NONE,
    MCPR_EVOCATION_EVOKER_SPELL_SUMMON,
    MCPR_EVOCATION_EVOKER_SPELL_WOLOLO,
};

enum mcpr_boss_bar_division
{
    // TODO
    MCPR_BOSS_BAR_DIVISION_
};

enum mcpr_update_block_entity_action
{
    MCPR_UPDATE_BLOCK_ENTITY_ACTION_SET_MOB_SPAWNER_DATA
};

enum mcpr_ocelot_variant
{
    MCPR_OCELOT_VARIANT_BLACK_CAT,
    MCPR_OCELOT_VARIANT_RED_CAT,
    MCPR_OCELOT_VARIANT_SIAMESE_CAT,
    MCPR_OCELOT_VARIANT_WILD_OCELOT
};

enum mcpr_dragon_phase
{
    MCPR_DRAGON_PHASE_BREATH_ATTACK,
    MCPR_DRAGON_PHASE_CHARGE_PLAYER,
    MCPR_DRAGON_PHASE_CIRCLING,
    MCPR_DRAGON_PHASE_DYING,
    MCPR_DRAGON_PHASE_FLY_TO_PORTAL,
    MCPR_DRAGON_PHASE_HOVER,
    MCPR_DRAGON_PHASE_LAND_ON_PORTAL,
    MCPR_DRAGON_PHASE_LEAVE_PORTAL,
    MCPR_DRAGON_PHASE_ROAR_BEFORE_ATTACK,
    MCPR_DRAGON_PHASE_SEARCH_BREATH_ATTACK_TARGET,
    MCPR_DRAGON_PHASE_STRAFING,
};

enum mcpr_rabbit_variant
{
    MCPR_RABBIT_VARIANT_BLACK,
    MCPR_RABBIT_VARIANT_BLACK_AND_WHITE,
    MCPR_RABBIT_VARIANT_BROWN,
    MCPR_RABBIT_VARIANT_GOLD,
    MCPR_RABBIT_VARIANT_SALT_AND_PEPPER, // Salt and pepper colored, whatever that means.
    MCPR_RABBIT_VARIANT_KILLER_BUNNY,
    MCPR_RABBIT_VARIANT_WHITE,
};

enum mcpr_dye_color
{
    MCPR_DYE_COLOR_RED,
    MCPR_DYE_COLOR_ORANGE,
    MCPR_DYE_COLOR_YELLOW,
    MCPR_DYE_COLOR_GREEN,
    MCPR_DYE_COLOR_BLUE,
    MCPR_DYE_COLOR_LIGHT_BLUE,
    MCPR_DYE_COLOR_MAGENTA,
    MCPR_DYE_COLOR_PINK,
    MCPR_DYE_COLOR_WHITE,
    MCPR_DYE_COLOR_LIGHT_GRAY,
    MCPR_DYE_COLOR_BLACK,
    MCPR_DYE_COLOR_BROWN
};

enum mcpr_mob
{
    MCPR_MOB_ELDER_GUARDIAN,
    MCPR_MOB_WITHER_SKELETON,
    MCPR_MOB_STRAY,
    MCPR_MOB_HUSK,
    MCPR_MOB_ZOMBIE_VILLAGER,
    MCPR_MOB_SKELETON_HORSE,
    MCPR_MOB_ZOMBIE_HORSE,
    MCPR_MOB_ARMOR_STAND,
    MCPR_MOB_DONKEY,
    MCPR_MOB_MULE,
    MCPR_MOB_EVOCATION_ILLAGER,
    MCPR_MOB_VEX,
    MCPR_MOB_VINDICATION_ILLAGER,
    MCPR_MOB_CREEPER,
    MCPR_MOB_SKELETON,
    MCPR_MOB_SPIDER,
    MCPR_MOB_GIANT,
    MCPR_MOB_ZOMBIE,
    MCPR_MOB_SLIME,
    MCPR_MOB_GHAST,
    MCPR_MOB_PIG_ZOMBIE,
    MCPR_MOB_ENDERMAN,
    MCPR_MOB_CAVE_SPIDER,
    MCPR_MOB_SILVERFISH,
    MCPR_MOB_BLAZE,
    MCPR_MOB_LAVA_SLIME,
    MCPR_MOB_ENDER_DRAGON,
    MCPR_MOB_WITHER_BOSS,
    MCPR_MOB_BAT,
    MCPR_MOB_WITCH,
    MCPR_MOB_ENDERMITE,
    MCPR_MOB_GUARDIAN,
    MCPR_MOB_SHULKER,
    MCPR_MOB_PIG,
    MCPR_MOB_SHEEP,
    MCPR_MOB_COW,
    MCPR_MOB_CHICKEN,
    MCPR_MOB_SQUID,
    MCPR_MOB_WOLF,
    MCPR_MOB_MUSHROOM_COW,
    MCPR_MOB_SNOW_MAN,
    MCPR_MOB_OCELOT,
    MCPR_MOB_VILLAGER_GOLEM,
    MCPR_MOB_HORSE,
    MCPR_MOB_RABBIT,
    MCPR_MOB_POLAR_BEAR,
    MCPR_MOB_LLAMA,
    MCPR_MOB_VILLAGER
};

enum mcpr_object
{
    MCPR_OBJECT_BOAT,
    MCPR_OBJECT_ITEM_STACK,
    MCPR_OBJECT_AREA_EFFECT_CLOUD,
    MCPR_OBJECT_MINECART,
    MCPR_OBJECT_ACTIVATED_TNT,
    MCPR_OBJECT_ENDER_CRYSTAL,
    MCPR_OBJECT_TIPPED_ARROW,
    MCPR_OBJECT_SNOWBALL,
    MCPR_OBJECT_EGG,
    MCPR_OBJECT_FIRE_BALL,
    MCPR_OBJECT_FIRE_CHARGE,
    MCPR_OBJECT_THROWN_ENDERPEARL,
    MCPR_OBJECT_WITHER_SKULL,
    MCPR_OBJECT_SHULKER_BULLET,
    MCPR_OBJECT_LLAMA_SPIT,
    MCPR_OBJECT_FALLING_OBJECT,
    MCPR_OBJECT_ITEM_FRAME,
    MCPR_OBJECT_EYE_OF_ENDER,
    MCPR_OBJECT_THROWN_POTION,
    MCPR_OBJECT_THROWN_EXP_BOTTLE,
    MCPR_OBJECT_FIREWORK_ROCKET,
    MCPR_OBJECT_LEASH_KNOT,
    MCPR_OBJECT_ARMOR_STAND,
    MCPR_OBJECT_EVOCATION_FANG,
    MCPR_OBJECT_FISHING_HOOK,
    MCPR_OBJECT_SPECTRAL_ARROW,
    MCPR_OBJECT_DRAGON_FIREBALL
};

enum mcpr_boss_bar_color
{
    MCPR_BOSS_BAR_COLOR_PINK,
    MCPR_BOSS_BAR_COLOR_BLUE,
    MCPR_BOSS_BAR_COLOR_RED,
    MCPR_BOSS_BAR_COLOR_GREEN,
    MCPR_BOSS_BAR_COLOR_YELLOW,
    MCPR_BOSS_BAR_COLOR_PURPLE,
    MCPR_BOSS_BAR_COLOR_WHITE,
};

enum mcpr_game_state_effect
{
    MCPR_GAME_STATE_EFFECT_INVALID_BED,
    MCPR_GAME_STATE_EFFECT_STOP_RAIN,
    MCPR_GAME_STATE_EFFECT_START_RAIN,
    MCPR_GAME_STATE_EFFECT_CHANGE_GAME_MODE,
    MCPR_GAME_STATE_EFFECT_EXIT_END,
    MCPR_GAME_STATE_EFFECT_DEMO_MESSAGE,
    MCPR_GAME_STATE_EFFECT_ARROW_HIT_PLAYER,
    MCPR_GAME_STATE_EFFECT_FADE_VALUE,
    MCPR_GAME_STATE_EFFECT_FADE_TIME,
    MCPR_GAME_STATE_EFFECT_ELDER_GUARDIAN_APPEARANCE,
};

enum mcpr_use_entity_type
{
    MCPR_USE_ENTITY_TYPE_INTERACT,
    MCPR_USE_ENTITY_TYPE_INTERACT_AT,
    MCPR_USE_ENTITY_TYPE_ATTACK
};

enum mcpr_entity_property
{
    MCPR_ENTITY_PROPERTY_GENERIC_MAX_HEALTH,
    MCPR_ENTITY_PROPERTY_GENERIC_FOLLOW_RANGE,
    MCPR_ENTITY_PROPERTY_GENERIC_KNOCKBACK_RESISTANCE,
    MCPR_ENTITY_PROPERTY_GENERIC_MOVEMENT_SPEED,
    MCPR_ENTITY_PROPERTY_GENERIC_ATTACK_DAMAGE,
    MCPR_ENTITY_PROPERTY_GENERIC_ATTACK_SPEED,
    MCPR_ENTITY_PROPERTY_GENERIC_FLYING_SPEED,
    MCPR_ENTITY_PROPERTY_HORSE_JUMP_SPEED,
    MCPR_ENTITY_PROPERTY_ZOMBIE_SPAWN_REINFORCEMENTS_CHANCE,
};

enum mcpr_animation
{
    MCPR_ANIMATION_SWING_MAIN_ARM,
    MCPR_ANIMATION_TAKE_DAMAGE,
    MCPR_ANIMATION_LEAVE_BED,
    MCPR_ANIMATION_SWING_OFFHAND,
    MCPR_ANIMATION_CRITICAL_EFFECT,
    MCPR_ANIMATION_MAGIC_CRITICAL_EFFECT
};

enum mcpr_dimension
{
    MCPR_DIMENSION_OVERWORLD,
    MCPR_DIMENSION_NETHER,
    MCPR_DIMENSION_END
};

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

enum mcpr_player_dig_status
{
    MCPR_PLAYER_DIG_STATUS_STARTED,
    MCPR_PLAYER_DIG_STATUS_CANCELLED,
    MCPR_PLAYER_DIG_STATUS_FINISHED,
    MCPR_PLAYER_DIG_STATUS_DROP_ITEM_STACK,
    MCPR_PLAYER_DIG_STATUS_DROP_ITEM,
    MCPR_PLAYER_DIG_STATUS_SHOOT_ARROW_FINISH_EATING,
    MCPR_PLAYER_DIG_STATUS_SWAP_ITEM_IN_HAND
};

struct mcpr_block_action
{
    // TODO
};

enum mcpr_window
{
    MCPR_WINDOW_CONTAINER,
    MCPR_WINDOW_CHEST,
    MCPR_WINDOW_CRAFTING_TABLE,
    MCPR_WINDOW_FURNACE,
    MCPR_WINDOW_DISPENSER,
    MCPR_WINDOW_ENCHANTING_TABLE,
    MCPR_WINDOW_BREWING_STAND,
    MCPR_WINDOW_VILLAGER,
    MCPR_WINDOW_BEACON,
    MCPR_WINDOW_ANVIL,
    MCPR_WINDOW_HOPPER,
    MCPR_WINDOW_DROPPER,
    MCPR_WINDOW_SHULKER_BOX,
    MCPR_WINDOW_HORSE,
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

// We're using the same names as in Bukkit.
// https://hub.spigotmc.org/javadocs/spigot/org/bukkit/entity/Horse.Color.html
enum mcpr_horse_color
{
    MCPR_HORSE_COLOR_WHITE,
    MCPR_HORSE_COLOR_BLACK,
    MCPR_HORSE_COLOR_BROWN,
    MCPR_HORSE_COLOR_CHESTNUT,
    MCPR_HORSE_COLOR_CREAMY,
    MCPR_HORSE_COLOR_DARK_BROWN,
    MCPR_HORSE_COLOR_GRAY
};

// We're using the same names as in Bukkit.
// https://hub.spigotmc.org/javadocs/spigot/org/bukkit/entity/Horse.Style.html
enum mcpr_horse_style
{
    MCPR_HORSE_STYLE_BLACK_DOTS,
    MCPR_HORSE_STYLE_NONE,
    MCPR_HORSE_STYLE_WHITE,
    MCPR_HORSE_STYLE_WHITE_DOTS,
    MCPR_HORSE_STYLE_WHITEFIELD
};

enum mcpr_horse_armor
{
    MCPR_HORSE_ARMOR_DIAMOND,
    MCPR_HORSE_ARMOR_IRON,
    MCPR_HORSE_ARMOR_GOLD,
};

enum mcpr_villager_profession
{
    MCPR_VILLAGER_PROFESSION_FARMER,
    MCPR_VILLAGER_PROFESSION_LIBRARIAN,
    MCPR_VILLAGER_PROFESSION_PRIEST,
    MCPR_VILLAGER_PROFESSION_BLACKSMITH,
    MCPR_VILLAGER_PROFESSION_BUTCHER,
    MCPR_VILLAGER_PROFESSION_NITWIT
};

struct mcpr_player_sample
{
    char *player_name;
    struct ninuuid uuid;
};

struct mcpr_entity_property_modifier
{
    struct ninuuid uuid;
    double amount;
    enum mcpr_entity_property_modifier_operation operation;
};

struct mcpr_entity_property_data
{
    enum mcpr_entity_property key;
    double value;
    int32_t number_of_modifiers;
    struct mcpr_entity_property_modifier *modifiers;
};

struct mcpr_chunk_section
{
    uint8_t bits_per_block;

    int32_t palette_length; // 0 if palette is NULL
    int32_t *palette; // or NULL

    int32_t block_array_length;
    uint64_t *blocks;

    uint8_t *block_light;
    uint8_t *sky_light; // Only in the overworld, else NULL
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

struct mcpr_rotation
{
    // TODO
};

struct mcpr_window_property
{
    enum mcpr_window window_type;
    int16_t property_index;
    union
    {
        struct
        {
            union
            {
                int16_t fuel_left;              // 0
                int16_t max_fuel_burn_time;     // 1
                int16_t progress_arrow;         // 2
                int16_t max_progress;           // 3
            };
        } furnace;

        struct
        {
            union
            {
                int16_t required_level_top;         // 0
                int16_t required_level_middle;      // 1
                int16_t required_level_bottom;      // 2
                int16_t enchantment_seed;           // 3
                int16_t enchantment_id_shown_top;   // 4
                int16_t enchantment_id_shown_mid;   // 5
                int16_t enchantment_id_shown_bottom;// 6
            };
        } enchantment_table;

        struct
        {
            union
            {
                int16_t power_level;                    // 0
                enum mcpr_potion_effect first_effect;   // 1
                enum mcpr_potion_effect second_effect;  // 2
            };
        } beacon;

        struct
        {
            union
            {
                int16_t repair_cost;                    // 0
            };
        } anvil;

        struct
        {
            union
            {
                int16_t brew_time;                      // 0
            };
        } brewing_stand;
    };
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
    struct ninuuid uuid;

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

enum mcpr_llama_variant
{
    MCPR_LLAMA_VARIANT_BROWN,
    MCPR_LLAMA_VARIANT_CREAMY,
    MCPR_LLAMA_VARIANT_GRAY,
    MCPR_LLAMA_VARIANT_WHITE,
};

enum mcpr_boss_bar_action
{
    MCPR_BOSS_BAR_ACTION_ADD,
    MCPR_BOSS_BAR_ACTION_REMOVE,
    MCPR_BOSS_BAR_ACTION_UPDATE_HEALTH,
    MCPR_BOSS_BAR_ACTION_UPDATE_TITLE,
    MCPR_BOSS_BAR_ACTION_UPDATE_STYLE,
    MCPR_BOSS_BAR_ACTION_UPDATE_FLAGS
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

enum mcpr_map_icon_type
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
    enum mcpr_map_icon_type type;
    int8_t x, z;
};

enum mcpr_combat_event
{
    MCPR_COMBAT_EVENT_ENTER_COMBAT,
    MCPR_COMBAT_EVENT_END_COMBAT,
    MCPR_COMBAT_EVENT_ENTITY_DEAD
};

enum unlock_recipes_action
{
    MCPR_UNLOCK_RECIPES_ACTION_INIT,
    MCPR_UNLOCK_RECIPES_ACTION_ADD,
    MCPR_UNLOCK_RECIPES_ACTION_REMOVE,
};

struct mcpr_entity_metadata_entry
{
    int32_t index; // TODO different way to specify what this entry is about.

    union
    {
        struct
        {
            union
            {
                struct
                {
                    bool on_fire;
                    bool crouched;
                    bool sprinting;
                    bool invisible;
                    bool glowing_effect;
                    bool elytra_flying;
                } flags;

                int32_t air;
                char *custom_name;
                bool custom_name_visible;
                bool is_silent;
                bool gravity;

                struct
                {
                    union
                    {
                        struct
                        {
                            void *potion_slot;
                        } potion;
                    };
                } projectile;

                struct
                {
                    struct mcpr_position spawn_position;
                } falling_block;

                struct
                {
                    union
                    {
                        float radius;
                        int32_t color;
                        bool ignore_radius;
                        int32_t particle_id;
                        int32_t particle_parameter_1;
                        int32_t particle_parameter_2;
                    };
                } area_effect_cloud;

                struct
                {
                    int32_t hooked_entity_id_plus_one; // Hooked entity id + 1, or 0 if there is no hooked entity
                } fishing_hook;

                struct
                {
                    union
                    {
                        struct
                        {
                            bool is_critical;
                        } flags;

                        struct
                        {
                            int32_t color;
                        } tipped_arrow;
                    };
                } arrow;

                struct
                {
                    union
                    {
                        int32_t time_since_last_hit;
                        int32_t forward_direction;
                        float damage_taken;
                        int32_t type;
                        bool right_paddle_turning, left_paddle_turning;
                    };
                } boat;

                struct
                {
                    union
                    {
                        struct mcpr_position *beam_target; // or NULL
                        bool show_bottom;
                    };
                } ender_crystal;

                struct
                {
                    union
                    {
                        struct
                        {
                            bool invulnerable;
                        } wither_skull;
                    };
                } fireball;

                struct
                {
                    union
                    {
                        void *firework_info_slot;
                        int32_t entity_id;
                    };
                } fireworks;

                struct
                {
                    union
                    {
                        struct
                        {
                            void *slot_item;
                        } item_frame;
                    };
                } hanging;

                struct
                {
                    union
                    {
                        void *slot_item;
                    };
                } item;

                struct
                {
                    union
                    {
                        struct
                        {
                            bool hand_active;
                            enum mcpr_hand active_hand;
                        } hand_flags;

                        float health;
                        int32_t potion_effect_color;
                        bool potion_is_ambient;
                        int32_t arrows_in_entity;

                        struct
                        {
                            union
                            {
                                float additional_hearts;
                                int32_t score;

                                struct
                                {
                                    bool cape_enabled;
                                    bool jacket_enabled;
                                    bool left_sleeve_enabled, right_sleeve_enabled;
                                    bool left_pants_enabled ,right_pants_enabled;
                                    bool hat_enabled;
                                } flags;

                                enum mcpr_side_based_hand main_hand;
                            };
                        } player;

                        struct
                        {
                            union
                            {
                                struct
                                {
                                    bool is_small;
                                    bool has_arms;
                                    bool no_baseplate;
                                    bool set_marker;
                                } flags;

                                struct mcpr_rotation head_rotation;
                                struct mcpr_rotation body_rotation;
                                struct mcpr_rotation right_arm_rotation, left_arm_rotation;
                                struct mcpr_rotation right_leg_rotation, left_leg_rotation;
                            };
                        } armor_stand;

                        struct
                        {
                            union
                            {
                                struct
                                {
                                    bool no_ai;
                                    bool left_handed;
                                } flags;

                                struct
                                {
                                    union
                                    {
                                        struct
                                        {
                                            struct
                                            {
                                                bool is_hanging;
                                            } flags;
                                        } bat;
                                    };
                                } ambient;

                                struct
                                {
                                    union
                                    {
                                        struct
                                        {

                                        } squid;
                                    };
                                } water_mob;

                                struct
                                {
                                    union
                                    {
                                        struct
                                        {
                                            union
                                            {
                                                bool is_baby;

                                                struct
                                                {
                                                    union
                                                    {
                                                        struct
                                                        {
                                                            union
                                                            {
                                                                struct
                                                                {
                                                                    bool is_tame;
                                                                    bool is_saddled;
                                                                    bool has_chest;
                                                                    bool is_bred;
                                                                    bool is_eating;
                                                                    bool is_rearing;
                                                                    bool is_mouth_open;
                                                                } flags;

                                                                struct ninuuid *owner; // or NULL
                                                            };

                                                            struct
                                                            {
                                                                union
                                                                {
                                                                    enum mcpr_horse_color color;
                                                                    enum mcpr_horse_style style;
                                                                    enum mcpr_horse_armor armor;
                                                                };
                                                            } horse;

                                                            struct
                                                            {
                                                                union
                                                                {
                                                                    bool has_chest;

                                                                    struct
                                                                    {
                                                                        int32_t strength;
                                                                        enum mcpr_dye_color carpet_color;
                                                                        enum mcpr_llama_variant variant;
                                                                    } llama;
                                                                };
                                                            } chested_horse;
                                                        } abstract_horse;

                                                        struct
                                                        {
                                                            union
                                                            {
                                                                bool has_saddle;
                                                                int32_t boost_time;
                                                            };
                                                        } pig;

                                                        struct
                                                        {
                                                            enum mcpr_rabbit_variant variant;
                                                        } rabbit;

                                                        struct
                                                        {
                                                            union
                                                            {
                                                                bool standing_up;
                                                            };
                                                        } polar_bear;

                                                        struct
                                                        {
                                                            union
                                                            {
                                                                struct
                                                                {
                                                                    unsigned char color;
                                                                    bool is_sheared;
                                                                } flags;
                                                            };
                                                        } sheep;

                                                        struct
                                                        {
                                                            union
                                                            {
                                                                struct
                                                                {
                                                                    bool is_sitting;
                                                                    bool is_angry;
                                                                    bool is_tamed;
                                                                } flags;

                                                                struct
                                                                {
                                                                    union
                                                                    {
                                                                        enum mcpr_ocelot_variant variant;
                                                                    };
                                                                } ocelot;

                                                                struct
                                                                {
                                                                    float damage_taken;
                                                                    bool is_begging;
                                                                    enum mcpr_dye_color collar_color;
                                                                } wolf;
                                                            };
                                                        } tameable_animal;


                                                    };
                                                } animal;
                                            };

                                            struct
                                            {
                                                union
                                                {
                                                    enum mcpr_villager_profession profession;
                                                };
                                            } villager;
                                        } ageable;

                                        struct
                                        {
                                            union
                                            {
                                                struct
                                                {
                                                    union
                                                    {
                                                        struct
                                                        {
                                                            bool is_player_made;
                                                        } flags;
                                                    };
                                                } iron_golem;

                                                struct
                                                {
                                                    union
                                                    {
                                                        struct
                                                        {
                                                            bool no_pumpkin_hat;
                                                        } flags;
                                                    };
                                                } snowman;

                                                struct
                                                {
                                                    union
                                                    {
                                                        int8_t direction;
                                                        struct mcpr_position *attachment_position; // or NULL
                                                        int8_t shield_height;
                                                        enum mcpr_dye_color color;
                                                    };
                                                } shulker;
                                            };
                                        } golem;

                                        struct
                                        {
                                            union
                                            {
                                                struct
                                                {
                                                    union
                                                    {
                                                        struct
                                                        {
                                                            bool is_on_fire;
                                                        } flags;
                                                    };
                                                } blaze;

                                                struct
                                                {
                                                    union
                                                    {
                                                        enum mcpr_creeper_state state;
                                                        bool is_charged;
                                                        bool is_ignited;
                                                    };
                                                } creeper;

                                                struct
                                                {
                                                    union
                                                    {
                                                        bool is_retracting_spikes;
                                                        int32_t target_entity_id;
                                                    };
                                                } guardian;

                                                struct
                                                {
                                                    union
                                                    {
                                                        enum mcpr_evocation_evoker_spell spell;
                                                    };
                                                } evocation_illager;

                                                struct
                                                {
                                                    union
                                                    {
                                                        struct
                                                        {
                                                            bool in_attack_mode;
                                                        } flags;
                                                    };
                                                } vex;

                                                struct
                                                {
                                                    union
                                                    {
                                                        struct
                                                        {
                                                            bool has_target;
                                                        } flags;
                                                    };
                                                } vindication_illager;

                                                struct
                                                {
                                                    union
                                                    {
                                                        bool is_swinging_arms;
                                                    };
                                                } abstract_skeleton;

                                                struct
                                                {
                                                    union
                                                    {
                                                        struct
                                                        {
                                                            bool is_climbing;
                                                        } flags;
                                                    };
                                                } spider;

                                                struct
                                                {
                                                    union
                                                    {
                                                        bool is_aggressive;
                                                    };
                                                } witch;

                                                struct
                                                {
                                                    union
                                                    {
                                                        int32_t center_head_target, left_head_target, right_head_target;
                                                        int32_t invulnerable_time;
                                                    };
                                                } wither;

                                                struct
                                                {
                                                    union
                                                    {
                                                        bool is_baby;
                                                        bool are_hands_up;

                                                        struct
                                                        {
                                                            union
                                                            {
                                                                bool is_converting;
                                                                enum mcpr_villager_profession profession;
                                                            };
                                                        } zombie_villager;
                                                    };
                                                } zombie;

                                                struct
                                                {
                                                    union
                                                    {
                                                        int32_t carried_block; // or -1
                                                        bool is_screaming;
                                                    };
                                                } enderman;
                                            };
                                        } monster;
                                    };
                                } creature;

                                struct
                                {
                                    enum mcpr_dragon_phase phase;
                                } ender_dragon;

                                struct
                                {
                                    union
                                    {
                                        struct
                                        {
                                            union
                                            {
                                                bool is_attacking;
                                            };
                                        } ghast;
                                    };
                                } flying;

                                struct
                                {
                                    union
                                    {
                                        int32_t size;
                                    };
                                } slime;
                            };
                        } insentient;
                    };
                } living;

                struct
                {
                    union
                    {
                        int32_t shaking_power;
                        int32_t shaking_direction;
                        float shaking_multiplier;
                        int32_t custom_block_id_and_damage;
                        int32_t custom_block_y_position;
                        bool show_custom_block;

                        struct
                        {
                            union
                            {
                                bool is_powered;
                            };
                        } minecart_furnace;

                        struct
                        {
                            union
                            {
                                char *command;
                                char *chat_last_output;
                            };
                        } minecart_command_block;
                    };
                } minecart;

                struct
                {
                    union
                    {
                        int32_t fuse_time;
                    };
                } tnt_primed;
            };
        } entity;

        struct
        {
            bool is_baby;
        } ageable;
    };
};

struct mcpr_entity_metadata
{
    struct mcpr_entity_metadata_entry *entries;
    size_t entry_count;
};

struct mcpr_packet
{
    enum mcpr_packet_type id;
    enum mcpr_state state;
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
                    char *reason; // Chat
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
                    struct ninuuid uuid;
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
                    char *version_name;
                    int protocol_version;
                    unsigned int max_players;
                    unsigned int online_players;
                    char *description;
                    char *favicon; // may be NULL.

                    size_t online_players_size;
                    struct mcpr_player_sample *player_sample; // or NULL.
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
                    void * clicked_item;
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
                    float field_of_view_modifier;
                } player_abilities;

                struct
                {
                    enum mcpr_player_dig_status status;
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
                    void * clicked_item;
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
                    struct  ninuuid target_player;
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
                    struct ninuuid uuid;
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
                    struct ninuuid entity_uuid;
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
                    struct ninuuid entity_uuid;
                    enum mcpr_painting type;
                    struct mcpr_position position;
                    int8_t direction;
                } spawn_painting;

                struct
                {
                    int32_t entity_id;
                    struct ninuuid player_uuid;
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
                    nbt_node *nbt;
                } update_block_entity;

                struct
                {
                    struct mcpr_position location;
                    struct mcpr_block_action action;
                } block_action;

                struct
                {
                    struct mcpr_position location;
                    int32_t block_id;
                } block_change;

                struct
                {
                    struct ninuuid uuid;
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
                    enum mcpr_chat_position position;
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
                    void * *slots;
                } window_items;

                struct
                {
                    uint8_t window_id;
                    struct mcpr_window_property property;
                    int16_t value;
                } window_property;

                struct
                {
                    uint8_t window_id;
                    int16_t slot;
                    void * slot_data;
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
                    char *sound_id; // TODO enum
                    enum mcpr_sound_category sound_category;
                    int32_t effect_position_x, effect_position_y, effect_position_z;
                    float volume;
                    float pitch;
                } named_sound_effect;

                struct
                {
                    char *reason; // should be JSON chat
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
                    enum mcpr_game_state_effect reason;
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
                    size_t size; // amount of chunk sections
                    struct mcpr_chunk_section *chunk_sections;
                    uint8_t *biomes; // or NULL if ground_up_continuous is false, 256 bytes if present.
                    int32_t block_entity_count; // Should be 0 when block_entities is NULL
                    nbt_node *block_entities; // or NULL
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
                    bool is_flying;
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
                    int32_t teleport_id;
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
                    void *slot_data;
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
                        } action_create;

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
                    struct mcpr_entity_property_data *properties;
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

                struct
                {
                    enum unlock_recipes_action action;
                    bool crafting_book_open;
                    bool filtering_craftable;
                    int32_t array_size_1;
                    int32_t *recipe_ids;

                    int32_t array_size_2; // optional, only present if action is UNLOCK_RECIPES_ACTION_INIT
                    int32_t *recipe_ids_2; // optional, only present if action is UNLOCK_RECIPES_ACTION_INIT
                } unlock_recipes;

                struct
                {
                    bool has_id;
                    enum mcpr_select_advancement_tab_id identifier; // optional, only if has_id is true
                } select_advancement_tab;

                // struct
                // {
                //     bool reset;
                //     int32_t mapping_size;
                //     struct mcpr_advancement_mapping *advancement_mapping;
                //     int32_t list_size;
                //     struct mcpr_identifier *identifiers;
                //     int32_t progress_size;
                //     struct mcpr_progress_mapping *progress_mapping;
                // } advancements;
            } clientbound;
        } play;
    } data;
};
END_IGNORE()

ssize_t mcpr_decode_packet(struct mcpr_packet **out, const void *in, enum mcpr_state state, size_t maxlen);
void mcpr_free_decoded_packet(struct mcpr_packet *pkt);
bool mcpr_encode_packet(void **buf, size_t *out_bytes_written, const struct mcpr_packet *pkt);


#endif // MCPR_PACKET_H

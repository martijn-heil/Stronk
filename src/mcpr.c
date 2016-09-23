#include "mcpr.h"



// ----------------- Handshake state ------------------

//              --- Serverbound ---
const uint8_t MCPR_PKT_HS_SB_HANDSHAKE                     = 0x00;




// ----------------- Status state ------------------

//              --- Clientbound ---
const uint8_t MCPR_PKT_ST_CB_RESPONSE                      = 0x00;
const uint8_t MCPR_PKT_ST_CB_PONG                          = 0x01;


//              --- Serverbound ---
const uint8_t MCPR_PKT_ST_SB_REQUEST                       = 0x00;
const uint8_t MCPR_PKT_ST_SB_PING                          = 0x01;




// ----------------- Login state ------------------

//              --- Clientbound ---
const uint8_t MCPR_PKT_LO_CB_DISCONNECT                    = 0x00;
const uint8_t MCPR_PKT_LO_CB_ENCRYPTION_REQUEST            = 0x01;
const uint8_t MCPR_PKT_LO_CB_LOGIN_SUCCESS                 = 0x02;
const uint8_t MCPR_PKT_LO_CB_SET_COMPRESSION               = 0x03;


//              --- Serverbound ---
const uint8_t MCPR_PKT_LO_SB_LOGIN_START                   = 0x00;
const uint8_t MCPR_PKT_LO_SB_ENCRYPTION_RESPONSE           = 0x01;



// ------------------- Play state ----------------------

//              --- Clientbound ---
const uint8_t MCPR_PKT_PL_CB_SPAWN_OBJECT                  = 0x00;
const uint8_t MCPR_PKT_PL_CB_SPAWN_EXPERIENCE_ORB          = 0x01;
const uint8_t MCPR_PKT_PL_CB_SPAWN_GLOBAL_ENTITY           = 0x02;
const uint8_t MCPR_PKT_PL_CB_SPAWN_MOB                     = 0x03;
const uint8_t MCPR_PKT_PL_CB_SPAWN_PAINTING                = 0x04;
const uint8_t MCPR_PKT_PL_CB_SPAWN_PLAYER                  = 0x05;
const uint8_t MCPR_PKT_PL_CB_ANIMATON                      = 0x06;
const uint8_t MCPR_PKTENUM_PL_CB_ANIMATION_ANIMATION_SWING_MAIN_ARM        = 0x00;
const uint8_t MCPR_PKTENUM_PL_CB_ANIMATION_ANIMATION_TAKE_DAMAGE           = 0x01;
const uint8_t MCPR_PKTENUM_PL_CB_ANIMATION_ANIMATION_LEAVE_BED             = 0x02;
const uint8_t MCPR_PKTENUM_PL_CB_ANIMATION_ANIMATION_SWING_OFFHAND         = 0x03;
const uint8_t MCPR_PKTENUM_PL_CB_ANIMATION_ANIMATION_CRITICAL_EFFECT       = 0x04;
const uint8_t MCPR_PKTENUM_PL_CB_ANIMATION_ANIMATION_MAGIC_CRITICAL_EFFECT = 0x05;

const uint8_t MCPR_PKT_PL_CB_STATISTICS                    = 0x07; // TODO http://wiki.vg/Protocol#Statistics
const uint8_t MCPR_PKT_PL_CB_BLOCK_BREAK_ANIMATON          = 0x08;
const uint8_t MCPR_PKT_PL_CB_UPDATE_BLOCK_ENTITY           = 0x09;
const uint8_t MCPR_PKT_PL_CB_BLOCK_ACTION                  = 0x0A;
const uint8_t MCPR_PKT_PL_CB_BLOCK_CHANGE                  = 0x0B;
const uint8_t MCPR_PKT_PL_CB_BOSS_BAR                      = 0x0C;
const int32_t MCPR_PKTENUM_PL_CB_BOSS_BAR_COLOR_PINK   = 0;
const int32_t MCPR_PKTENUM_PL_CB_BOSS_BAR_COLOR_BLUE   = 1;
const int32_t MCPR_PKTENUM_PL_CB_BOSS_BAR_COLOR_RED    = 2;
const int32_t MCPR_PKTENUM_PL_CB_BOSS_BAR_COLOR_GREEN  = 3;
const int32_t MCPR_PKTENUM_PL_CB_BOSS_BAR_COLOR_YELLOW = 4;
const int32_t MCPR_PKTENUM_PL_CB_BOSS_BAR_COLOR_PURPLE = 5;
const int32_t MCPR_PKTENUM_PL_CB_BOSS_BAR_COLOR_WHITE  = 6;
const int32_t MCPR_PKTENUM_PL_CB_BOSS_BAR_DIVISION_NONE        = 0;
const int32_t MCPR_PKTENUM_PL_CB_BOSS_BAR_DIVISION_6_NOTCHES   = 1;
const int32_t MCPR_PKTENUM_PL_CB_BOSS_BAR_DIVISION_10_NOTCHES  = 2;
const int32_t MCPR_PKTENUM_PL_CB_BOSS_BAR_DIVISION_12_NOTCHES  = 3;
const int32_t MCPR_PKTENUM_PL_CB_BOSS_BAR_DIVISION_20_NOTCHES  = 4;

const uint8_t MCPR_PKT_PL_CB_SERVER_DIFFICULTY             = 0x0D;
const uint8_t MCPR_PKTENUM_PL_CB_SERVER_DIFFICULTY_DIFFICULTY_PEACEFUL = 0;
const uint8_t MCPR_PKTENUM_PL_CB_SERVER_DIFFICULTY_DIFFICULTY_EASY     = 1;
const uint8_t MCPR_PKTENUM_PL_CB_SERVER_DIFFICULTY_DIFFICULTY_NORMAL   = 2;
const uint8_t MCPR_PKTENUM_PL_CB_SERVER_DIFFICULTY_DIFFICULTY_HARD     = 3;

const uint8_t MCPR_PKT_PL_CB_TAB_COMPLETE                  = 0x0E;
const uint8_t MCPR_PKT_PL_CB_CHAT_MESSAGE                  = 0x0F;
const uint8_t MCPR_PKT_PL_CB_MULTI_BLOCK_CHANGE            = 0x10;
const uint8_t MCPR_PKT_PL_CB_CONFIRM_TRANSACTION           = 0x11;
const uint8_t MCPR_PKT_PL_CB_CLOSE_WINDOW                  = 0x12;
const uint8_t MCPR_PKT_PL_CB_OPEN_WINDOW                   = 0x13;
const uint8_t MCPR_PKT_PL_CB_WINDOW_ITEMS                  = 0x14;
const uint8_t MCPR_PKT_PL_CB_WINDOW_PROPERTY               = 0x15;
const uint8_t MCPR_PKT_PL_CB_SET_SLOT                      = 0x16;
const uint8_t MCPR_PKT_PL_CB_SET_COOLDOWN                  = 0x17;
const uint8_t MCPR_PKT_PL_CB_PLUGIN_MESSAGE                = 0x18;
const uint8_t MCPR_PKT_PL_CB_NAMED_SOUND_EFFECT            = 0x19;
const uint8_t MCPR_PKT_PL_CB_DISCONNECT                    = 0x1A;
const uint8_t MCPR_PKT_PL_CB_ENTITY_STATUS                 = 0x1B;
const uint8_t MCPR_PKT_PL_CB_EXPLOSION                     = 0x1C;
const uint8_t MCPR_PKT_PL_CB_UNLOAD_CHUNK                  = 0x1D;
const uint8_t MCPR_PKT_PL_CB_CHANGE_GAME_STATE             = 0x1E;
const uint8_t MCPR_PKT_PL_CB_KEEP_ALIVE                    = 0x1F;
const uint8_t MCPR_PKT_PL_CB_CHUNK_DATA                    = 0x20;
const uint8_t MCPR_PKT_PL_CB_EFFECT                        = 0x21;
const uint8_t MCPR_PKT_PL_CB_PARTICLE                      = 0x22;
const uint8_t MCPR_PKT_PL_CB_JOIN_GAME                     = 0x23;
const uint8_t MCPR_PKT_PL_CB_MAP                           = 0x24;
const uint8_t MCPR_PKT_PL_CB_ENTITY_RELATIVE_MOVE          = 0x25;
const uint8_t MCPR_PKT_PL_CB_ENTITY_LOOK_AND_RELATIVE_MOVE = 0x26;
const uint8_t MCPR_PKT_PL_CB_ENTITY_LOOK                   = 0x27;
const uint8_t MCPR_PKT_PL_CB_ENTITY                        = 0x28;
const uint8_t MCPR_PKT_PL_CB_VEHICLE_MOVE                  = 0x29;
const uint8_t MCPR_PKT_PL_CB_OPEN_SIGN_EDITOR              = 0x2A;
const uint8_t MCPR_PKT_PL_CB_PLAYER_ABILITIES              = 0x2B;
const uint8_t MCPR_PKT_PL_CB_COMBAT_EVENT                  = 0x2C;
const uint8_t MCPR_PKT_PL_CB_PLAYER_LIST_ITEM              = 0x2D;
const uint8_t MCPR_PKT_PL_CB_PLAYER_POSITION_AND_LOOK      = 0x2E;
const uint8_t MCPR_PKT_PL_CB_USE_BED                       = 0x2F;
const uint8_t MCPR_PKT_PL_CB_DESTROY_ENTITIES              = 0x30;
const uint8_t MCPR_PKT_PL_CB_REMOVE_ENTITY_EFFECT          = 0x31;
const uint8_t MCPR_PKT_PL_CB_RESOURCE_PACK_SEND            = 0x32;
const uint8_t MCPR_PKT_PL_CB_RESPAWN                       = 0x33;
const uint8_t MCPR_PKT_PL_CB_ENTITY_HEAD_LOOK              = 0x34;
const uint8_t MCPR_PKT_PL_CB_WORLD_BORDER                  = 0x35;
const uint8_t MCPR_PKT_PL_CB_CAMERA                        = 0x36;
const uint8_t MCPR_PKT_PL_CB_HELD_ITEM_CHANGE              = 0x37;
const uint8_t MCPR_PKT_PL_CB_DISPLAY_SCOREBOARD            = 0x38;
const uint8_t MCPR_PKT_PL_CB_ENTITY_METADATA               = 0x39;
const uint8_t MCPR_PKT_PL_CB_ATTACH_ENTITY                 = 0x3A;
const uint8_t MCPR_PKT_PL_CB_ENTITY_VELOCITY               = 0x3B;
const uint8_t MCPR_PKT_PL_CB_ENTITY_EQUIPMENT              = 0x3C;
const uint8_t MCPR_PKT_PL_CB_SET_EXPERIENCE                = 0x3D;
const uint8_t MCPR_PKT_PL_CB_UPDATE_HEALTH                 = 0x3E;
const uint8_t MCPR_PKT_PL_CB_SCOREBOARD_OBJECTIVE          = 0x3F;
const uint8_t MCPR_PKT_PL_CB_SET_PASSENGERS                = 0x40;
const uint8_t MCPR_PKT_PL_CB_TEAMS                         = 0x41;
const uint8_t MCPR_PKT_PL_CB_UPDATE_SCORE                  = 0x42;
const uint8_t MCPR_PKT_PL_CB_SPAWN_POSITION                = 0x43;
const uint8_t MCPR_PKT_PL_CB_TIME_UPDATE                   = 0x44;
const uint8_t MCPR_PKT_PL_CB_TITLE                         = 0x45;
const uint8_t MCPR_PKT_PL_CB_SOUND_EFFECT                  = 0x46;
const uint8_t MCPR_PKT_PL_CB_PLAYER_LIST_HEADER_AND_FOOTER = 0x47;
const uint8_t MCPR_PKT_PL_CB_COLLECT_ITEM                  = 0x48;
const uint8_t MCPR_PKT_PL_CB_ENTITY_TELEPORT               = 0x49;
const uint8_t MCPR_PKT_PL_CB_ENTITY_PROPERTIES             = 0x4A;
const uint8_t MCPR_PKT_PL_CB_ENTITY_EFFECT                 = 0x4B;

//              --- Serverbound ---
const uint8_t MCPR_PKT_PL_SB_TELEPORT_CONFIRM              = 0x00;
const uint8_t MCPR_PKT_PL_SB_TAB_COMPLETE                  = 0x01;
const uint8_t MCPR_PKT_PL_SB_CHAT_MESSAGE                  = 0x02;
const uint8_t MCPR_PKT_PL_SB_CLIENT_STATUS                 = 0x03;
const uint8_t MCPR_PKT_PL_SB_CLIENT_SETTINGS               = 0x04;
const uint8_t MCPR_PKT_PL_SB_CONFIRM_TRANSACTION           = 0x05;
const uint8_t MCPR_PKT_PL_SB_ENCHANT_ITEM                  = 0x06;
const uint8_t MCPR_PKT_PL_SB_CLICK_WINDOW                  = 0x07;
const uint8_t MCPR_PKT_PL_SB_CLOSE_WINDOW                  = 0x08;
const uint8_t MCPR_PKT_PL_SB_PLUGIN_MESSAGE                = 0x09;
const uint8_t MCPR_PKT_PL_SB_USE_ENTITY                    = 0x0A;
const uint8_t MCPR_PKT_PL_SB_KEEP_ALIVE                    = 0x0B;
const uint8_t MCPR_PKT_PL_SB_PLAYER_POSITION               = 0x0C;
const uint8_t MCPR_PKT_PL_SB_PLAYER_POSITION_AND_LOOK      = 0x0D;
const uint8_t MCPR_PKT_PL_SB_PLAYER_LOOK                   = 0x0E;
const uint8_t MCPR_PKT_PL_SB_PLAYER                        = 0x0F;
const uint8_t MCPR_PKT_PL_SB_VEHICLE_MOVE                  = 0x10;
const uint8_t MCPR_PKT_PL_SB_STEER_BOAT                    = 0x11;
const uint8_t MCPR_PKT_PL_SB_PLAYER_ABILITIES              = 0x12;
const uint8_t MCPR_PKT_PL_SB_PLAYER_DIGGING                = 0x13;
const uint8_t MCPR_PKT_PL_SB_ENTITY_ACTION                 = 0x14;
const uint8_t MCPR_PKT_PL_SB_STEER_VEHICLE                 = 0x15;
const uint8_t MCPR_PKT_PL_SB_RESOURCE_PACK_STATUS          = 0x16;
const uint8_t MCPR_PKT_PL_SB_HELD_ITEM_CHANGE              = 0x17;
const uint8_t MCPR_PKT_PL_SB_CREATIVE_INVENTORY_ACTION     = 0x18;
const uint8_t MCPR_PKT_PL_SB_UPDATE_SIGN                   = 0x19;
const uint8_t MCPR_PKT_PL_SB_ANIMATION                     = 0x1A;
const uint8_t MCPR_PKT_PL_SB_SPECTATE                      = 0x1B;
const uint8_t MCPR_PKT_PL_SB_PLAYER_BLOCK_PLACEMENT        = 0x1C;
const uint8_t MCPR_PKT_PL_SB_USE_ITEM                      = 0x1D;

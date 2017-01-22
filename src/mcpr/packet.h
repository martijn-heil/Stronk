#ifndef MCPR_PACKET_H
#define MCPR_PACKET_H

#include <stdbool.h>
#include <stdint.h>

#include <jansson/jansson.h>

#include "mcpr.h"

#define PACKET_HANDSHAKE_SERVERBOUND_HANDSHAKE 0x00;

struct mcpr_packet {
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
            } clientbound;
        } play;
    } data;
};

#endif // MCPR_PACKET_H

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
                
            } clientbound;
        } play;
    } data;
};

#endif // MCPR_PACKET_H

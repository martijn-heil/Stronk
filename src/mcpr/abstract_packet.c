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

#include <stdlib.h>
#include <string.h>

#include <mcpr/mcpr.h>
#include <mcpr/codec.h>
#include <mcpr/abstract_packet.h>

#include <ninuuid/ninuuid.h>


static ssize_t mcpr_encode_packet(void **buf, const struct mcpr_abstract_packet *pkt)
{
    size_t data_size;

    switch(pkt->state)
    {
        case MCPR_STATE_HANDSHAKE:
        {
            switch(pkt->id)
            {
                case MCPR_PKT_HS_SB_HANDSHAKE:
                {
                    *buf = malloc(12 + strlen(pkt->data.handshake.serverbound.handshake.server_address));
                    if(buf == NULL) return -1;
                    void *bufpointer = *buf;

                    ssize_t bytes_written_1 = mcpr_encode_varint(bufpointer, MCPR_PKT_HS_SB_HANDSHAKE);
                    if(bytes_written_1 < 0) { free(*buf); return -1; }
                    bufpointer += bytes_written_1;

                    ssize_t bytes_written_2 = mcpr_encode_varint(bufpointer, MCPR_PROTOCOL_VERSION);
                    if(bytes_written_2 < 0) { free(*buf); return -1; }
                    bufpointer += bytes_written_2;

                    ssize_t bytes_written_3 = mcpr_encode_string(bufpointer, pkt->data.handshake.serverbound.handshake.server_address);
                    if(bytes_written_3 < 0) { free(*buf); return -1; }
                    bufpointer += bytes_written_3;

                    ssize_t bytes_written_4 = mcpr_encode_ushort(bufpointer, pkt->data.handshake.serverbound.handshake.server_port);
                    if(bytes_written_4 < 0) { free(*buf); return -1; }
                    bufpointer += bytes_written_4;

                    ssize_t bytes_written_5 = mcpr_encode_varint(bufpointer, pkt->data.handshake.serverbound.handshake.next_state);
                    if(bytes_written_5 < 0) { free(*buf); return -1; }
                    bufpointer += bytes_written_5;

                    data_size = bufpointer - *buf;
                    return data_size;
                }
            }
        }

        case MCPR_STATE_LOGIN:
        {
            switch(pkt->id)
            {
                case MCPR_PKT_LG_CB_DISCONNECT:
                {
                    *buf = malloc(5 + strlen(pkt->data.login.clientbound.disconnect.reason));
                    if(*buf == NULL) return -1;

                    ssize_t bytes_written_1 = mcpr_encode_chat(buf, pkt->data.login.clientbound.disconnect.reason);
                    if(bytes_written_1 < 0) { free(*buf); return -1; }
                    data_size = bytes_written_1;

                    return data_size;
                }

                case MCPR_PKT_LG_CB_ENCRYPTION_REQUEST:
                {
                    *buf = malloc(15 + strlen(pkt->data.login.clientbound.encryption_request.server_id) + pkt->data.login.clientbound.encryption_request.public_key_length + pkt->data.login.clientbound.encryption_request.verify_token_length);
                    if(*buf == NULL) return -1;
                    void *bufpointer = *buf;

                    ssize_t bytes_written_1 = mcpr_encode_string(bufpointer, pkt->data.login.clientbound.encryption_request.server_id);
                    if(bytes_written_1 < 0) { free(*buf); return -1; }
                    bufpointer += bytes_written_1;

                    ssize_t bytes_written_2 = mcpr_encode_varint(bufpointer, pkt->data.login.clientbound.encryption_request.public_key_length);
                    if(bytes_written_2 < 0) { free(*buf); return -1; }
                    bufpointer += bytes_written_2;

                    memcpy(bufpointer, pkt->data.login.clientbound.encryption_request.public_key, pkt->data.login.clientbound.encryption_request.public_key_length);
                    bufpointer += pkt->data.login.clientbound.encryption_request.public_key_length;

                    ssize_t bytes_written_3 = mcpr_encode_varint(bufpointer, pkt->data.login.clientbound.encryption_request.verify_token_length);
                    if(bytes_written_3 < 0) { free(*buf); return -1; }
                    bufpointer += bytes_written_3;

                    memcpy(bufpointer, pkt->data.login.clientbound.encryption_request.verify_token, pkt->data.login.clientbound.encryption_request.verify_token_length);
                    bufpointer += pkt->data.login.clientbound.encryption_request.verify_token_length;

                    data_size = bufpointer - *buf;
                    return data_size;
                }

                case MCPR_PKT_LG_CB_LOGIN_SUCCESS:
                {
                    *buf = malloc(46 + strlen(pkt->data.login.clientbound.login_success.username));
                    if(*buf == NULL) return -1;
                    void *bufpointer = *buf;

                    char uuid_string[37];
                    ninuuid_to_string(&(pkt->data.login.clientbound.login_success.uuid), uuid_string);

                    ssize_t bytes_written_1 = mcpr_encode_string(bufpointer, uuid_string);
                    if(bytes_written_1 < 0) { free(*buf); return -1; }
                    bufpointer += bytes_written_1;

                    ssize_t bytes_written_2 = mcpr_encode_string(bufpointer, pkt->data.login.clientbound.login_success.username);
                    if(bytes_written_2 < 0) { free(*buf); return -1; }
                    bufpointer += bytes_written_2;

                    data_size = bufpointer - *buf;
                    return data_size;
                }

                case MCPR_PKT_LG_CB_SET_COMPRESSION:
                {
                    *buf = malloc(5);
                    if(*buf == NULL) return -1;

                    ssize_t bytes_written_1 = mcpr_encode_varint(*buf, pkt->data.login.clientbound.set_compression.threshold);
                    if(bytes_written_1 < 0) { free(*buf); return -1; }
                    data_size = bytes_written_1;
                    return data_size;
                }

                case MCPR_PKT_LG_SB_LOGIN_START:
                {
                    *buf = malloc(5 + strlen(pkt->data.login.serverbound.login_start.name));
                    if(*buf == NULL) return -1;

                    ssize_t bytes_written_1 = mcpr_encode_string(*buf ,pkt->data.login.serverbound.login_start.name);
                    if(bytes_written_1 < 0) { free(*buf); return -1; }

                    data_size = bytes_written_1;
                    return data_size;
                }

                case MCPR_PKT_LG_SB_ENCRYPTION_RESPONSE:
                {
                    int32_t shared_secret_length = pkt->data.login.serverbound.encryption_response.shared_secret_length;
                    int32_t verify_token_length = pkt->data.login.serverbound.encryption_response.verify_token_length;
                    *buf = malloc(10 + shared_secret_length + verify_token_length);
                    if(*buf == NULL) return -1;

                    void *bufpointer = *buf;

                    ssize_t bytes_written_1 = mcpr_encode_varint(bufpointer, shared_secret_length);
                    if(bytes_written_1 < 0) { free(*buf); return -1; }
                    bufpointer += bytes_written_1;

                    memcpy(bufpointer, pkt->data.login.serverbound.encryption_response.shared_secret, shared_secret_length);
                    bufpointer += shared_secret_length;

                    ssize_t bytes_written_2 = mcpr_encode_varint(bufpointer, verify_token_length);
                    if(bytes_written_2 < 0) { free(*buf); return -1; }
                    bufpointer += bytes_written_2;

                    memcpy(bufpointer, pkt->data.login.serverbound.encryption_response.verify_token, verify_token_length);
                    bufpointer += verify_token_length;

                    data_size = bufpointer - *buf;
                    return data_size;
                }
            }
        }

        case MCPR_STATE_PLAY:
        {
            case MCPR_PKT_PL_CB_DISCONNECT:
            {
                char *reason = pkt->data.play.clientbound.disconnect.reason; // It's JSON chat, not a normal string
                *buf = malloc(5 + strlen(reason));
                if(*buf == NULL) return -1;

                ssize_t bytes_written_1 = mcpr_encode_string(*buf, reason);
                if(bytes_written_1 < 0) { free(*buf); return -1; }

                data_size = bytes_written_1;
                return data_size;
            }

            case MCPR_PKT_PL_CB_KEEP_ALIVE:
            {
                *buf = malloc(5);
                if(*buf == NULL) return -1;

                ssize_t bytes_written_1 = mcpr_encode_varint(*buf, pkt->data.play.clientbound.keep_alive.keep_alive_id);
                if(bytes_written_1 < 0) { free(*buf); return -1; }

                data_size = bytes_written_1;
                return data_size;
            }

            case MCPR_PKT_PL_CB_JOIN_GAME:
            {
                *buf = malloc(28);
                if(*buf == NULL) return -1;

                void *bufpointer = *buf;

                ssize_t bytes_written_1 = mcpr_encode_int(bufpointer, pkt->data.play.clientbound.join_game.entity_id);
                if(bytes_written_1 < 0) { free(*buf); return -1; }
                bufpointer += bytes_written_1;

                uint8_t gamemode;
                switch(pkt->data.play.clientbound.join_game.gamemode)
                {
                    case MCPR_GAMEMODE_SURVIVAL:    gamemode = 0x00; break;
                    case MCPR_GAMEMODE_CREATIVE:    gamemode = 0x01; break;
                    case MCPR_GAMEMODE_ADVENTURE:   gamemode = 0x02; break;
                    case MCPR_GAMEMODE_SPECTATOR:   gamemode = 0x03; break;
                }
                if(pkt->data.play.clientbound.join_game.hardcore) gamemode = gamemode | 0x08;
                ssize_t bytes_written_2 = mcpr_encode_ubyte(bufpointer, gamemode);
                if(bytes_written_2 < 0) { free(*buf); return -1; }
                bufpointer += bytes_written_2;

                int32_t dimension;
                switch(pkt->data.play.clientbound.join_game.dimension)
                {
                    case MCPR_DIMENSION_NETHER:     dimension = -1; break;
                    case MCPR_DIMENSION_OVERWORLD:  dimension = 0;  break;
                    case MCPR_DIMENSION_END:        dimension = 1;  break;
                }
                ssize_t bytes_written_3 = mcpr_encode_int(bufpointer, dimension);
                if(bytes_written_3 < 0) { free(*buf); return -1; }
                bufpointer += bytes_written_3;

                uint8_t difficulty;
                switch(pkt->data.play.clientbound.join_game.difficulty)
                {
                    case MCPR_DIFFICULTY_PEACEFUL:  difficulty = 0; break;
                    case MCPR_DIFFICULTY_EASY:      difficulty = 1; break;
                    case MCPR_DIFFICULTY_NORMAL:    difficulty = 2; break;
                    case MCPR_DIFFICULTY_HARD:      difficulty = 3; break;
                }
                ssize_t bytes_written_4 = mcpr_encode_ubyte(bufpointer, difficulty);
                if(bytes_written_4 < 0) { free(*buf); return -1; }
                bufpointer += bytes_written_4;

                ssize_t bytes_written_5 = mcpr_encode_ubyte(bufpointer, pkt->data.play.clientbound.join_game.max_players);
                if(bytes_written_5 < 0) { free(*buf); return -1; }
                bufpointer += bytes_written_5;

                char *level_type;
                switch(pkt->data.play.clientbound.join_game.level_type)
                {
                    case MCPR_LEVEL_DEFAULT:        level_type = "default";     break;
                    case MCPR_LEVEL_FLAT:           level_type = "flat";        break;
                    case MCPR_LEVEL_LARGE_BIOMES:   level_type = "largeBiomes"; break;
                    case MCPR_LEVEL_AMPLIFIED:      level_type = "amplified";   break;
                    case MCPR_LEVEL_DEFAULT_1_1:    level_type = "default_1_1"; break;
                }
                ssize_t bytes_written_6 = mcpr_encode_string(bufpointer, level_type);
                if(bytes_written_6 < 0) { free(*buf); return -1; }
                bufpointer += bytes_written_6;

                ssize_t bytes_written_7 = mcpr_encode_bool(bufpointer, pkt->data.play.clientbound.join_game.reduced_debug_info);
                if(bytes_written_7 < 0) { free(*buf); return -1; }
                bufpointer += bytes_written_7;

                data_size = bufpointer - *buf;
                return data_size;
            }

            case MCPR_PKT_PL_CB_PLUGIN_MESSAGE:
            {
                *buf = malloc(5 + strlen(pkt->data.play.clientbound.plugin_message.channel) + pkt->data.play.clientbound.plugin_message.data_length);
                if(*buf == NULL) return -1;
                void *bufpointer = *buf;

                ssize_t bytes_written_1 = mcpr_encode_string(bufpointer, pkt->data.play.clientbound.plugin_message.channel);
                if(bytes_written_1 < 0) { free(*buf); return -1; }
                bufpointer += bytes_written_1;

                memcpy(bufpointer, pkt->data.play.clientbound.plugin_message.data, pkt->data.play.clientbound.plugin_message.data_length);
                bufpointer += pkt->data.play.clientbound.plugin_message.data_length;

                data_size = bufpointer - *buf;
                return data_size;
            }

            case MCPR_PKT_PL_CB_SPAWN_POSITION:
            {
                *buf = malloc(MCPR_POSITION_SIZE);
                if(*buf == NULL) return -1;
                void *bufpointer = *buf;

                ssize_t bytes_written_1 = mcpr_encode_position(bufpointer, &(pkt->data.play.clientbound.spawn_position.location));
                if(bytes_written_1 < 0) { free(*buf); return -1; }
                bufpointer += bytes_written_1;

                data_size = bufpointer - *buf;
                return data_size;
            }

            case MCPR_PKT_PL_CB_PLAYER_ABILITIES:
            {
                *buf = malloc(MCPR_BYTE_SIZE + MCPR_FLOAT_SIZE + MCPR_FLOAT_SIZE);
                if(*buf == NULL) return -1;
                void *bufpointer = *buf;

                int8_t flags = 0;
                if(pkt->data.play.clientbound.player_abilities.invulnerable)    flags = flags | 0x01;
                if(pkt->data.play.clientbound.player_abilities.flying)          flags = flags | 0x02;
                if(pkt->data.play.clientbound.player_abilities.allow_flying)    flags = flags | 0x04;
                if(pkt->data.play.clientbound.player_abilities.creative_mode)   flags = flags | 0x08;
                ssize_t bytes_written_1 = mcpr_encode_byte(bufpointer, flags);
                if(bytes_written_1 < 0) { free(*buf); return -1; }
                bufpointer += bytes_written_1;

                ssize_t bytes_written_2 = mcpr_encode_float(bufpointer, pkt->data.play.clientbound.player_abilities.flying_speed);
                if(bytes_written_2 < 0) { free(*buf); return -1; }
                bufpointer += bytes_written_2;

                ssize_t bytes_written_3 = mcpr_encode_float(bufpointer, pkt->data.play.clientbound.player_abilities.field_of_view_modifier);
                if(bytes_written_3 < 0) { free(*buf); return -1; }
                bufpointer += bytes_written_3;

                data_size = bufpointer - *buf;
                return data_size;
            }

            case MCPR_PKT_PL_CB_PLAYER_POSITION_AND_LOOK:
            {
                *buf = malloc(MCPR_DOUBLE_SIZE * 3 + MCPR_FLOAT_SIZE * 2 + MCPR_BYTE_SIZE + MCPR_VARINT_SIZE_MAX);
                if(*buf == NULL) return -1;
                void *bufpointer = *buf;

                ssize_t bytes_written_1 = mcpr_encode_double(bufpointer, pkt->data.play.clientbound.player_position_and_look.x);
                if(bytes_written_1 < 0) { free(*buf); return -1; }
                bufpointer += bytes_written_1;

                ssize_t bytes_written_2 = mcpr_encode_double(bufpointer, pkt->data.play.clientbound.player_position_and_look.y);
                if(bytes_written_2 < 0) { free(*buf); return -1; }
                bufpointer += bytes_written_2;

                ssize_t bytes_written_3 = mcpr_encode_double(bufpointer, pkt->data.play.clientbound.player_position_and_look.z);
                if(bytes_written_3 < 0) { free(*buf); return -1; }
                bufpointer += bytes_written_3;

                ssize_t bytes_written_4 = mcpr_encode_float(bufpointer, pkt->data.play.clientbound.player_position_and_look.yaw);
                if(bytes_written_4 < 0) { free(*buf); return -1; }
                bufpointer += bytes_written_4;

                ssize_t bytes_written_5 = mcpr_encode_float(bufpointer, pkt->data.play.clientbound.player_position_and_look.pitch);
                if(bytes_written_5 < 0) { free(*buf); return -1; }
                bufpointer += bytes_written_5;

                int8_t flags = 0;
                if(pkt->data.play.clientbound.player_position_and_look.x_is_relative)       flags = flags | 0x01;
                if(pkt->data.play.clientbound.player_position_and_look.y_is_relative)       flags = flags | 0x02;
                if(pkt->data.play.clientbound.player_position_and_look.z_is_relative)       flags = flags | 0x04;
                if(pkt->data.play.clientbound.player_position_and_look.pitch_is_relative)   flags = flags | 0x08;
                if(pkt->data.play.clientbound.player_position_and_look.yaw_is_relative)     flags = flags | 0x10;
                ssize_t bytes_written_6 = mcpr_encode_byte(bufpointer, flags);
                if(bytes_written_6 < 0) { free(*buf); return -1; }
                bufpointer += bytes_written_6;

                ssize_t bytes_written_7 = mcpr_encode_varint(bufpointer, pkt->data.play.clientbound.player_position_and_look.teleport_id);
                if(bytes_written_7 < 0) { free(*buf); return -1; }
                bufpointer += bytes_written_7;

                data_size = bufpointer - *buf;
                return data_size;
            }

            case MCPR_PKT_PL_SB_KEEP_ALIVE:
            {
                *buf = malloc(5);
                if(*buf == NULL) return -1;

                ssize_t bytes_written_1 = mcpr_encode_varint(*buf, pkt->data.play.serverbound.keep_alive.keep_alive_id);
                if(bytes_written_1 < 0) { free(*buf); return -1; }

                data_size = bytes_written_1;
                return data_size;
            }

            case MCPR_PKT_PL_SB_PLUGIN_MESSAGE:
            {
                *buf = malloc(5 + strlen(pkt->data.play.serverbound.plugin_message.channel) + pkt->data.play.serverbound.plugin_message.data_length);
                if(*buf == NULL) return -1;
                void *bufpointer = *buf;

                ssize_t bytes_written_1 = mcpr_encode_string(bufpointer, pkt->data.play.serverbound.plugin_message.channel);
                if(bytes_written_1 < 0) { free(*buf); return -1; }
                bufpointer += bytes_written_1;

                memcpy(bufpointer, pkt->data.play.serverbound.plugin_message.data, pkt->data.play.serverbound.plugin_message.data_length);
                bufpointer += pkt->data.play.serverbound.plugin_message.data_length;

                data_size = bufpointer - *buf;
                return data_size;
            }
        }
    }

    return data_size;
}

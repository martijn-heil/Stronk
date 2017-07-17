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
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

#include <ninerr/ninerr.h>

#include <mcpr/mcpr.h>
#include <mcpr/codec.h>
#include <mcpr/packet.h>
#include "util.h"

#include <ninuuid/ninuuid.h>

static char *server_list_response_to_json(const struct mcpr_packet *pkt)
{
    char *version_name = pkt->data.status.clientbound.response.version_name;
    int protocol_version = pkt->data.status.clientbound.response.protocol_version;
    unsigned int max_players = pkt->data.status.clientbound.response.max_players;
    unsigned int online_players = pkt->data.status.clientbound.response.online_players;
    char *description = pkt->data.status.clientbound.response.description;
    char *favicon = pkt->data.status.clientbound.response.favicon;
    size_t online_players_size = pkt->data.status.clientbound.response.online_players_size;
    struct mcpr_player_sample *player_sample = pkt->data.status.clientbound.response.player_sample;


    const char *main_fmt;
    if(favicon != NULL && player_sample != NULL)
    {
        main_fmt = "{\"version\":{\"name\":\"%s\",\"protocol\":%i},\"players\":{\"max\":%i,\"online\":%i,\"sample\":%s},\"description\":%s,\"favicon\":\"%s\"}";
    }
    else if(favicon != NULL)
    {
        main_fmt = "{\"version\":{\"name\":\"%s\",\"protocol\":%i},\"players\":{\"max\":%i,\"online\":%i},\"description\":%s,\"favicon\":\"%s\"}";
    }
    else if(player_sample != NULL)
    {
        main_fmt = "{\"version\":{\"name\":\"%s\",\"protocol\":%i},\"players\":{\"max\":%i,\"online\":%i,\"sample\":%s},\"description\":%s}";
    }
    else
    {
        main_fmt = "{\"version\":{\"name\":\"%s\",\"protocol\":%i},\"players\":{\"max\":%i,\"online\":%i},\"description\":%s}";
    }

    if(player_sample != NULL)
    {
        size_t online_players_size = online_players_size;
        const char *player_sample_fmt = "{\"name\":\"%s\",\"id\":\"%s\"}";

        size_t buf_size = 256;
        char *buf = malloc(buf_size);
        if(buf == NULL) { ninerr_set_err(ninerr_from_errno()); return NULL; }
        buf[0] = '[';
        char *bufp = buf + 1;

        char tmp_id[NINUUID_STRING_SIZE + 1];
        for(size_t i = 0; i < online_players_size; i++)
        {
            const struct mcpr_player_sample sample = player_sample[i];
            ninuuid_to_string(&(sample.uuid), tmp_id, LOWERCASE, false);
            size_t remaining_size = buf_size - (bufp - buf);
            if(buf_size < 19 + strlen(sample.player_name) + 3) // +3 for the potential comma, closing ] and nul byte, 19 comes from strlen(player_sample_fmt), minus the %s's
            {
                char *tmp = realloc(buf, buf_size + 256);
                if(tmp == NULL) { ninerr_set_err(ninerr_from_errno()); free(buf); return NULL; }
                bufp = tmp + (bufp - buf);
                buf = tmp;
            }

            int result = sprintf(bufp, player_sample_fmt, sample.player_name, tmp_id);
            if(result < 0) { ninerr_set_err(ninerr_new("sprintf failed.")); free(buf); return NULL; }
            bufp += result;
            if(i != (online_players_size - 1)) { *bufp = ','; bufp++; } // only add the comma if this isn't the last entry.
        }
        *bufp = ']';
        *(bufp + 1) = '\0';

        if(favicon != NULL)
        {
            char tmpbuf;
            int required_size = snprintf(&tmpbuf, 1, main_fmt, version_name, protocol_version, max_players, online_players, buf, description, favicon);
            if(required_size < 0) { ninerr_set_err(ninerr_new("sprintf failed.")); free(buf); return NULL; }

            char *final_buf = malloc(required_size + 1);
            if(final_buf == NULL) { ninerr_set_err(ninerr_from_errno()); free(buf); return NULL; }

            int final_result = sprintf(final_buf, main_fmt, version_name, protocol_version, max_players, online_players, buf, description, favicon);
            if(final_result < 0) { ninerr_set_err(ninerr_new("sprintf failed.")); free(buf); free(final_buf); return NULL; }

            free(buf);
            return final_buf;
        }
        else
        {
            char tmpbuf;
            int required_size = snprintf(&tmpbuf, 1, main_fmt, version_name, protocol_version, max_players, online_players, buf, description);
            if(required_size < 0) { ninerr_set_err(ninerr_new("sprintf failed.")); free(buf); return NULL; }

            char *final_buf = malloc(required_size + 1);
            if(final_buf == NULL) { ninerr_set_err(ninerr_from_errno()); free(buf); return NULL; }

            int final_result = sprintf(final_buf, main_fmt, version_name, protocol_version, max_players, online_players, buf, description);
            if(final_result < 0) { ninerr_set_err(ninerr_new("sprintf failed.")); free(buf); free(final_buf); return NULL; }

            free(buf);
            return final_buf;
        }

    }
    else
    {
        if(favicon != NULL)
        {
            char tmpbuf;
            int required_size = snprintf(&tmpbuf, 1, main_fmt, version_name, protocol_version, max_players, online_players, description, favicon);
            if(required_size < 0) { ninerr_set_err(ninerr_new("sprintf failed.")); return NULL; }

            char *final_buf = malloc(required_size + 1);
            if(final_buf == NULL) { ninerr_set_err(ninerr_from_errno()); return NULL; }

            int final_result = sprintf(final_buf, main_fmt, version_name, protocol_version, max_players, online_players, description, favicon);
            if(final_result < 0) { ninerr_set_err(ninerr_new("sprintf failed.")); return NULL; }

            return final_buf;
        }
        else
        {
            char tmpbuf;
            int required_size = snprintf(&tmpbuf, 1, main_fmt, version_name, protocol_version, max_players, online_players, description);
            if(required_size < 0) { ninerr_set_err(ninerr_new("sprintf failed.")); return NULL; }

            char *final_buf = malloc(required_size + 1);
            if(final_buf == NULL) { ninerr_set_err(ninerr_from_errno()); return NULL; }

            int final_result = sprintf(final_buf, main_fmt, version_name, protocol_version, max_players, online_players, description);
            if(final_result < 0) { ninerr_set_err(ninerr_new("sprintf failed.")); return NULL; }

            return final_buf;
        }
    }
}


bool mcpr_encode_packet(void **buf, size_t *out_bytes_written, const struct mcpr_packet *pkt)
{
    printf("In mcpr_encode_packet\n");
    IGNORE("-Wswitch")

    switch(pkt->state)
    {
        case MCPR_STATE_HANDSHAKE:
        {
            switch(pkt->id)
            {
                case MCPR_PKT_HS_SB_HANDSHAKE:
                {
                    *buf = malloc(12 + strlen(pkt->data.handshake.serverbound.handshake.server_address));
                    if(buf == NULL) { ninerr_set_err(ninerr_from_errno()); return false; }
                    void *bufpointer = *buf;

                    ssize_t bytes_written_1 = mcpr_encode_varint(bufpointer, mcpr_packet_type_to_byte(MCPR_PKT_HS_SB_HANDSHAKE));
                    if(bytes_written_1 < 0) { free(*buf); return false; }
                    bufpointer += bytes_written_1;

                    ssize_t bytes_written_2 = mcpr_encode_varint(bufpointer, MCPR_PROTOCOL_VERSION);
                    if(bytes_written_2 < 0) { free(*buf); return false; }
                    bufpointer += bytes_written_2;

                    ssize_t bytes_written_3 = mcpr_encode_string(bufpointer, pkt->data.handshake.serverbound.handshake.server_address);
                    if(bytes_written_3 < 0) { free(*buf); return false; }
                    bufpointer += bytes_written_3;

                    ssize_t bytes_written_4 = mcpr_encode_ushort(bufpointer, pkt->data.handshake.serverbound.handshake.server_port);
                    if(bytes_written_4 < 0) { free(*buf); return false; }
                    bufpointer += bytes_written_4;

                    ssize_t bytes_written_5 = mcpr_encode_varint(bufpointer, pkt->data.handshake.serverbound.handshake.next_state);
                    if(bytes_written_5 < 0) { free(*buf); return false; }
                    bufpointer += bytes_written_5;

                    *out_bytes_written = bufpointer - *buf;
                    return true;
                }
            }
        }

        case MCPR_STATE_STATUS:
        {
            switch(pkt->id)
            {
                case MCPR_PKT_ST_CB_PONG:
                {
                    *buf = malloc(MCPR_VARINT_SIZE_MAX + MCPR_LONG_SIZE);
                    if(*buf == NULL) { ninerr_set_err(ninerr_from_errno()); free(*buf); return false; }
                    void *bufpointer = *buf;

                    ssize_t bytes_written_1 = mcpr_encode_varint(bufpointer, mcpr_packet_type_to_byte(MCPR_PKT_ST_CB_PONG));
                    if(bytes_written_1 < 0) { free(*buf); return false; }
                    bufpointer += bytes_written_1;

                    ssize_t bytes_written_2 = mcpr_encode_long(bufpointer, pkt->data.status.clientbound.pong.payload);
                    if(bytes_written_2 < 0) { free(*buf); return false; }
                    bufpointer += bytes_written_2;

                    *out_bytes_written = bufpointer - *buf;
                    return true;
                }

                case MCPR_PKT_ST_CB_RESPONSE:
                {
                    char *response = server_list_response_to_json(pkt);
                    if(response == NULL) { return false; }
                    printf("Response: %s\n", response);
                    size_t len = strlen(response);

                    *buf = malloc(MCPR_VARINT_SIZE_MAX + MCPR_VARINT_SIZE_MAX + len);
                    if(*buf == NULL) { ninerr_set_err(ninerr_from_errno()); free(*buf); return false; }
                    void *bufpointer = *buf;

                    ssize_t bytes_written_1 = mcpr_encode_varint(bufpointer, mcpr_packet_type_to_byte(MCPR_PKT_ST_CB_RESPONSE));
                    if(bytes_written_1 < 0) { free(*buf); return false; }
                    bufpointer += bytes_written_1;

                    ssize_t bytes_written_2 = mcpr_encode_varint(bufpointer, len);
                    if(bytes_written_2 < 0) { free(*buf); free(response); return false; }
                    bufpointer += bytes_written_2;
                    memcpy(bufpointer, response, len);
                    bufpointer += len;
                    free(response);
                    *out_bytes_written = bufpointer - *buf;
                    return true;
                }
            }
        }

        case MCPR_STATE_LOGIN:
        {
            switch(pkt->id)
            {
                case MCPR_PKT_LG_CB_DISCONNECT:
                {
                    *buf = malloc(10 + strlen(pkt->data.login.clientbound.disconnect.reason));
                    if(*buf == NULL) { ninerr_set_err(ninerr_from_errno()); return false; }
                    void *bufpointer = *buf;

                    ssize_t bytes_written_1 = mcpr_encode_varint(bufpointer, mcpr_packet_type_to_byte(MCPR_PKT_LG_CB_DISCONNECT));
                    if(bytes_written_1 < 0) { free(*buf); return false; }
                    bufpointer += bytes_written_1;

                    ssize_t bytes_written_2 = mcpr_encode_chat(buf, pkt->data.login.clientbound.disconnect.reason);
                    if(bytes_written_2 < 0) { free(*buf); return false; }

                    *out_bytes_written = bufpointer - *buf;
                    return true;
                }

                case MCPR_PKT_LG_CB_ENCRYPTION_REQUEST:
                {
                    *buf = malloc(20 + strlen(pkt->data.login.clientbound.encryption_request.server_id) + pkt->data.login.clientbound.encryption_request.public_key_length + pkt->data.login.clientbound.encryption_request.verify_token_length);
                    if(*buf == NULL) { ninerr_set_err(ninerr_from_errno()); return false; }
                    void *bufpointer = *buf;

                    ssize_t bytes_written_1 = mcpr_encode_varint(bufpointer, mcpr_packet_type_to_byte(MCPR_PKT_LG_CB_ENCRYPTION_REQUEST));
                    if(bytes_written_1 < 0) { free(*buf); return false; }
                    bufpointer += bytes_written_1;

                    ssize_t bytes_written_2 = mcpr_encode_string(bufpointer, pkt->data.login.clientbound.encryption_request.server_id);
                    if(bytes_written_2 < 0) { free(*buf); return false; }
                    bufpointer += bytes_written_2;

                    ssize_t bytes_written_3 = mcpr_encode_varint(bufpointer, pkt->data.login.clientbound.encryption_request.public_key_length);
                    if(bytes_written_3 < 0) { free(*buf); return false; }
                    bufpointer += bytes_written_3;

                    memcpy(bufpointer, pkt->data.login.clientbound.encryption_request.public_key, pkt->data.login.clientbound.encryption_request.public_key_length);
                    bufpointer += pkt->data.login.clientbound.encryption_request.public_key_length;

                    ssize_t bytes_written_4 = mcpr_encode_varint(bufpointer, pkt->data.login.clientbound.encryption_request.verify_token_length);
                    if(bytes_written_4 < 0) { free(*buf); return false; }
                    bufpointer += bytes_written_4;

                    memcpy(bufpointer, pkt->data.login.clientbound.encryption_request.verify_token, pkt->data.login.clientbound.encryption_request.verify_token_length);
                    bufpointer += pkt->data.login.clientbound.encryption_request.verify_token_length;

                    *out_bytes_written = bufpointer - *buf;
                    return true;
                }

                case MCPR_PKT_LG_CB_LOGIN_SUCCESS:
                {
                    *buf = malloc(51 + strlen(pkt->data.login.clientbound.login_success.username));
                    if(*buf == NULL) { ninerr_set_err(ninerr_from_errno()); return false; }
                    void *bufpointer = *buf;

                    ssize_t bytes_written_1 = mcpr_encode_varint(bufpointer, mcpr_packet_type_to_byte(MCPR_PKT_LG_CB_ENCRYPTION_REQUEST));
                    if(bytes_written_1 < 0) { free(*buf); return false; }
                    bufpointer += bytes_written_1;

                    char uuid_string[37];
                    ninuuid_to_string(&(pkt->data.login.clientbound.login_success.uuid), uuid_string, LOWERCASE, false);
                    ssize_t bytes_written_2 = mcpr_encode_string(bufpointer, uuid_string);
                    if(bytes_written_2 < 0) { free(*buf); return false; }
                    bufpointer += bytes_written_2;

                    ssize_t bytes_written_3 = mcpr_encode_string(bufpointer, pkt->data.login.clientbound.login_success.username);
                    if(bytes_written_3 < 0) { free(*buf); return false; }
                    bufpointer += bytes_written_3;

                    *out_bytes_written = bufpointer - *buf;;
                    return true;
                }

                case MCPR_PKT_LG_CB_SET_COMPRESSION:
                {
                    *buf = malloc(10);
                    if(*buf == NULL) { ninerr_set_err(ninerr_from_errno()); return false; }
                    void *bufpointer = *buf;

                    ssize_t bytes_written_1 = mcpr_encode_varint(bufpointer, mcpr_packet_type_to_byte(MCPR_PKT_LG_CB_SET_COMPRESSION));
                    if(bytes_written_1 < 0) { free(*buf); return false; }
                    bufpointer += bytes_written_1;

                    ssize_t bytes_written_2 = mcpr_encode_varint(bufpointer, pkt->data.login.clientbound.set_compression.threshold);
                    if(bytes_written_2 < 0) { free(*buf); return false; }

                    *out_bytes_written = bufpointer - *buf;
                    return true;
                }

                case MCPR_PKT_LG_SB_LOGIN_START:
                {
                    *buf = malloc(10 + strlen(pkt->data.login.serverbound.login_start.name));
                    if(*buf == NULL) { ninerr_set_err(ninerr_from_errno()); return false; }
                    void *bufpointer = *buf;

                    ssize_t bytes_written_1 = mcpr_encode_varint(bufpointer, mcpr_packet_type_to_byte(MCPR_PKT_LG_SB_LOGIN_START));
                    if(bytes_written_1 < 0) { free(*buf); return false; }
                    bufpointer += bytes_written_1;

                    ssize_t bytes_written_2 = mcpr_encode_string(bufpointer ,pkt->data.login.serverbound.login_start.name);
                    if(bytes_written_2 < 0) { free(*buf); return false; }

                    *out_bytes_written = bufpointer - *buf;
                    return true;
                }

                case MCPR_PKT_LG_SB_ENCRYPTION_RESPONSE:
                {
                    int32_t shared_secret_length = pkt->data.login.serverbound.encryption_response.shared_secret_length;
                    int32_t verify_token_length = pkt->data.login.serverbound.encryption_response.verify_token_length;
                    *buf = malloc(15 + shared_secret_length + verify_token_length);
                    if(*buf == NULL) { ninerr_set_err(ninerr_from_errno()); return false; }
                    void *bufpointer = *buf;

                    ssize_t bytes_written_1 = mcpr_encode_varint(bufpointer, mcpr_packet_type_to_byte(MCPR_PKT_LG_SB_ENCRYPTION_RESPONSE));
                    if(bytes_written_1 < 0) { free(*buf); return false; }
                    bufpointer += bytes_written_1;

                    ssize_t bytes_written_2 = mcpr_encode_varint(bufpointer, shared_secret_length);
                    if(bytes_written_2 < 0) { free(*buf); return false; }
                    bufpointer += bytes_written_2;

                    memcpy(bufpointer, pkt->data.login.serverbound.encryption_response.shared_secret, shared_secret_length);
                    bufpointer += shared_secret_length;

                    ssize_t bytes_written_3 = mcpr_encode_varint(bufpointer, verify_token_length);
                    if(bytes_written_3 < 0) { free(*buf); return false; }
                    bufpointer += bytes_written_3;

                    memcpy(bufpointer, pkt->data.login.serverbound.encryption_response.verify_token, verify_token_length);
                    bufpointer += verify_token_length;

                    *out_bytes_written = bufpointer - *buf;
                    return true;
                }
            }
        }

        case MCPR_STATE_PLAY:
        {
            case MCPR_PKT_PL_CB_DISCONNECT:
            {
                const char *reason = pkt->data.play.clientbound.disconnect.reason; // It's JSON chat, not a normal string
                *buf = malloc(10 + strlen(reason));
                if(*buf == NULL) { ninerr_set_err(ninerr_from_errno()); return false; }
                void *bufpointer = *buf;

                ssize_t bytes_written_1 = mcpr_encode_varint(bufpointer, mcpr_packet_type_to_byte(MCPR_PKT_PL_CB_DISCONNECT));
                if(bytes_written_1 < 0) { free(*buf); return false; }
                bufpointer += bytes_written_1;

                ssize_t bytes_written_2 = mcpr_encode_string(*buf, reason);
                if(bytes_written_2 < 0) { free(*buf); return false; }
                bufpointer += bytes_written_2;

                *out_bytes_written = bufpointer - *buf;
                return true;
            }

            case MCPR_PKT_PL_CB_KEEP_ALIVE:
            {
                *buf = malloc(10);
                if(*buf == NULL) { ninerr_set_err(ninerr_from_errno()); return false; }
                void *bufpointer = *buf;

                ssize_t bytes_written_1 = mcpr_encode_varint(bufpointer, mcpr_packet_type_to_byte(MCPR_PKT_PL_CB_KEEP_ALIVE));
                if(bytes_written_1 < 0) { free(*buf); return false; }
                bufpointer += bytes_written_1;

                ssize_t bytes_written_2 = mcpr_encode_varint(*buf, pkt->data.play.clientbound.keep_alive.keep_alive_id);
                if(bytes_written_2 < 0) { free(*buf); return false; }
                bufpointer += bytes_written_2;

                *out_bytes_written = bufpointer - *buf;
                return true;
            }

            case MCPR_PKT_PL_CB_JOIN_GAME:
            {
                *buf = malloc(33);
                if(*buf == NULL) { ninerr_set_err(ninerr_from_errno()); return false; }
                void *bufpointer = *buf;

                ssize_t bytes_written_1 = mcpr_encode_varint(bufpointer, mcpr_packet_type_to_byte(MCPR_PKT_PL_CB_JOIN_GAME));
                if(bytes_written_1 < 0) { free(*buf); return false; }
                bufpointer += bytes_written_1;

                ssize_t bytes_written_2 = mcpr_encode_int(bufpointer, pkt->data.play.clientbound.join_game.entity_id);
                if(bytes_written_2 < 0) { free(*buf); return false; }
                bufpointer += bytes_written_2;

                uint8_t gamemode;
                switch(pkt->data.play.clientbound.join_game.gamemode)
                {
                    case MCPR_GAMEMODE_SURVIVAL:    gamemode = 0x00; break;
                    case MCPR_GAMEMODE_CREATIVE:    gamemode = 0x01; break;
                    case MCPR_GAMEMODE_ADVENTURE:   gamemode = 0x02; break;
                    case MCPR_GAMEMODE_SPECTATOR:   gamemode = 0x03; break;
                    default: abort(); return false; // Won't be reached, but else the compiler will complain.
                }
                if(pkt->data.play.clientbound.join_game.hardcore) gamemode = gamemode | 0x08;
                ssize_t bytes_written_3 = mcpr_encode_ubyte(bufpointer, gamemode);
                if(bytes_written_3 < 0) { free(*buf); return false; }
                bufpointer += bytes_written_3;

                int32_t dimension;
                switch(pkt->data.play.clientbound.join_game.dimension)
                {
                    case MCPR_DIMENSION_NETHER:     dimension = -1; break;
                    case MCPR_DIMENSION_OVERWORLD:  dimension = 0;  break;
                    case MCPR_DIMENSION_END:        dimension = 1;  break;
                    default: abort(); return false; // Won't be reached, but else the compiler will complain.
                }
                ssize_t bytes_written_4 = mcpr_encode_int(bufpointer, dimension);
                if(bytes_written_4 < 0) { free(*buf); return false; }
                bufpointer += bytes_written_4;

                uint8_t difficulty;
                switch(pkt->data.play.clientbound.join_game.difficulty)
                {
                    case MCPR_DIFFICULTY_PEACEFUL:  difficulty = 0; break;
                    case MCPR_DIFFICULTY_EASY:      difficulty = 1; break;
                    case MCPR_DIFFICULTY_NORMAL:    difficulty = 2; break;
                    case MCPR_DIFFICULTY_HARD:      difficulty = 3; break;
                    default: abort(); return false; // Won't be reached, but else the compiler will complain.
                }
                ssize_t bytes_written_5 = mcpr_encode_ubyte(bufpointer, difficulty);
                if(bytes_written_5 < 0) { free(*buf); return false; }
                bufpointer += bytes_written_5;

                ssize_t bytes_written_6 = mcpr_encode_ubyte(bufpointer, pkt->data.play.clientbound.join_game.max_players);
                if(bytes_written_6 < 0) { free(*buf); return false; }
                bufpointer += bytes_written_6;

                char *level_type;
                switch(pkt->data.play.clientbound.join_game.level_type)
                {
                    case MCPR_LEVEL_DEFAULT:        level_type = "default";     break;
                    case MCPR_LEVEL_FLAT:           level_type = "flat";        break;
                    case MCPR_LEVEL_LARGE_BIOMES:   level_type = "largeBiomes"; break;
                    case MCPR_LEVEL_AMPLIFIED:      level_type = "amplified";   break;
                    case MCPR_LEVEL_DEFAULT_1_1:    level_type = "default_1_1"; break;
                    default: abort(); return false; // Won't be reached, but else the compiler will complain.
                }
                ssize_t bytes_written_7 = mcpr_encode_string(bufpointer, level_type);
                if(bytes_written_7 < 0) { free(*buf); return false; }
                bufpointer += bytes_written_7;

                ssize_t bytes_written_8 = mcpr_encode_bool(bufpointer, pkt->data.play.clientbound.join_game.reduced_debug_info);
                if(bytes_written_8 < 0) { free(*buf); return false; }
                bufpointer += bytes_written_8;

                *out_bytes_written = bufpointer - *buf;
                return true;
            }

            case MCPR_PKT_PL_CB_PLUGIN_MESSAGE:
            {
                *buf = malloc(10 + strlen(pkt->data.play.clientbound.plugin_message.channel) + pkt->data.play.clientbound.plugin_message.data_length);
                if(*buf == NULL) { ninerr_set_err(ninerr_from_errno()); return false; }
                void *bufpointer = *buf;

                ssize_t bytes_written_1 = mcpr_encode_varint(bufpointer, mcpr_packet_type_to_byte(MCPR_PKT_PL_CB_PLUGIN_MESSAGE));
                if(bytes_written_1 < 0) { free(*buf); return false; }
                bufpointer += bytes_written_1;

                ssize_t bytes_written_2 = mcpr_encode_string(bufpointer, pkt->data.play.clientbound.plugin_message.channel);
                if(bytes_written_2 < 0) { free(*buf); return false; }
                bufpointer += bytes_written_2;

                memcpy(bufpointer, pkt->data.play.clientbound.plugin_message.data, pkt->data.play.clientbound.plugin_message.data_length);
                bufpointer += pkt->data.play.clientbound.plugin_message.data_length;

                *out_bytes_written = bufpointer - *buf;
                return true;
            }

            case MCPR_PKT_PL_CB_SPAWN_POSITION:
            {
                *buf = malloc(MCPR_VARINT_SIZE_MAX + MCPR_POSITION_SIZE);
                if(*buf == NULL) { ninerr_set_err(ninerr_from_errno()); return false; }
                void *bufpointer = *buf;

                ssize_t bytes_written_1 = mcpr_encode_varint(bufpointer, mcpr_packet_type_to_byte(MCPR_PKT_PL_CB_SPAWN_POSITION));
                if(bytes_written_1 < 0) { free(*buf); return false; }
                bufpointer += bytes_written_1;

                ssize_t bytes_written_2 = mcpr_encode_position(bufpointer, &(pkt->data.play.clientbound.spawn_position.location));
                if(bytes_written_2 < 0) { free(*buf); return false; }
                bufpointer += bytes_written_2;

                *out_bytes_written = bufpointer - *buf;
                return true;
            }

            case MCPR_PKT_PL_CB_PLAYER_ABILITIES:
            {
                *buf = malloc(MCPR_VARINT_SIZE_MAX + MCPR_BYTE_SIZE + MCPR_FLOAT_SIZE + MCPR_FLOAT_SIZE);
                if(*buf == NULL) { ninerr_set_err(ninerr_from_errno()); return false; }
                void *bufpointer = *buf;

                ssize_t bytes_written_1 = mcpr_encode_varint(bufpointer, mcpr_packet_type_to_byte(MCPR_PKT_PL_CB_PLAYER_ABILITIES));
                if(bytes_written_1 < 0) { free(*buf); return false; }
                bufpointer += bytes_written_1;

                int8_t flags = 0;
                if(pkt->data.play.clientbound.player_abilities.invulnerable)    flags = flags | 0x01;
                if(pkt->data.play.clientbound.player_abilities.is_flying)       flags = flags | 0x02;
                if(pkt->data.play.clientbound.player_abilities.allow_flying)    flags = flags | 0x04;
                if(pkt->data.play.clientbound.player_abilities.creative_mode)   flags = flags | 0x08;
                ssize_t bytes_written_2 = mcpr_encode_byte(bufpointer, flags);
                if(bytes_written_2 < 0) { free(*buf); return false; }
                bufpointer += bytes_written_2;

                ssize_t bytes_written_3 = mcpr_encode_float(bufpointer, pkt->data.play.clientbound.player_abilities.flying_speed);
                if(bytes_written_3 < 0) { free(*buf); return false; }
                bufpointer += bytes_written_3;

                ssize_t bytes_written_4 = mcpr_encode_float(bufpointer, pkt->data.play.clientbound.player_abilities.field_of_view_modifier);
                if(bytes_written_4 < 0) { free(*buf); return false; }
                bufpointer += bytes_written_4;

                *out_bytes_written = bufpointer - *buf;
                return true;
            }

            case MCPR_PKT_PL_CB_PLAYER_POSITION_AND_LOOK:
            {
                *buf = malloc(MCPR_VARINT_SIZE_MAX + MCPR_DOUBLE_SIZE * 3 + MCPR_FLOAT_SIZE * 2 + MCPR_BYTE_SIZE + MCPR_VARINT_SIZE_MAX);
                if(*buf == NULL) { ninerr_set_err(ninerr_from_errno()); return false; }
                void *bufpointer = *buf;

                ssize_t bytes_written_1 = mcpr_encode_varint(bufpointer, mcpr_packet_type_to_byte(MCPR_PKT_PL_CB_PLAYER_POSITION_AND_LOOK));
                if(bytes_written_1 < 0) { free(*buf); return false; }
                bufpointer += bytes_written_1;

                ssize_t bytes_written_2 = mcpr_encode_double(bufpointer, pkt->data.play.clientbound.player_position_and_look.x);
                if(bytes_written_2 < 0) { free(*buf); return false; }
                bufpointer += bytes_written_2;

                ssize_t bytes_written_3 = mcpr_encode_double(bufpointer, pkt->data.play.clientbound.player_position_and_look.y);
                if(bytes_written_3 < 0) { free(*buf); return false; }
                bufpointer += bytes_written_3;

                ssize_t bytes_written_4 = mcpr_encode_double(bufpointer, pkt->data.play.clientbound.player_position_and_look.z);
                if(bytes_written_4 < 0) { free(*buf); return false; }
                bufpointer += bytes_written_4;

                ssize_t bytes_written_5 = mcpr_encode_float(bufpointer, pkt->data.play.clientbound.player_position_and_look.yaw);
                if(bytes_written_5 < 0) { free(*buf); return false; }
                bufpointer += bytes_written_5;

                ssize_t bytes_written_6 = mcpr_encode_float(bufpointer, pkt->data.play.clientbound.player_position_and_look.pitch);
                if(bytes_written_6 < 0) { free(*buf); return false; }
                bufpointer += bytes_written_6;

                int8_t flags = 0;
                if(pkt->data.play.clientbound.player_position_and_look.x_is_relative)       flags = flags | 0x01;
                if(pkt->data.play.clientbound.player_position_and_look.y_is_relative)       flags = flags | 0x02;
                if(pkt->data.play.clientbound.player_position_and_look.z_is_relative)       flags = flags | 0x04;
                if(pkt->data.play.clientbound.player_position_and_look.pitch_is_relative)   flags = flags | 0x08;
                if(pkt->data.play.clientbound.player_position_and_look.yaw_is_relative)     flags = flags | 0x10;
                ssize_t bytes_written_7 = mcpr_encode_byte(bufpointer, flags);
                if(bytes_written_7 < 0) { free(*buf); return false; }
                bufpointer += bytes_written_7;

                ssize_t bytes_written_8 = mcpr_encode_varint(bufpointer, pkt->data.play.clientbound.player_position_and_look.teleport_id);
                if(bytes_written_8 < 0) { free(*buf); return false; }
                bufpointer += bytes_written_8;

                *out_bytes_written = bufpointer - *buf;
                return true;
            }

            case MCPR_PKT_PL_SB_KEEP_ALIVE:
            {
                *buf = malloc(10);
                if(*buf == NULL) { ninerr_set_err(ninerr_from_errno()); return false; }
                void *bufpointer = *buf;

                ssize_t bytes_written_1 = mcpr_encode_varint(bufpointer, mcpr_packet_type_to_byte(MCPR_PKT_PL_SB_KEEP_ALIVE));
                if(bytes_written_1 < 0) { free(*buf); return false; }
                bufpointer += bytes_written_1;

                ssize_t bytes_written_2 = mcpr_encode_varint(*buf, pkt->data.play.serverbound.keep_alive.keep_alive_id);
                if(bytes_written_2 < 0) { free(*buf); return false; }

                *out_bytes_written = bufpointer - *buf;
                return true;
            }

            case MCPR_PKT_PL_SB_PLUGIN_MESSAGE:
            {
                *buf = malloc(10+ strlen(pkt->data.play.serverbound.plugin_message.channel) + pkt->data.play.serverbound.plugin_message.data_length);
                if(*buf == NULL) { ninerr_set_err(ninerr_from_errno()); return false; }
                void *bufpointer = *buf;

                ssize_t bytes_written_1 = mcpr_encode_varint(bufpointer, mcpr_packet_type_to_byte(MCPR_PKT_PL_SB_PLUGIN_MESSAGE));
                if(bytes_written_1 < 0) { free(*buf); return false; }
                bufpointer += bytes_written_1;

                ssize_t bytes_written_2 = mcpr_encode_string(bufpointer, pkt->data.play.serverbound.plugin_message.channel);
                if(bytes_written_2 < 0) { free(*buf); return false; }
                bufpointer += bytes_written_2;

                memcpy(bufpointer, pkt->data.play.serverbound.plugin_message.data, pkt->data.play.serverbound.plugin_message.data_length);
                bufpointer += pkt->data.play.serverbound.plugin_message.data_length;

                *out_bytes_written = bufpointer - *buf;
                return true;
            }
        }
    }
    END_IGNORE()

    // Shouldn't be reached anyway

    fprintf(stderr, "Packet encoding for packet with id %i is not implemented yet! (state: %s)\n", pkt->id, mcpr_state_to_string(pkt->state));
    exit(EXIT_FAILURE);
}

ssize_t mcpr_decode_packet(struct mcpr_packet **out, const void *in, enum mcpr_state state, size_t maxlen)
{
    const void *ptr = in;
    size_t len_left = maxlen;
    int32_t packet_id;
    ssize_t bytes_read_1 = mcpr_decode_varint(&packet_id, ptr, len_left);
    if(bytes_read_1 < 0) return -1;
    len_left -= bytes_read_1;
    ptr += bytes_read_1;

    *out = malloc(sizeof(struct mcpr_packet));
    if(*out == NULL) { ninerr_set_err(ninerr_from_errno()); return -1; }
    struct mcpr_packet *pkt = *out;
    if(!mcpr_get_packet_type(&(pkt->id), packet_id, state)) { ninerr_set_err(ninerr_new("Invalid packet id %i", packet_id)); return -1; }

    IGNORE("-Wswitch")
    switch(state)
    {
        case MCPR_STATE_HANDSHAKE:
        {
            ssize_t bytes_read_2 = mcpr_decode_varint(&(pkt->data.handshake.serverbound.handshake.protocol_version), ptr, len_left);
            if(bytes_read_2 < 0) { free(pkt); return -1; }
            len_left -= bytes_read_2;
            ptr += bytes_read_2;

            // Dont forget to free server_address
            ssize_t bytes_read_3 = mcpr_decode_string(&(pkt->data.handshake.serverbound.handshake.server_address), ptr, len_left);
            if(bytes_read_3 < 0) { free(pkt); return -1; }
            len_left -= bytes_read_3;
            ptr += bytes_read_3;

            if(len_left < MCPR_USHORT_SIZE) { free(pkt->data.handshake.serverbound.handshake.server_address); free(pkt); ninerr_set_err(ninerr_new("Max packet length exceeded.")); return false; }
            ssize_t bytes_read_4 = mcpr_decode_ushort(&(pkt->data.handshake.serverbound.handshake.server_port), ptr);
            if(bytes_read_4 < 0) { free(pkt->data.handshake.serverbound.handshake.server_address); free(pkt); return -1; }
            len_left -= bytes_read_4;
            ptr += bytes_read_4;

            int32_t next_state;
            ssize_t bytes_read_5 = mcpr_decode_varint(&next_state, ptr, len_left);
            if(bytes_read_5 < 0) { free(pkt->data.handshake.serverbound.handshake.server_address); free(pkt); return -1;  }
            if(next_state != 1 && next_state != 2) { free(pkt->data.handshake.serverbound.handshake.server_address); free(pkt); ninerr_set_err(ninerr_new("Received invalid next state %i in handshake packet.", next_state)); return false; }
            pkt->data.handshake.serverbound.handshake.next_state = (next_state == 1) ? MCPR_STATE_STATUS : MCPR_STATE_LOGIN;
            ptr += bytes_read_5;

            return ptr - in;
        }

        case MCPR_STATE_LOGIN:
        {
            switch(pkt->id)
            {
                case MCPR_PKT_LG_SB_LOGIN_START:
                {
                    ssize_t bytes_read_2 = mcpr_decode_string(&(pkt->data.login.serverbound.login_start.name), ptr, len_left);
                    if(bytes_read_2 < 0) { free(pkt); return -1; }
                    return bytes_read_2 + bytes_read_1;
                }

                case MCPR_PKT_LG_SB_ENCRYPTION_RESPONSE:
                {
                    int32_t shared_secret_length;
                    ssize_t bytes_read_2 = mcpr_decode_varint(&shared_secret_length, ptr, len_left);
                    if(bytes_read_2 < 0) { free(pkt); return -1; }
                    if(shared_secret_length < 0) { ninerr_set_err(ninerr_new("Read invalid shared secret length (" PRId32 ").", shared_secret_length)); free(pkt); return -1; }
                    IGNORE("-Wtype-limits")
                    if((uint32_t) shared_secret_length > SIZE_MAX) { ninerr_set_err(ninerr_arithmetic_new()); free(pkt); return -1; }
                    END_IGNORE()
                    pkt->data.login.serverbound.encryption_response.shared_secret_length = shared_secret_length;
                    ptr += bytes_read_2;
                    len_left -= bytes_read_2;

                    pkt->data.login.serverbound.encryption_response.shared_secret = malloc(shared_secret_length);
                    if(pkt->data.login.serverbound.encryption_response.shared_secret == NULL) { ninerr_set_err(ninerr_from_errno()); free(pkt); return -1; }
                    memcpy(pkt->data.login.serverbound.encryption_response.shared_secret, ptr, shared_secret_length);
                    ptr += shared_secret_length;
                    len_left -= shared_secret_length;

                    int32_t verify_token_length;
                    ssize_t bytes_read_3 = mcpr_decode_varint(&verify_token_length, ptr, len_left);
                    if(bytes_read_3 < 0) { free(pkt); return -1; }
                    if(verify_token_length < 0) { ninerr_set_err(ninerr_new("Read invalid verify token length (" PRId32 ").", shared_secret_length)); free(pkt); return -1; }
                    IGNORE("-Wtype-limits")
                    if((uint32_t) verify_token_length > SIZE_MAX) { ninerr_set_err(ninerr_arithmetic_new()); free(pkt); return -1; }
                    END_IGNORE()
                    pkt->data.login.serverbound.encryption_response.verify_token_length = verify_token_length;
                    ptr += bytes_read_2;
                    len_left -= bytes_read_2;

                    pkt->data.login.serverbound.encryption_response.verify_token = malloc(verify_token_length);
                    if(pkt->data.login.serverbound.encryption_response.verify_token == NULL) { ninerr_set_err(ninerr_from_errno()); free(pkt); return -1; }
                    memcpy(pkt->data.login.serverbound.encryption_response.verify_token, ptr, verify_token_length);
                    ptr += verify_token_length;
                    len_left -= verify_token_length;

                    return ptr - in;
                }
            }
        }

        case MCPR_STATE_STATUS:
        {
            switch(pkt->id)
            {
                case MCPR_PKT_ST_SB_PING:
                {
                    if(len_left < MCPR_LONG_SIZE) { ninerr_set_err(ninerr_new("Max packet length exceeded.")); free(pkt); return -1; }
                    ssize_t bytes_read_2 = mcpr_decode_long(&(pkt->data.status.serverbound.ping.payload), ptr);
                    if(bytes_read_2 < 0) { free(pkt); return -1; }
                    return bytes_read_2;
                }

                case MCPR_PKT_ST_SB_REQUEST:
                {
                    return bytes_read_1;
                }
            }
        }
    }
    END_IGNORE()

    // Shouldn't be reached anyway
    abort();
}

bool mcpr_get_packet_type(enum mcpr_packet_type *out, uint8_t id, enum mcpr_state state)
{
    // only serverbound packets for now
    switch(state)
    {
        case MCPR_STATE_HANDSHAKE: *out = 0x00; return true;

        case MCPR_STATE_LOGIN:
        {
            switch(id)
            {
                case 0x00: *out = MCPR_PKT_LG_SB_LOGIN_START;           return true;
                case 0x01: *out = MCPR_PKT_LG_SB_ENCRYPTION_RESPONSE;   return true;
                default: return false;
            }
        }

        case MCPR_STATE_STATUS:
        {
            switch(id)
            {
                case 0x00: *out = MCPR_PKT_ST_SB_REQUEST;   return true;
                case 0x01: *out = MCPR_PKT_ST_SB_PING;      return true;
                default: return false;
            }
        }

        case MCPR_STATE_PLAY:
        {
            switch(id)
            {
                case 0x00: *out = MCPR_PKT_PL_SB_TELEPORT_CONFIRM;          return true;
                case 0x01: *out = MCPR_PKT_PL_SB_PREPARE_CRAFTING_GRID;     return true;
                case 0x02: *out = MCPR_PKT_PL_SB_TAB_COMPLETE;              return true;
                case 0x03: *out = MCPR_PKT_PL_SB_CHAT_MESSAGE;              return true;
                case 0x04: *out = MCPR_PKT_PL_SB_CLIENT_STATUS;             return true;
                case 0x05: *out = MCPR_PKT_PL_SB_CLIENT_SETTINGS;           return true;
                case 0x06: *out = MCPR_PKT_PL_SB_CONFIRM_TRANSACTION;       return true;
                case 0x07: *out = MCPR_PKT_PL_SB_ENCHANT_ITEM;              return true;
                case 0x08: *out = MCPR_PKT_PL_SB_CLICK_WINDOW;              return true;
                case 0x09: *out = MCPR_PKT_PL_SB_CLOSE_WINDOW;              return true;
                case 0x0A: *out = MCPR_PKT_PL_SB_PLUGIN_MESSAGE;            return true;
                case 0x0B: *out = MCPR_PKT_PL_SB_USE_ENTITY;                return true;
                case 0x0C: *out = MCPR_PKT_PL_SB_KEEP_ALIVE;                return true;
                case 0x0D: *out = MCPR_PKT_PL_SB_PLAYER;                    return true;
                case 0x0E: *out = MCPR_PKT_PL_SB_PLAYER_POSITION;           return true;
                case 0x0F: *out = MCPR_PKT_PL_SB_PLAYER_POSITION_AND_LOOK;  return true;
                case 0x10: *out = MCPR_PKT_PL_SB_PLAYER_LOOK;               return true;
                case 0x11: *out = MCPR_PKT_PL_SB_VEHICLE_MOVE;              return true;
                case 0x12: *out = MCPR_PKT_PL_SB_STEER_BOAT;                return true;
                case 0x13: *out = MCPR_PKT_PL_SB_PLAYER_ABILITIES;          return true;
                case 0x14: *out = MCPR_PKT_PL_SB_PLAYER_DIGGING;            return true;
                case 0x15: *out = MCPR_PKT_PL_SB_ENTITY_ACTION;             return true;
                case 0x16: *out = MCPR_PKT_PL_SB_STEER_VEHICLE;             return true;
                case 0x17: *out = MCPR_PKT_PL_SB_CRAFTING_BOOK_DATA;        return true;
                case 0x18: *out = MCPR_PKT_PL_SB_RESOURCE_PACK_STATUS;      return true;
                case 0x19: *out = MCPR_PKT_PL_SB_ADVANCEMENT_TAB;           return true;
                case 0x1A: *out = MCPR_PKT_PL_SB_HELD_ITEM_CHANGE;          return true;
                case 0x1B: *out = MCPR_PKT_PL_SB_CREATIVE_INVENTORY_ACTION; return true;
                case 0x1C: *out = MCPR_PKT_PL_SB_UPDATE_SIGN;               return true;
                case 0x1D: *out = MCPR_PKT_PL_SB_ANIMATION;                 return true;
                case 0x1E: *out = MCPR_PKT_PL_SB_SPECTATE;                  return true;
                case 0x1F: *out = MCPR_PKT_PL_SB_PLAYER_BLOCK_PLACEMENT;    return true;
                case 0X20: *out = MCPR_PKT_PL_SB_USE_ITEM;                  return true;
                default: return false;
            }
        }
    }
    return false; // Won't even be reached, but else the compiler will complain.
}

uint8_t mcpr_packet_type_to_byte(enum mcpr_packet_type id)
{
    // only serverbound packets for now
    switch(id)
    {
        case MCPR_PKT_ST_CB_PONG:                       return 0x00;
        case MCPR_PKT_ST_CB_RESPONSE:                   return 0x00;

        case MCPR_PKT_LG_CB_DISCONNECT:                 return 0x00;
        case MCPR_PKT_LG_CB_ENCRYPTION_REQUEST:         return 0x01;
        case MCPR_PKT_LG_CB_LOGIN_SUCCESS:              return 0x02;
        case MCPR_PKT_LG_CB_SET_COMPRESSION:            return 0x03;

        case MCPR_PKT_PL_CB_DISCONNECT:                 return 0x1A;
        case MCPR_PKT_PL_CB_KEEP_ALIVE:                 return 0x1F;
        case MCPR_PKT_PL_CB_JOIN_GAME:                  return 0x23;
        case MCPR_PKT_PL_CB_PLUGIN_MESSAGE:             return 0x0A;
        case MCPR_PKT_PL_CB_SPAWN_POSITION:             return 0x45;
        case MCPR_PKT_PL_CB_PLAYER_ABILITIES:           return 0x13;
        case MCPR_PKT_PL_CB_PLAYER_POSITION_AND_LOOK:   return 0x2E;
        default: fprintf(stderr, "Unimplemented packet id given in mcpr_packet_type_to_byte().. Aborting at mcpr.c:%i", __LINE__); abort();
    }
    return 0; // Won't even be reached, but else the compiler will complain
}

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

#include <nbt/nbt.h>

#include <mcpr/mcpr.h>
#include <mcpr/codec.h>
#include <mcpr/packet.h>
#include "util.h"

#include <ninuuid/ninuuid.h>

#include "internal.h"

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
    DEBUG_PRINT("In mcpr_encode_packet, numerical packet ID: 0x%02x, state: %s\n", mcpr_packet_type_to_byte(pkt->id), mcpr_state_to_string(pkt->state));
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

                    bufpointer += mcpr_encode_varint(bufpointer, mcpr_packet_type_to_byte(MCPR_PKT_HS_SB_HANDSHAKE));
                    bufpointer += mcpr_encode_varint(bufpointer, MCPR_PROTOCOL_VERSION);

                    ssize_t bytes_written_3 = mcpr_encode_string(bufpointer, pkt->data.handshake.serverbound.handshake.server_address);
                    if(bytes_written_3 < 0) { free(*buf); return false; }
                    bufpointer += bytes_written_3;

                    mcpr_encode_ushort(bufpointer, pkt->data.handshake.serverbound.handshake.server_port); bufpointer += MCPR_USHORT_SIZE;
                    bufpointer += mcpr_encode_varint(bufpointer, pkt->data.handshake.serverbound.handshake.next_state);

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

                    bufpointer += mcpr_encode_varint(bufpointer, mcpr_packet_type_to_byte(MCPR_PKT_ST_CB_PONG));
                    mcpr_encode_long(bufpointer, pkt->data.status.clientbound.pong.payload); bufpointer += MCPR_LONG_SIZE;

                    *out_bytes_written = bufpointer - *buf;
                    return true;
                }

                case MCPR_PKT_ST_CB_RESPONSE:
                {
                    char *response = server_list_response_to_json(pkt);
                    if(response == NULL) { return false; }
                    DEBUG_PRINT("Response: %s\n", response);
                    size_t len = strlen(response);

                    *buf = malloc(MCPR_VARINT_SIZE_MAX + MCPR_VARINT_SIZE_MAX + len);
                    if(*buf == NULL) { ninerr_set_err(ninerr_from_errno()); free(*buf); return false; }
                    void *bufpointer = *buf;

                    bufpointer += mcpr_encode_varint(bufpointer, mcpr_packet_type_to_byte(MCPR_PKT_ST_CB_RESPONSE));

                    bufpointer += mcpr_encode_varint(bufpointer, len);
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

                    bufpointer += mcpr_encode_varint(bufpointer, mcpr_packet_type_to_byte(MCPR_PKT_LG_CB_DISCONNECT));

                    ssize_t bytes_written_2 = mcpr_encode_chat(bufpointer, pkt->data.login.clientbound.disconnect.reason);
                    if(bytes_written_2 < 0) { free(*buf); return false; }
                    bufpointer += bytes_written_2;

                    *out_bytes_written = bufpointer - *buf;
                    return true;
                }

                case MCPR_PKT_LG_CB_ENCRYPTION_REQUEST:
                {
                    *buf = malloc(20 + strlen(pkt->data.login.clientbound.encryption_request.server_id) + pkt->data.login.clientbound.encryption_request.public_key_length + pkt->data.login.clientbound.encryption_request.verify_token_length);
                    if(*buf == NULL) { ninerr_set_err(ninerr_from_errno()); return false; }
                    void *bufpointer = *buf;

                    bufpointer += mcpr_encode_varint(bufpointer, mcpr_packet_type_to_byte(MCPR_PKT_LG_CB_ENCRYPTION_REQUEST));
                    ssize_t bytes_written_2 = mcpr_encode_string(bufpointer, pkt->data.login.clientbound.encryption_request.server_id);
                    if(bytes_written_2 < 0) { free(*buf); return false; }
                    bufpointer += bytes_written_2;
                    bufpointer += mcpr_encode_varint(bufpointer, pkt->data.login.clientbound.encryption_request.public_key_length);
                    memcpy(bufpointer, pkt->data.login.clientbound.encryption_request.public_key, pkt->data.login.clientbound.encryption_request.public_key_length);
                    bufpointer += pkt->data.login.clientbound.encryption_request.public_key_length;
                    bufpointer += mcpr_encode_varint(bufpointer, pkt->data.login.clientbound.encryption_request.verify_token_length);
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

                    bufpointer += mcpr_encode_varint(bufpointer, mcpr_packet_type_to_byte(MCPR_PKT_LG_CB_LOGIN_SUCCESS));

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

                    bufpointer += mcpr_encode_varint(bufpointer, mcpr_packet_type_to_byte(MCPR_PKT_LG_CB_SET_COMPRESSION));
                    bufpointer += mcpr_encode_varint(bufpointer, pkt->data.login.clientbound.set_compression.threshold);

                    *out_bytes_written = bufpointer - *buf;
                    return true;
                }

                case MCPR_PKT_LG_SB_LOGIN_START:
                {
                    *buf = malloc(10 + strlen(pkt->data.login.serverbound.login_start.name));
                    if(*buf == NULL) { ninerr_set_err(ninerr_from_errno()); return false; }
                    void *bufpointer = *buf;

                    bufpointer += mcpr_encode_varint(bufpointer, mcpr_packet_type_to_byte(MCPR_PKT_LG_SB_LOGIN_START));

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

                    bufpointer += mcpr_encode_varint(bufpointer, mcpr_packet_type_to_byte(MCPR_PKT_LG_SB_ENCRYPTION_RESPONSE));
                    bufpointer += mcpr_encode_varint(bufpointer, shared_secret_length);
                    memcpy(bufpointer, pkt->data.login.serverbound.encryption_response.shared_secret, shared_secret_length); bufpointer += shared_secret_length;
                    bufpointer += mcpr_encode_varint(bufpointer, verify_token_length);
                    memcpy(bufpointer, pkt->data.login.serverbound.encryption_response.verify_token, verify_token_length); bufpointer += verify_token_length;

                    *out_bytes_written = bufpointer - *buf;
                    return true;
                }
            }
        }

        case MCPR_STATE_PLAY:
        {
            switch(pkt->id)
            {
                case MCPR_PKT_PL_CB_DISCONNECT:
                {
                    const char *reason = pkt->data.play.clientbound.disconnect.reason; // It's JSON chat, not a normal string
                    *buf = malloc(10 + strlen(reason));
                    if(*buf == NULL) { ninerr_set_err(ninerr_from_errno()); return false; }
                    void *bufpointer = *buf;

                    bufpointer += mcpr_encode_varint(bufpointer, mcpr_packet_type_to_byte(MCPR_PKT_PL_CB_DISCONNECT));

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

                    bufpointer += mcpr_encode_varint(bufpointer, mcpr_packet_type_to_byte(MCPR_PKT_PL_CB_KEEP_ALIVE));
                    bufpointer += mcpr_encode_varint(*buf, pkt->data.play.clientbound.keep_alive.keep_alive_id);

                    *out_bytes_written = bufpointer - *buf;
                    return true;
                }

                case MCPR_PKT_PL_CB_JOIN_GAME:
                {
                    *buf = malloc(33);
                    if(*buf == NULL) { ninerr_set_err(ninerr_from_errno()); return false; }
                    void *bufpointer = *buf;

                    bufpointer += mcpr_encode_varint(bufpointer, mcpr_packet_type_to_byte(MCPR_PKT_PL_CB_JOIN_GAME));
                    mcpr_encode_int(bufpointer, pkt->data.play.clientbound.join_game.entity_id); bufpointer += MCPR_INT_SIZE;

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
                    mcpr_encode_ubyte(bufpointer, gamemode); bufpointer += MCPR_UBYTE_SIZE;

                    int32_t dimension;
                    switch(pkt->data.play.clientbound.join_game.dimension)
                    {
                        case MCPR_DIMENSION_NETHER:     dimension = -1; break;
                        case MCPR_DIMENSION_OVERWORLD:  dimension = 0;  break;
                        case MCPR_DIMENSION_END:        dimension = 1;  break;
                        default: abort(); return false; // Won't be reached, but else the compiler will complain.
                    }
                    mcpr_encode_int(bufpointer, dimension); bufpointer += MCPR_INT_SIZE;

                    uint8_t difficulty;
                    switch(pkt->data.play.clientbound.join_game.difficulty)
                    {
                        case MCPR_DIFFICULTY_PEACEFUL:  difficulty = 0; break;
                        case MCPR_DIFFICULTY_EASY:      difficulty = 1; break;
                        case MCPR_DIFFICULTY_NORMAL:    difficulty = 2; break;
                        case MCPR_DIFFICULTY_HARD:      difficulty = 3; break;
                        default: abort(); return false; // Won't be reached, but else the compiler will complain.
                    }
                    mcpr_encode_ubyte(bufpointer, difficulty); bufpointer += MCPR_UBYTE_SIZE;
                    mcpr_encode_ubyte(bufpointer, pkt->data.play.clientbound.join_game.max_players); bufpointer += MCPR_UBYTE_SIZE;

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

                    mcpr_encode_bool(bufpointer, pkt->data.play.clientbound.join_game.reduced_debug_info); bufpointer += MCPR_BOOL_SIZE;

                    *out_bytes_written = bufpointer - *buf;
                    return true;
                }

                case MCPR_PKT_PL_CB_PLUGIN_MESSAGE:
                {
                    size_t channel_len = strlen(pkt->data.play.clientbound.plugin_message.channel);
                    if(channel_len > 20) { DEBUG_PRINT("Error for clientbound plugin message packet! Channel string length is greater than 20."); abort(); }

                    *buf = malloc(10 + channel_len + pkt->data.play.clientbound.plugin_message.data_length);
                    if(*buf == NULL) { ninerr_set_err(ninerr_from_errno()); return false; }
                    void *bufpointer = *buf;

                    bufpointer += mcpr_encode_varint(bufpointer, mcpr_packet_type_to_byte(MCPR_PKT_PL_CB_PLUGIN_MESSAGE));

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

                    bufpointer += mcpr_encode_varint(bufpointer, mcpr_packet_type_to_byte(MCPR_PKT_PL_CB_SPAWN_POSITION));
                    mcpr_encode_position(bufpointer, &(pkt->data.play.clientbound.spawn_position.location)); bufpointer += MCPR_POSITION_SIZE;

                    *out_bytes_written = bufpointer - *buf;
                    return true;
                }

                case MCPR_PKT_PL_CB_PLAYER_ABILITIES:
                {
                    *buf = malloc(MCPR_VARINT_SIZE_MAX + MCPR_BYTE_SIZE + MCPR_FLOAT_SIZE + MCPR_FLOAT_SIZE);
                    if(*buf == NULL) { ninerr_set_err(ninerr_from_errno()); return false; }
                    void *bufpointer = *buf;

                    bufpointer += mcpr_encode_varint(bufpointer, mcpr_packet_type_to_byte(MCPR_PKT_PL_CB_PLAYER_ABILITIES));

                    uint8_t flags = 0;
                    if(pkt->data.play.clientbound.player_abilities.invulnerable)    flags |= 0x01;
                    if(pkt->data.play.clientbound.player_abilities.is_flying)       flags |= 0x02;
                    if(pkt->data.play.clientbound.player_abilities.allow_flying)    flags |= 0x04;
                    if(pkt->data.play.clientbound.player_abilities.creative_mode)   flags |= 0x08;
                    mcpr_encode_byte(bufpointer, flags); bufpointer += MCPR_BYTE_SIZE;

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

                    bufpointer += mcpr_encode_varint(bufpointer, mcpr_packet_type_to_byte(MCPR_PKT_PL_CB_PLAYER_POSITION_AND_LOOK));

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
                    if(pkt->data.play.clientbound.player_position_and_look.x_is_relative)       flags |= 0x01;
                    if(pkt->data.play.clientbound.player_position_and_look.y_is_relative)       flags |= 0x02;
                    if(pkt->data.play.clientbound.player_position_and_look.z_is_relative)       flags |= 0x04;
                    if(pkt->data.play.clientbound.player_position_and_look.pitch_is_relative)   flags |= 0x08;
                    if(pkt->data.play.clientbound.player_position_and_look.yaw_is_relative)     flags |= 0x10;
                    mcpr_encode_byte(bufpointer, flags);
                    bufpointer += MCPR_BYTE_SIZE;

                    bufpointer += mcpr_encode_varint(bufpointer, pkt->data.play.clientbound.player_position_and_look.teleport_id);

                    *out_bytes_written = bufpointer - *buf;
                    return true;
                }

                case MCPR_PKT_PL_SB_KEEP_ALIVE:
                {
                    *buf = malloc(10);
                    if(*buf == NULL) { ninerr_set_err(ninerr_from_errno()); return false; }
                    void *bufpointer = *buf;

                    bufpointer += mcpr_encode_varint(bufpointer, mcpr_packet_type_to_byte(MCPR_PKT_PL_SB_KEEP_ALIVE));
                    bufpointer += mcpr_encode_varint(*buf, pkt->data.play.serverbound.keep_alive.keep_alive_id);

                    *out_bytes_written = bufpointer - *buf;
                    return true;
                }

                case MCPR_PKT_PL_SB_PLUGIN_MESSAGE:
                {
                    *buf = malloc(10+ strlen(pkt->data.play.serverbound.plugin_message.channel) + pkt->data.play.serverbound.plugin_message.data_length);
                    if(*buf == NULL) { ninerr_set_err(ninerr_from_errno()); return false; }
                    void *bufpointer = *buf;

                    bufpointer += mcpr_encode_varint(bufpointer, mcpr_packet_type_to_byte(MCPR_PKT_PL_SB_PLUGIN_MESSAGE));

                    ssize_t bytes_written_2 = mcpr_encode_string(bufpointer, pkt->data.play.serverbound.plugin_message.channel);
                    if(bytes_written_2 < 0) { free(*buf); return false; }
                    bufpointer += bytes_written_2;

                    memcpy(bufpointer, pkt->data.play.serverbound.plugin_message.data, pkt->data.play.serverbound.plugin_message.data_length);
                    bufpointer += pkt->data.play.serverbound.plugin_message.data_length;

                    *out_bytes_written = bufpointer - *buf;
                    return true;
                }

                case MCPR_PKT_PL_CB_CHUNK_DATA:
                {
                    size_t raw_block_entities_size = 0;
                    struct buffer *raw_block_entities = NULL;
                    if(pkt->data.play.clientbound.chunk_data.block_entities != NULL)
                    {
                        struct buffer tmp = nbt_dump_binary(pkt->data.play.clientbound.chunk_data.block_entities);
                        raw_block_entities = &tmp;
                        raw_block_entities_size = tmp.len;
                    }

                    int32_t data_size = 0;
                    if(pkt->data.play.clientbound.chunk_data.ground_up_continuous) data_size += 256;
                    for(size_t i = 0; i < pkt->data.play.clientbound.chunk_data.size; i++) // TODO what if integer overflow occurs here.
                    {
                        struct mcpr_chunk_section *section = pkt->data.play.clientbound.chunk_data.chunk_sections + i;
                        data_size += MCPR_UBYTE_SIZE;
                        data_size += mcpr_varint_bounds(section->palette_length);
                        data_size += section->palette_length * MCPR_VARINT_SIZE_MAX;
                        data_size += mcpr_varint_bounds(section->block_array_length);
                        data_size += section->block_array_length * 8;
                        data_size += 2048; // block light, half a byte per block in 16x16x16 chunk section.
                        if(section->sky_light != NULL) data_size += 2048;// sky light, half a byte per block in 16x16x16 chunk section.
                    }

                    *buf = malloc(MCPR_INT_SIZE + MCPR_INT_SIZE + MCPR_BOOL_SIZE + MCPR_VARINT_SIZE_MAX + MCPR_VARINT_SIZE_MAX + data_size +
                         MCPR_VARINT_SIZE_MAX + raw_block_entities_size);
                    if(*buf == NULL) { ninerr_set_err(ninerr_from_errno()); return false; }
                    void *bufpointer = *buf;

                    bufpointer += mcpr_encode_varint(bufpointer, mcpr_packet_type_to_byte(MCPR_PKT_PL_CB_CHUNK_DATA));
                    mcpr_encode_int(bufpointer, pkt->data.play.clientbound.chunk_data.chunk_x); bufpointer += MCPR_INT_SIZE;
                    mcpr_encode_int(bufpointer, pkt->data.play.clientbound.chunk_data.chunk_z); bufpointer += MCPR_INT_SIZE;
                    mcpr_encode_bool(bufpointer, pkt->data.play.clientbound.chunk_data.ground_up_continuous); bufpointer += MCPR_BOOL_SIZE;
                    bufpointer += mcpr_encode_varint(bufpointer, pkt->data.play.clientbound.chunk_data.primary_bit_mask);
                    bufpointer += mcpr_encode_varint(bufpointer, data_size);

                    for(size_t i = 0; i < pkt->data.play.clientbound.chunk_data.size; i++)
                    {
                        struct mcpr_chunk_section *section = pkt->data.play.clientbound.chunk_data.chunk_sections + i;

                        mcpr_encode_ubyte(bufpointer, section->bits_per_block); bufpointer += MCPR_UBYTE_SIZE;
                        bufpointer += mcpr_encode_varint(bufpointer, section->palette_length);

                        if(section->palette_length > 0)
                        {
                            if((uint32_t) section->palette_length > SIZE_MAX) { free(*buf); ninerr_set_err(ninerr_new("palette length of mcpr_chunk_section does not fit within size_t type.")); return false; }
                            for(size_t j = 0; j < (size_t) section->palette_length; j++)
                            {
                                bufpointer += mcpr_encode_varint(bufpointer, section->palette[j]);
                            }
                        }
                        bufpointer += mcpr_encode_varint(bufpointer, section->block_array_length);
                        memcpy(bufpointer, section->blocks, section->block_array_length * sizeof(uint64_t)); bufpointer += section->block_array_length * sizeof(uint64_t);
                        memcpy(bufpointer, section->block_light, 2048); bufpointer += 2048;
                        if(section->sky_light != NULL)
                        {
                            memcpy(bufpointer, section->sky_light, 2048); bufpointer += 2048;
                        }
                    }
                    if(pkt->data.play.clientbound.chunk_data.ground_up_continuous)
                    {
                        memcpy(bufpointer, pkt->data.play.clientbound.chunk_data.biomes, 256); bufpointer += 256;
                    }
                    bufpointer += mcpr_encode_varint(bufpointer, pkt->data.play.clientbound.chunk_data.block_entity_count);
                    if(pkt->data.play.clientbound.chunk_data.block_entity_count > 0)
                    {
                        memcpy(bufpointer, raw_block_entities->data, raw_block_entities->len); bufpointer += raw_block_entities->len;
                    }

                    if(raw_block_entities != NULL) buffer_free(raw_block_entities);
                    DEBUG_PRINT("Size of raw chunk data packet: %zu", bufpointer - *buf)
                    *out_bytes_written = bufpointer - *buf;
                    return true;
                }
            }
        }
    }
    END_IGNORE()

    // Shouldn't be reached anyway

    fprintf(stderr, "Packet encoding for packet with id %i is not implemented yet! (state: %s)\n", pkt->id, mcpr_state_to_string(pkt->state));
    abort();
}

ssize_t mcpr_decode_packet(struct mcpr_packet **out, const void *in, enum mcpr_state state, size_t maxlen)
{
    DEBUG_PRINT("in mcpr_decode_packet(state=%s, maxlen=%zu)", mcpr_state_to_string(state), maxlen);

    const void *ptr = in;
    size_t len_left = maxlen;
    int32_t packet_id;
    ssize_t bytes_read_1 = mcpr_decode_varint(&packet_id, ptr, len_left);
    if(bytes_read_1 < 0) return -1;
    len_left -= bytes_read_1;
    ptr += bytes_read_1;
    DEBUG_PRINT("Decoding packet with id %ld", packet_id);

    *out = malloc(sizeof(struct mcpr_packet));
    if(*out == NULL) { ninerr_set_err(ninerr_from_errno()); return -1; }
    struct mcpr_packet *pkt = *out;
    if(!mcpr_get_packet_type(&(pkt->id), packet_id, state)) { ninerr_set_err(ninerr_new("Invalid packet id %ld", (long) packet_id)); return -1; }

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
            mcpr_decode_ushort(&(pkt->data.handshake.serverbound.handshake.server_port), ptr);
            len_left -= MCPR_USHORT_SIZE;
            ptr += MCPR_USHORT_SIZE;

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
                    mcpr_decode_long(&(pkt->data.status.serverbound.ping.payload), ptr);
                    return MCPR_LONG_SIZE + bytes_read_1;
                }

                case MCPR_PKT_ST_SB_REQUEST:
                {
                    return bytes_read_1;
                }
            }
        }

        case MCPR_STATE_PLAY:
        {
            switch(pkt->id)
            {
                case MCPR_PKT_PL_SB_CLIENT_SETTINGS:
                {
                    ssize_t bytes_read_2 = mcpr_decode_string(&(pkt->data.play.serverbound.client_settings.locale), ptr, len_left);
                    if(bytes_read_2 < 0) { free(pkt); return -1; }
                    ptr += bytes_read_2;
                    len_left -= bytes_read_2;

                    if(len_left < 1) { ninerr_set_err(ninerr_new("Max decoding length exceeded.")); free(pkt); free(pkt->data.play.serverbound.client_settings.locale); return -1; }
                    mcpr_decode_byte(&(pkt->data.play.serverbound.client_settings.view_distance), ptr);
                    ptr += MCPR_BYTE_SIZE;
                    len_left -= MCPR_BYTE_SIZE;

                    int32_t chat_mode_int;
                    ssize_t bytes_read_4 = mcpr_decode_varint(&chat_mode_int, ptr, len_left);
                    if(bytes_read_4 < 0) { free(pkt); free(pkt->data.play.serverbound.client_settings.locale); return -1; }
                    switch(chat_mode_int)
                    {
                        case 0: pkt->data.play.serverbound.client_settings.chat_mode = MCPR_CHAT_MODE_ENABLED; break;
                        case 1: pkt->data.play.serverbound.client_settings.chat_mode = MCPR_CHAT_MODE_COMMANDS_ONLY; break;
                        case 2: pkt->data.play.serverbound.client_settings.chat_mode = MCPR_CHAT_MODE_HIDDEN; break;
                        default: ninerr_set_err(ninerr_new("Invalid value %ld for chat mode in client settings packet.", (long) chat_mode_int)); free(pkt); free(pkt->data.play.serverbound.client_settings.locale); return -1;
                    }
                    ptr += bytes_read_4;
                    len_left -= bytes_read_4;

                    if(len_left < MCPR_BOOL_SIZE) { ninerr_set_err(ninerr_new("Max packet length exceeded.")); free(pkt); free(pkt->data.play.serverbound.client_settings.locale); return -1; }
                    mcpr_decode_bool(&(pkt->data.play.serverbound.client_settings.chat_colors), ptr);
                    ptr += MCPR_BOOL_SIZE;
                    len_left -= MCPR_BOOL_SIZE;

                    if(len_left < 1) { ninerr_set_err(ninerr_new("Max packet length exceeded.")); free(pkt); free(pkt->data.play.serverbound.client_settings.locale); return -1; }
                    uint8_t displayed_skin_parts;
                    mcpr_decode_ubyte(&displayed_skin_parts, ptr);
                    pkt->data.play.serverbound.client_settings.displayed_skin_parts.cape_enabled = displayed_skin_parts & 0x01;
                    pkt->data.play.serverbound.client_settings.displayed_skin_parts.jacket_enabled = displayed_skin_parts & 0x02;
                    pkt->data.play.serverbound.client_settings.displayed_skin_parts.left_sleeve_enabled = displayed_skin_parts & 0x04;
                    pkt->data.play.serverbound.client_settings.displayed_skin_parts.right_sleeve_enabled = displayed_skin_parts & 0x08;
                    pkt->data.play.serverbound.client_settings.displayed_skin_parts.left_pants_enabled = displayed_skin_parts & 0x10;
                    pkt->data.play.serverbound.client_settings.displayed_skin_parts.right_pants_enabled = displayed_skin_parts & 0x20;
                    pkt->data.play.serverbound.client_settings.displayed_skin_parts.hat_enabled = displayed_skin_parts & 0x40;
                    ptr += MCPR_UBYTE_SIZE;
                    len_left -= MCPR_UBYTE_SIZE;

                    int32_t main_hand;
                    ssize_t bytes_read_7 = mcpr_decode_varint(&main_hand, ptr, len_left);
                    if(bytes_read_7 < 0) { free(pkt); free(pkt->data.play.serverbound.client_settings.locale); return -1; }
                    switch(main_hand)
                    {
                        case 0: pkt->data.play.serverbound.client_settings.main_hand = MCPR_HAND_LEFT; break;
                        case 1: pkt->data.play.serverbound.client_settings.main_hand = MCPR_HAND_RIGHT; break;
                        default: ninerr_set_err(ninerr_new("Invalid value %ld for main hand in client settings packet. Expected either 0 for left hand or 1 for right hand.",
                                    (long) main_hand)); free(pkt); free(pkt->data.play.serverbound.client_settings.locale); return -1;
                    }
                    ptr += bytes_read_7;
                    len_left -= bytes_read_7;

                    return ptr - in;
                }

                case MCPR_PKT_PL_SB_PLUGIN_MESSAGE:
                {
                    ssize_t bytes_read_2 = mcpr_decode_string(&(pkt->data.play.serverbound.plugin_message.channel), ptr, len_left);
                    if(bytes_read_2 < 0) { free(pkt); return -1; }
                    ptr += bytes_read_2;
                    len_left -= bytes_read_2;

                    pkt->data.play.serverbound.plugin_message.data = malloc(len_left);
                    if(pkt->data.play.serverbound.plugin_message.data == NULL) { free(pkt->data.play.serverbound.plugin_message.channel); free(pkt); ninerr_set_err(ninerr_from_errno()); return -1; }
                    memcpy(pkt->data.play.serverbound.plugin_message.data, ptr, len_left);
                    ptr += len_left;
                    return ptr - in;
                }

                case MCPR_PKT_PL_SB_KEEP_ALIVE:
                {
                    ssize_t bytes_read_2 = mcpr_decode_varint(&(pkt->data.play.serverbound.keep_alive.keep_alive_id), ptr, len_left);
                    if(bytes_read_2 < 0) { free(pkt); return -1; }
                    ptr += bytes_read_2;
                    return ptr - in;
                }

                case MCPR_PKT_PL_SB_PLAYER_POSITION:
                {
                    if(len_left < MCPR_DOUBLE_SIZE) { free(pkt); ninerr_set_err(ninerr_new("Not enough length left.")); return -1; }
                    ssize_t bytes_read_2 = mcpr_decode_double(&(pkt->data.play.serverbound.player_position.x), ptr);
                    if(bytes_read_2 < 0) { free(pkt); return -1; }
                    ptr += bytes_read_2;
                    len_left -= bytes_read_2;

                    if(len_left < MCPR_DOUBLE_SIZE) { free(pkt); ninerr_set_err(ninerr_new("Not enough length left.")); return -1; }
                    ssize_t bytes_read_3 = mcpr_decode_double(&(pkt->data.play.serverbound.player_position.feet_y), ptr);
                    if(bytes_read_3 < 0) { free(pkt); return -1; }
                    ptr += bytes_read_3;
                    len_left -= bytes_read_3;

                    if(len_left < MCPR_DOUBLE_SIZE) { free(pkt); ninerr_set_err(ninerr_new("Not enough length left.")); return -1; }
                    ssize_t bytes_read_4 = mcpr_decode_double(&(pkt->data.play.serverbound.player_position.z), ptr);
                    if(bytes_read_4 < 0) { free(pkt); return -1; }
                    ptr += bytes_read_4;
                    len_left -= bytes_read_4;

                    if(len_left < MCPR_BOOL_SIZE) { free(pkt); ninerr_set_err(ninerr_new("Not enough length left.")); return -1; }
                    mcpr_decode_bool(&(pkt->data.play.serverbound.player_position.on_ground), ptr);
                    ptr += MCPR_BOOL_SIZE;

                    return ptr - in;
                }

                case MCPR_PKT_PL_SB_PLAYER_POSITION_AND_LOOK:
                {
                    if(len_left < MCPR_DOUBLE_SIZE) { free(pkt); ninerr_set_err(ninerr_new("Not enough length left.")); return -1; }
                    ssize_t bytes_read_2 = mcpr_decode_double(&(pkt->data.play.serverbound.player_position.x), ptr);
                    if(bytes_read_2 < 0) { free(pkt); return -1; }
                    ptr += bytes_read_2;
                    len_left -= bytes_read_2;

                    if(len_left < MCPR_DOUBLE_SIZE) { free(pkt); ninerr_set_err(ninerr_new("Not enough length left.")); return -1; }
                    ssize_t bytes_read_3 = mcpr_decode_double(&(pkt->data.play.serverbound.player_position.feet_y), ptr);
                    if(bytes_read_3 < 0) { free(pkt); return -1; }
                    ptr += bytes_read_3;
                    len_left -= bytes_read_3;

                    if(len_left < MCPR_DOUBLE_SIZE) { free(pkt); ninerr_set_err(ninerr_new("Not enough length left.")); return -1; }
                    ssize_t bytes_read_4 = mcpr_decode_double(&(pkt->data.play.serverbound.player_position.z), ptr);
                    if(bytes_read_4 < 0) { free(pkt); return -1; }
                    ptr += bytes_read_4;
                    len_left -= bytes_read_4;

                    if(len_left < MCPR_FLOAT_SIZE) { free(pkt); ninerr_set_err(ninerr_new("Not enough length left.")); return -1; }
                    ssize_t bytes_read_5 = mcpr_decode_float(&(pkt->data.play.serverbound.player_position.yaw), ptr);
                    if(bytes_read_5 < 0) { free(pkt); return -1; }
                    ptr += bytes_read_5;
                    len_left -= bytes_read_5;

                    if(len_left < MCPR_FLOAT_SIZE) { free(pkt); ninerr_set_err(ninerr_new("Not enough length left.")); return -1; }
                    ssize_t bytes_read_6 = mcpr_decode_float(&(pkt->data.play.serverbound.player_position.pitch), ptr);
                    if(bytes_read_6 < 0) { free(pkt); return -1; }
                    ptr += bytes_read_6;
                    len_left -= bytes_read_6;

                    if(len_left < MCPR_BOOL_SIZE) { free(pkt); ninerr_set_err(ninerr_new("Not enough length left.")); return -1; }
                    mcpr_decode_bool(&(pkt->data.play.serverbound.player_position.on_ground), ptr);
                    ptr += MCPR_BOOL_SIZE;

                    return ptr - in;
                }
            }
        }
    }
    END_IGNORE()

    // Shouldn't be reached anyway
    DEBUG_PRINT("Decoding for packet is not implemented yet! (state: %s)", mcpr_state_to_string(state));
    free(*out);
    return -1;
}

// Last updated for 1.12.1 protocol
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
                case 0x01: *out = MCPR_PKT_PL_SB_TAB_COMPLETE;              return true;
                case 0x02: *out = MCPR_PKT_PL_SB_CHAT_MESSAGE;              return true;
                case 0x03: *out = MCPR_PKT_PL_SB_CLIENT_STATUS;             return true;
                case 0x04: *out = MCPR_PKT_PL_SB_CLIENT_SETTINGS;           return true;
                case 0x05: *out = MCPR_PKT_PL_SB_CONFIRM_TRANSACTION;       return true;
                case 0x06: *out = MCPR_PKT_PL_SB_ENCHANT_ITEM;              return true;
                case 0x07: *out = MCPR_PKT_PL_SB_CLICK_WINDOW;              return true;
                case 0x08: *out = MCPR_PKT_PL_SB_CLOSE_WINDOW;              return true;
                case 0x09: *out = MCPR_PKT_PL_SB_PLUGIN_MESSAGE;            return true;
                case 0x0A: *out = MCPR_PKT_PL_SB_USE_ENTITY;                return true;
                case 0x0B: *out = MCPR_PKT_PL_SB_KEEP_ALIVE;                return true;
                case 0x0C: *out = MCPR_PKT_PL_SB_PLAYER;                    return true;
                case 0x0D: *out = MCPR_PKT_PL_SB_PLAYER_POSITION;           return true;
                case 0x0E: *out = MCPR_PKT_PL_SB_PLAYER_POSITION_AND_LOOK;  return true;
                case 0x0F: *out = MCPR_PKT_PL_SB_PLAYER_LOOK;               return true;
                case 0x10: *out = MCPR_PKT_PL_SB_VEHICLE_MOVE;              return true;
                case 0x11: *out = MCPR_PKT_PL_SB_STEER_BOAT;                return true;
                case 0x12: *out = MCPR_PKT_PL_SB_CRAFT_RECIPE_REQUEST;      return true;
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

// Last update was for 1.12.1
uint8_t mcpr_packet_type_to_byte(enum mcpr_packet_type id)
{
    switch(id)
    {
        case MCPR_PKT_ST_CB_RESPONSE:                   return 0x00;
        case MCPR_PKT_ST_CB_PONG:                       return 0x01;

        case MCPR_PKT_LG_CB_DISCONNECT:                 return 0x00;
        case MCPR_PKT_LG_CB_ENCRYPTION_REQUEST:         return 0x01;
        case MCPR_PKT_LG_CB_LOGIN_SUCCESS:              return 0x02;
        case MCPR_PKT_LG_CB_SET_COMPRESSION:            return 0x03;

        case MCPR_PKT_PL_CB_DISCONNECT:                 return 0x1A;
        case MCPR_PKT_PL_CB_KEEP_ALIVE:                 return 0x1F;
        case MCPR_PKT_PL_CB_JOIN_GAME:                  return 0x23;
        case MCPR_PKT_PL_CB_PLUGIN_MESSAGE:             return 0x18;
        case MCPR_PKT_PL_CB_SPAWN_POSITION:             return 0x46;
        case MCPR_PKT_PL_CB_CHUNK_DATA:                 return 0x20;
        case MCPR_PKT_PL_CB_PLAYER_ABILITIES:           return 0x2C;
        case MCPR_PKT_PL_CB_PLAYER_POSITION_AND_LOOK:   return 0x2F;
        default: DEBUG_PRINT("Unimplemented packet id given in mcpr_packet_type_to_byte().. Aborting at mcpr.c:%i", __LINE__); abort();
    }
    return 0; // Won't even be reached, but else the compiler will complain
}

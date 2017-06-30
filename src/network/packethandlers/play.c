#include <stdlib.h>
#include <string.h>

#include <mcpr/codec.h>
#include <mcpr/connection.h>

#include <network/packethandlers/packethandlers.h>
#include <server.h>
#include <world/world.h>
#include <logging/logging.h>

struct hp_result handle_pl_keep_alive(const struct mcpr_packet *pkt, struct connection *conn)
{
    // We currently ignore keep alive ID's, we may need change that in the future.
    server_get_internal_clock_time(&(conn->player->last_keepalive_received));

    struct hp_result result;
    result.result = HP_RESULT_OK;
    result.disconnect_message = NULL;
    result.free_disconnect_message = false;
    return result;
}

struct hp_result handle_pl_plugin_message(const struct mcpr_packet *pkt, struct connection *conn)
{
    if(strcmp(pkt->data.play.serverbound.plugin_message.channel, "MC|BRAND") == 0)
    {
        int32_t data_length = pkt->data.play.serverbound.plugin_message.data_length;
        if(data_length < 0) goto err;
        if(data_length > SIZE_MAX) goto err;
        void *data = pkt->data.play.serverbound.plugin_message.data;

        char *client_brand;
        ssize_t result = mcpr_decode_string(&client_brand, data, (size_t) data_length);
        if(result < 0) goto err;

        conn->player->client_brand = client_brand;
    }

    struct hp_result hp_result;
    hp_result.result = HP_RESULT_OK;
    hp_result.disconnect_message = NULL;
    hp_result.free_disconnect_message = false;
    return hp_result;

    err: {
        struct hp_result result;
        result.result = HP_RESULT_ERR;
        result.disconnect_message = NULL;
        result.free_disconnect_message = false;
        return result;
    }
}

struct hp_result handle_pl_client_settings(const struct mcpr_packet *pkt, struct connection *conn)
{
    struct player *player = conn->player;
    // We don't know how the locale in the packet is allocated, so we need to copy the string to ensure
    // that it is allocated via malloc()
    char *locale = pkt->data.play.serverbound.client_settings.locale;
    player->client_settings.locale = malloc(strlen(locale) + 1);
    if(player->client_settings.locale == NULL)
    {
        nlog_error("Could not allocate memory. (%s)", strerror(errno));
    }
    else
    {
        strcpy(player->client_settings.locale, locale);
    }

    player->client_settings.view_distance = pkt->data.play.serverbound.client_settings.view_distance;
    player->client_settings.chat_mode = pkt->data.play.serverbound.client_settings.chat_mode;
    player->client_settings.chat_colors = pkt->data.play.serverbound.client_settings.chat_colors;
    player->client_settings.chat_mode = pkt->data.play.serverbound.client_settings.chat_mode;

    player->client_settings.displayed_skin_parts.cape_enabled = pkt->data.play.serverbound.client_settings.displayed_skin_parts.cape_enabled;
    player->client_settings.displayed_skin_parts.jacket_enabled = pkt->data.play.serverbound.client_settings.displayed_skin_parts.jacket_enabled;
    player->client_settings.displayed_skin_parts.left_sleeve_enabled = pkt->data.play.serverbound.client_settings.displayed_skin_parts.left_sleeve_enabled;
    player->client_settings.displayed_skin_parts.right_sleeve_enabled = pkt->data.play.serverbound.client_settings.displayed_skin_parts.right_sleeve_enabled;
    player->client_settings.displayed_skin_parts.left_pants_enabled = pkt->data.play.serverbound.client_settings.displayed_skin_parts.left_pants_enabled;
    player->client_settings.displayed_skin_parts.right_pants_enabled = pkt->data.play.serverbound.client_settings.displayed_skin_parts.right_pants_enabled;
    player->client_settings.displayed_skin_parts.hat_enabled = pkt->data.play.serverbound.client_settings.displayed_skin_parts.hat_enabled;

    player->client_settings_known = true;


    if(world_send_chunk_data(player) < 0)
    {
        nlog_error("Could not send chunk data to player. (%s ?)", strerror(errno));
        goto fatal_err;
    }

    struct mcpr_packet response;
    response.id = MCPR_PKT_PL_CB_PLAYER_POSITION_AND_LOOK;
    response.data.play.clientbound.player_position_and_look.x = player->pos.x;
    response.data.play.clientbound.player_position_and_look.y = player->pos.y;
    response.data.play.clientbound.player_position_and_look.z = player->pos.z;
    response.data.play.clientbound.player_position_and_look.pitch = player->pos.pitch;
    response.data.play.clientbound.player_position_and_look.yaw = player->pos.yaw;

    response.data.play.clientbound.player_position_and_look.x_is_relative = false;
    response.data.play.clientbound.player_position_and_look.y_is_relative = false;
    response.data.play.clientbound.player_position_and_look.z_is_relative = false;
    response.data.play.clientbound.player_position_and_look.pitch_is_relative = false;
    response.data.play.clientbound.player_position_and_look.yaw_is_relative = false;

    response.data.play.clientbound.player_position_and_look.teleport_id = 0;

    if(mcpr_connection_write_packet(conn->conn, &response) < 0)
    {
        if(mcpr_errno == ECONNRESET)
        {
            goto closed;
        }
        else
        {
            nlog_error("Could not send player position and look packet for login sequence. (%s)", mcpr_strerror(mcpr_errno));
            goto fatal_err;
        }
    }
    player->last_teleport_id = 0;

    struct hp_result result;
    result.result = HP_RESULT_OK;
    result.disconnect_message = NULL;
    result.free_disconnect_message = false;
    return result;

    fatal_err: {
        struct hp_result result;
        result.result = HP_RESULT_FATAL;
        result.disconnect_message = NULL;
        result.free_disconnect_message = false;
        return result;
    }

    closed: {
        struct hp_result result;
        result.result = HP_RESULT_CLOSED;
        result.disconnect_message = NULL;
        result.free_disconnect_message = false;
        return result;
    }
}

struct hp_result handle_pl_teleport_confirm(const struct mcpr_packet *pkt, struct connection *conn)
{
    struct player *player = conn->player;
    if(player->last_teleport_id == 0)
    {
        struct mcpr_packet response;
        response.id = MCPR_PKT_PL_CB_PLAYER_POSITION_AND_LOOK;
        response.data.play.clientbound.player_position_and_look.x = player->pos.x;
        response.data.play.clientbound.player_position_and_look.y = player->pos.y;
        response.data.play.clientbound.player_position_and_look.z = player->pos.z;
        response.data.play.clientbound.player_position_and_look.pitch = player->pos.pitch;
        response.data.play.clientbound.player_position_and_look.yaw = player->pos.yaw;

        response.data.play.clientbound.player_position_and_look.x_is_relative = false;
        response.data.play.clientbound.player_position_and_look.y_is_relative = false;
        response.data.play.clientbound.player_position_and_look.z_is_relative = false;
        response.data.play.clientbound.player_position_and_look.pitch_is_relative = false;
        response.data.play.clientbound.player_position_and_look.yaw_is_relative = false;

        response.data.play.clientbound.player_position_and_look.teleport_id = 0;

        if(mcpr_connection_write_packet(conn->conn, &response) < 0)
        {
            if(mcpr_errno == ECONNRESET)
            {
                goto closed;
            }
            else
            {
                nlog_error("Could not send confirmation player position and look packet for login sequence. (%s)", mcpr_strerror(mcpr_errno));
                goto fatal_err;
            }
        }
    }
    player->last_teleport_id++;

    struct hp_result result;
    result.result = HP_RESULT_OK;
    result.disconnect_message = NULL;
    result.free_disconnect_message = false;
    return result;

    fatal_err: {
        struct hp_result result;
        result.result = HP_RESULT_FATAL;
        result.disconnect_message = NULL;
        result.free_disconnect_message = false;
        return result;
    }

    closed: {
        struct hp_result result;
        result.result = HP_RESULT_CLOSED;
        result.disconnect_message = NULL;
        result.free_disconnect_message = false;
        return result;
    }
}

#include <string.h>

#include <ninerr/ninerr.h>

#include <network/network.h>
#include <logging/logging.h>
#include <mcpr/packet.h>
#include <network/packethandlers/packethandlers.h>

struct hp_result handle_st_request(const struct mcpr_packet *pkt, struct connection *conn)
{
    struct mcpr_packet response;
    response.id = MCPR_PKT_ST_CB_RESPONSE;
    response.data.status.clientbound.response.version_name = MCPR_MINECRAFT_VERSION;
    response.data.status.clientbound.response.protocol_version = MCPR_PROTOCOL_VERSION;
    response.data.status.clientbound.response.max_players = net_get_max_players();
    response.data.status.clientbound.response.online_players_size = 0;
    #ifdef __GNUC__
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
    #endif
        response.data.status.clientbound.response.description = net_get_motd();
    #ifdef __GNUC__
        #pragma GCC diagnostic pop
    #endif
    response.data.status.clientbound.response.online_players = -1;
    response.data.status.clientbound.response.player_sample = NULL;
    response.data.status.clientbound.response.favicon = NULL;


    if(mcpr_connection_write_packet(conn->conn, &response) < 0)
    {
        if(ninerr != NULL && strcmp(ninerr->type, "ninerr_closed") == 0)
        {
            struct hp_result hp_result;
            hp_result.result = HP_RESULT_CLOSED;
            hp_result.disconnect_message = NULL;
            hp_result.free_disconnect_message = false;
            return hp_result;
        }
        else
        {
            if(ninerr != NULL && ninerr->message != NULL)
            {
                nlog_error("Could not write packet to connection (%s ?)", ninerr->message);
            }
            else
            {
                nlog_error("Could not write packet to connection.");
            }

            struct hp_result hp_result;
            hp_result.result = HP_RESULT_ERR;
            hp_result.disconnect_message = NULL;
            hp_result.free_disconnect_message = false;
            return hp_result;
        }
    }

    struct hp_result hp_result;
    hp_result.result = HP_RESULT_OK;
    hp_result.disconnect_message = NULL;
    hp_result.free_disconnect_message = false;
    return hp_result;
}

struct hp_result handle_st_ping(const struct mcpr_packet *pkt, struct connection *conn)
{
    struct mcpr_packet response;
    response.id = MCPR_PKT_ST_CB_PONG;
    response.data.status.clientbound.pong.payload = pkt->data.status.serverbound.ping.payload;

    if(mcpr_connection_write_packet(conn->conn, &response) < 0)
    {
        if(ninerr != NULL && ninerr->message != NULL)
        {
            struct hp_result hp_result;
            hp_result.result = HP_RESULT_CLOSED;
            hp_result.disconnect_message = NULL;
            hp_result.free_disconnect_message = false;
            return hp_result;
        }
        else
        {
            if(ninerr != NULL && ninerr->message != NULL)
            {
                nlog_error("Could not write packet to connection (%s ?)", ninerr->message);
            }
            else
            {
                nlog_error("Could not write packet to connection.");
            }

            struct hp_result hp_result;
            hp_result.result = HP_RESULT_ERR;
            hp_result.disconnect_message = NULL;
            hp_result.free_disconnect_message = false;
            return hp_result;
        }
    }

    struct hp_result hp_result;
    hp_result.result = HP_RESULT_OK;
    hp_result.disconnect_message = NULL;
    hp_result.free_disconnect_message = false;
    return hp_result;
}

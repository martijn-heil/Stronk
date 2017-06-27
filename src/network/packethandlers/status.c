#include <logging/logging.h>
#include <network/network.h>
#include <mcpr/abstract_packet.h>
#include <network/packethandlers/packethandlers.h>

struct hp_result handle_st_request(const struct mcpr_abstract_packet *pkt, struct connection *conn)
{
    struct mcpr_abstract_packet response;
    response.id = MCPR_PKT_ST_CB_RESPONSE;
    response.data.status.clientbound.response.version_name = MCPR_MINECRAFT_VERSION;
    response.data.status.clientbound.response.protocol_version = MCPR_PROTOCOL_VERSION;
    response.data.status.clientbound.response.max_players = net_get_max_players();
    response.data.status.clientbound.response.online_players_size = 0;
    response.data.status.clientbound.response.description = net_get_motd();
    response.data.status.clientbound.response.online_players = -1;
    response.data.status.clientbound.response.player_sample = NULL;
    response.data.status.clientbound.response.favicon = NULL;


    if(mcpr_connection_write_packet(conn->conn, &response) < 0)
    {
        if(mcpr_errno == ECONNRESET)
        {
            struct hp_result hp_result;
            hp_result.result = HP_RESULT_CLOSED;
            hp_result.disconnect_message = NULL;
            hp_result.free_disconnect_message = false;
            return hp_result;
        }
        else
        {
            nlog_error("Could not write packet to connection (%s ?)", mcpr_strerror(mcpr_errno));
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

struct hp_result handle_st_ping(const struct mcpr_abstract_packet *pkt, struct connection *conn)
{
    struct mcpr_abstract_packet response;
    response.id = MCPR_PKT_ST_CB_PONG;
    response.data.status.clientbound.pong.payload = pkt->data.status.serverbound.ping.payload;

    if(mcpr_connection_write_packet(conn->conn, &response) < 0)
    {
        if(mcpr_errno == ECONNRESET)
        {
            struct hp_result hp_result;
            hp_result.result = HP_RESULT_CLOSED;
            hp_result.disconnect_message = NULL;
            hp_result.free_disconnect_message = false;
            return hp_result;
        }
        else
        {
            nlog_error("Could not write packet to connection (%s ?)", mcpr_strerror(mcpr_errno));
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

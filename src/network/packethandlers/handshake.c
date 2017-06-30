#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <mcpr/mcpr.h>
#include <mcpr/packet.h>
#include <network/connection.h>
#include <network/packethandlers/packethandlers.h>

struct hp_result handle_hs_handshake(const struct mcpr_packet *pkt, struct connection *conn)\
{
    int32_t protocol_version = pkt->data.handshake.serverbound.handshake.protocol_version;
    mcpr_connection_set_state(conn->conn, pkt->data.handshake.serverbound.handshake.next_state);

    if(protocol_version != MCPR_PROTOCOL_VERSION)
    {
        if(mcpr_connection_get_state(conn->conn) == MCPR_STATE_LOGIN)
        {
            char *reason = mcpr_as_chat("Protocol version " PRId32 " is not supported! I'm on protocol version %i (MC %s)", protocol_version, MCPR_PROTOCOL_VERSION, MCPR_MINECRAFT_VERSION);
            struct hp_result hp_result;
            hp_result.result = HP_RESULT_FATAL;
            hp_result.disconnect_message = reason;
            hp_result.free_disconnect_message = true;
            return hp_result;
        }
    }

    struct hp_result hp_result;
    hp_result.result = HP_RESULT_OK;
    hp_result.disconnect_message = NULL;
    hp_result.free_disconnect_message = false;
    return hp_result;
}

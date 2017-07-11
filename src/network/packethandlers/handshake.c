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
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <mcpr/mcpr.h>
#include <mcpr/packet.h>
#include <network/connection.h>
#include <network/packethandlers/packethandlers.h>
#include <logging/logging.h>

struct hp_result handle_hs_handshake(const struct mcpr_packet *pkt, struct connection *conn)\
{
    nlog_info("Received a handshake packet.");
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
    size_t server_address_len = strlen(pkt->data.handshake.serverbound.handshake.server_address);
    conn->server_address_used = malloc(server_address_len + 1);
    if(conn->server_address_used == NULL)
    {
        nlog_error("Could not allocate memory for conn->server_address (%s)", strerror(errno));
        struct hp_result hp_result;
        hp_result.result = HP_RESULT_FATAL;
        if(mcpr_connection_get_state(conn->conn) == MCPR_STATE_LOGIN)
        {
            hp_result.disconnect_message = mcpr_as_chat("A fatal error occurred whilst logging in.");
            hp_result.free_disconnect_message = true;
        }
        else
        {
            hp_result.disconnect_message = NULL;
            hp_result.free_disconnect_message = false;
        }
        return hp_result;
    }
    memcpy(conn->server_address_used, pkt->data.handshake.serverbound.handshake.server_address, server_address_len + 1);

    struct hp_result hp_result;
    hp_result.result = HP_RESULT_OK;
    hp_result.disconnect_message = NULL;
    hp_result.free_disconnect_message = false;
    return hp_result;
}

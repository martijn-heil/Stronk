/*
  MIT License

  Copyright (c) 2017-2018 Martijn Heil

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

#include <string.h>

#include <ninerr/ninerr.h>

#include <network/network.h>
#include <logging/logging.h>
#include <mcpr/packet.h>
#include <network/packethandlers/packethandlers.h>

struct hp_result handle_st_request(const struct mcpr_packet *pkt, struct connection *conn)
{
  nlog_debug("in handle_st_request");

  struct mcpr_packet response;
  response.id = MCPR_PKT_ST_CB_RESPONSE;
  response.state = MCPR_STATE_STATUS;
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
  response.data.status.clientbound.response.online_players = 999;
  response.data.status.clientbound.response.player_sample = NULL;
  response.data.status.clientbound.response.favicon = NULL;
  nlog_info("Motd: %s", net_get_motd());


  if(!mcpr_connection_write_packet(conn->conn, &response))
  {
    if(ninerr != NULL && strcmp(ninerr->type, "ninerr_closed") == 0)
    {
      nlog_debug("Could not write packet to connection, it was closed.");
      struct hp_result hp_result;
      hp_result.result = HP_RESULT_CLOSED;
      hp_result.disconnect_message = NULL;
      hp_result.free_disconnect_message = false;
      return hp_result;
    }
    else
    {
      nlog_error("Could not write packet to connection.");
      ninerr_print(ninerr);

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
  nlog_debug("in handle_st_ping");
  struct mcpr_packet response;
  response.id = MCPR_PKT_ST_CB_PONG;
  response.state = MCPR_STATE_STATUS;
  response.data.status.clientbound.pong.payload = pkt->data.status.serverbound.ping.payload;

  if(!mcpr_connection_write_packet(conn->conn, &response))
  {
    if(ninerr != NULL && strcmp(ninerr->type, "ninerr_closed") == 0)
    {
      nlog_debug("Could not write packet to connection, it was closed.");
      struct hp_result hp_result;
      hp_result.result = HP_RESULT_CLOSED;
      hp_result.disconnect_message = NULL;
      hp_result.free_disconnect_message = false;
      return hp_result;
    }
    else
    {
      nlog_error("Could not write packet to connection.");
      ninerr_print(ninerr);

      struct hp_result hp_result;
      hp_result.result = HP_RESULT_ERR;
      hp_result.disconnect_message = NULL;
      hp_result.free_disconnect_message = false;
      return hp_result;
    }
  }

  struct hp_result hp_result;
  hp_result.result = HP_RESULT_CLOSED;
  hp_result.disconnect_message = NULL;
  hp_result.free_disconnect_message = false;
  return hp_result;
}

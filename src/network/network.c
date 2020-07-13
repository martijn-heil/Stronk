/*
  MIT License

  Copyright (c) 2016-2018 Martijn Heil

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
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include <thpool.h>

#include <algo/slist.h>
#include <algo/hash-table.h>
#include <algo/hash-pointer.h>
#include <algo/compare-pointer.h>
#include <algo/hash-string.h>
#include <algo/compare-string.h>

#include <ninerr/ninerr.h>

#include <mcpr/mcpr.h>
#include <mcpr/packet.h>
#include <mcpr/codec.h>
#include <mcpr/connection.h>

#include <logging/logging.h>

#include "network/network.h"
#include <network/connection.h>
#include <network/packethandlers/packethandlers.h>
#include <server.h>
#include "stronk.h"
#include "util.h"

static int server_socket;
static size_t client_count = 0;
static pthread_rwlock_t clients_lock;
static SListEntry *clients = NULL; // List of clients.
static char *motd;
static struct addrinfo *addressinfo;


static void accept_incoming_connections(void);
static void serve_client_batch(void *arg);
static void serve_clients(void);


int net_init(void) {
  const char *service = "25565"; // TODO configuration of port.
  nlog_info("Creating server socket..");

  struct addrinfo hints;

  // first, load up address structs with getaddrinfo():
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC; // use IPv4 or IPv6, whichever
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // fill in my IP for me

  int getaddrinfo_result = getaddrinfo(NULL, service, &hints, &addressinfo);
  if(getaddrinfo_result != 0)
  {
    nlog_fatal("Could not set up inbound socket address, getaddrinfo() returned an error: %s", gai_strerror(getaddrinfo_result));
    return -1;
  }

  // make a socket:
  server_socket = socket(addressinfo->ai_family, addressinfo->ai_socktype, addressinfo->ai_protocol);
  if(server_socket == -1)
  {
    nlog_fatal("Could not create server socket. (%s)", strerror(errno));
    return -1;
  }

  int yes=1;
  //char yes='1'; // Solaris people use this

  // lose the pesky "Address already in use" error message
  if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
    nlog_fatal("Could not setsockopt");
    return -1;
  }

  // bind it to the port we passed in to getaddrinfo():
  if(bind(server_socket, addressinfo->ai_addr, addressinfo->ai_addrlen) == -1)
  {
    nlog_fatal("Could not bind socket. (%s)", strerror(errno));
    return -1;
  }
  if(addressinfo->ai_addr->sa_family == AF_INET6)
  {
    char buf[INET6_ADDRSTRLEN];
    const char *ip = inet_ntop(AF_INET6, addressinfo->ai_addr, buf, addressinfo->ai_addrlen);
    if(ip != NULL)
    {
      nlog_info("Successfully bound socket to [%s]:%hu", ip, ntoh16(((struct sockaddr_in6 *) addressinfo->ai_addr)->sin6_port));
    }
  }
  else if(addressinfo->ai_addr->sa_family == AF_INET)
  {
    char buf[INET_ADDRSTRLEN];
    const char *ip = inet_ntop(AF_INET, addressinfo->ai_addr, buf, addressinfo->ai_addrlen);
    if(ip != NULL)
    {
      nlog_info("Successfully bound socket to %s:%hu", ip, ntoh16(((struct sockaddr_in *) addressinfo->ai_addr)->sin_port));
    }
  }


  if(fcntl(server_socket, F_SETFL, O_NONBLOCK) == -1)
  {
    nlog_fatal("Could not set O_NONBLOCK flag for server socket. (%s)", strerror(errno));
    return -1;
  }

  // Ignore broken pipe signals, has to do with sockets.
  nlog_info("Making sure that broken pipe (SIGPIPE) signals are ignored..");
  struct sigaction new_actn, old_actn;
  new_actn.sa_handler = SIG_IGN;
  sigemptyset (&new_actn.sa_mask);
  new_actn.sa_flags = 0;
  sigaction (SIGPIPE, &new_actn, &old_actn);

  nlog_info("Listening on server socket with a backlog of 20..");
  if(listen(server_socket, 20) == -1)
  {
    nlog_fatal("Could not listen on server socket. (%s)", strerror(errno));
    return -1;
  }

  if(pthread_rwlock_init(&clients_lock, NULL) != 0)
  {
    nlog_fatal("Could not initialize clients delete lock.");
    return -1;
  }

  motd = mcpr_as_chat("A bloody stronk server.");
  if(motd == NULL)
  {
    nlog_fatal("Could not generate MOTD.");
    ninerr_print(ninerr);
    return -1;
  }

  return 1;
}

void net_cleanup(void)
{
  freeaddrinfo(addressinfo);
  // TODO
}

void net_tick(void)
{
  accept_incoming_connections();
  serve_clients();
}

void connection_close(struct connection *conn, const char *disconnect_message)
{
  // TODO should we free player here?
  again: if(pthread_rwlock_wrlock(&clients_lock) != 0) { nlog_debug("Could not lock internal clock lock! Retrying.."); goto again; }

  bool found = false;
  int i = 0;
  for(; i < slist_length(clients); i++) if(slist_nth_data(clients, i) == conn) { found = true; break; }
  if(!found) { nlog_fatal("Fatal error! Could not find client which needs to be closed in client list!"); exit(EXIT_FAILURE); }
  SListEntry *entry = slist_nth_entry(clients, i);
  if(entry == SLIST_NULL) { nlog_error("Fatal error! Could not find client which needs to be closed in client list!"); exit(EXIT_FAILURE); }


  if(slist_remove_entry(&clients, entry) == 0) { nlog_error("Could not remove entry from clients"); }
  client_count--;
  pthread_rwlock_unlock(&clients_lock);
  mcpr_connection_close(conn->conn, disconnect_message);
  fclose(conn->rawstream);
  if(close(conn->fd) == -1) nlog_warn("Error whilst closing a socket: %s", strerror(errno));
  free(conn->server_address_used);
  free(conn);

  nlog_info("Connection at address %p closed.", (void *) conn);
}

static bool packet_handler(const struct mcpr_packet *pkt, struct connection *conn2)
{
  nlog_debug("Received a packet! at packet_handler");
  mcpr_connection *conn = conn2->conn;

  struct hp_result result;
  IGNORE("-Wswitch")
  switch(mcpr_connection_get_state(conn))
  {
    case MCPR_STATE_HANDSHAKE:
    {
      switch(pkt->id)
      {
        case MCPR_PKT_HS_SB_HANDSHAKE: { result = handle_hs_handshake(pkt, conn2); goto finish; }
      }
      break;
    }

    case MCPR_STATE_STATUS:
    {
      switch(pkt->id)
      {
        case MCPR_PKT_ST_SB_REQUEST:  { result = handle_st_request(pkt, conn2); goto finish; }
        case MCPR_PKT_ST_SB_PING:     { result = handle_st_ping(pkt, conn2);    goto finish; }
      }
      break;
    }

    case MCPR_STATE_LOGIN:
    {
      switch(pkt->id)
      {
        case MCPR_PKT_LG_SB_LOGIN_START:          { result = handle_lg_login_start(pkt, conn2);         goto finish; }
        case MCPR_PKT_LG_SB_ENCRYPTION_RESPONSE:  { result = handle_lg_encryption_response(pkt, conn2); goto finish; }
      }
      break;
    }

    case MCPR_STATE_PLAY:
    {
      switch(pkt->id)
      {
        case MCPR_PKT_PL_SB_TELEPORT_CONFIRM:           { result = handle_pl_teleport_confirm(pkt, conn2);          goto finish; }
        case MCPR_PKT_PL_SB_TAB_COMPLETE:               { result = handle_pl_tab_complete(pkt, conn2);              goto finish; }
        case MCPR_PKT_PL_SB_CHAT_MESSAGE:               { result = handle_pl_chat_message(pkt, conn2);              goto finish; }
        case MCPR_PKT_PL_SB_CLIENT_STATUS:              { result = handle_pl_client_status(pkt, conn2);             goto finish; }
        case MCPR_PKT_PL_SB_CLIENT_SETTINGS:            { result = handle_pl_client_settings(pkt, conn2);           goto finish; }
        case MCPR_PKT_PL_SB_CONFIRM_TRANSACTION:        { result = handle_pl_confirm_transaction(pkt, conn2);       goto finish; }
        case MCPR_PKT_PL_SB_ENCHANT_ITEM:               { result = handle_pl_enchant_item(pkt, conn2);              goto finish; }
        case MCPR_PKT_PL_SB_CLICK_WINDOW:               { result = handle_pl_click_window(pkt, conn2);              goto finish; }
        case MCPR_PKT_PL_SB_CLOSE_WINDOW:               { result = handle_pl_close_window(pkt, conn2);              goto finish; }
        case MCPR_PKT_PL_SB_PLUGIN_MESSAGE:             { result = handle_pl_plugin_message(pkt, conn2);            goto finish; }
        case MCPR_PKT_PL_SB_USE_ENTITY:                 { result = handle_pl_use_entity(pkt, conn2);                goto finish; }
        case MCPR_PKT_PL_SB_KEEP_ALIVE:                 { result = handle_pl_keep_alive(pkt, conn2);                goto finish; }
        case MCPR_PKT_PL_SB_PLAYER_POSITION:            { result = handle_pl_player_position(pkt, conn2);           goto finish; }
        case MCPR_PKT_PL_SB_PLAYER_POSITION_AND_LOOK:   { result = handle_pl_player_position_and_look(pkt, conn2);  goto finish; }
        case MCPR_PKT_PL_SB_PLAYER_LOOK:                { result = handle_pl_player_look(pkt, conn2);               goto finish; }
        case MCPR_PKT_PL_SB_PLAYER:                     { result = handle_pl_player(pkt, conn2);                    goto finish; }
        case MCPR_PKT_PL_SB_VEHICLE_MOVE:               { result = handle_pl_vehicle_move(pkt, conn2);              goto finish; }
        case MCPR_PKT_PL_SB_STEER_BOAT:                 { result = handle_pl_steer_boat(pkt, conn2);                goto finish; }
        case MCPR_PKT_PL_SB_PLAYER_ABILITIES:           { result = handle_pl_player_abilities(pkt, conn2);          goto finish; }
        case MCPR_PKT_PL_SB_PLAYER_DIGGING:             { result = handle_pl_player_digging(pkt, conn2);            goto finish; }
        case MCPR_PKT_PL_SB_ENTITY_ACTION:              { result = handle_pl_entity_action(pkt, conn2);             goto finish; }
        case MCPR_PKT_PL_SB_STEER_VEHICLE:              { result = handle_pl_steer_vehicle(pkt, conn2);             goto finish; }
        case MCPR_PKT_PL_SB_RESOURCE_PACK_STATUS:       { result = handle_pl_resource_pack_status(pkt, conn2);      goto finish; }
        case MCPR_PKT_PL_SB_HELD_ITEM_CHANGE:           { result = handle_pl_held_item_change(pkt, conn2);          goto finish; }
        case MCPR_PKT_PL_SB_CREATIVE_INVENTORY_ACTION:  { result = handle_pl_creative_inventory_action(pkt, conn2); goto finish; }
        case MCPR_PKT_PL_SB_UPDATE_SIGN:                { result = handle_pl_update_sign(pkt, conn2);               goto finish; }
        case MCPR_PKT_PL_SB_ANIMATION:                  { result = handle_pl_animation(pkt, conn2);                 goto finish; }
        case MCPR_PKT_PL_SB_SPECTATE:                   { result = handle_pl_spectate(pkt, conn2);                  goto finish; }
        case MCPR_PKT_PL_SB_PLAYER_BLOCK_PLACEMENT:     { result = handle_pl_player_block_placement(pkt, conn2);    goto finish; }
        case MCPR_PKT_PL_SB_USE_ITEM:                   { result = handle_pl_use_item(pkt, conn2);                  goto finish; }
      }
      break;
    }
  }
  END_IGNORE()

  finish:
    if(result.result == HP_RESULT_FATAL)
    {
      nlog_error("Fatal error, closing connection");
      connection_close(conn2, result.disconnect_message);
      IGNORE("-Wdiscarded-qualifiers")
      if(result.free_disconnect_message) free(result.disconnect_message);
      END_IGNORE()
      return false;
    }
    else if(result.result == HP_RESULT_CLOSED)
    {
      connection_close(conn2, result.disconnect_message);
      IGNORE("-Wdiscarded-qualifiers")
      if(result.free_disconnect_message) free(result.disconnect_message);
      END_IGNORE()
      return false;
    }
    else
    {
      IGNORE("-Wdiscarded-qualifiers")
      if(result.free_disconnect_message) free(result.disconnect_message);
      END_IGNORE()
      return true;
    }
}

static void accept_incoming_connections(void)
{
  char ip_str_buf[128];
  while(true)
  {
    struct sockaddr_storage clientname;
    socklen_t clientname_size = (socklen_t) sizeof(clientname);
    int newfd = accept(server_socket, (struct sockaddr *) &clientname, &clientname_size);
    if(newfd == -1)
    {
      if(errno == EAGAIN || errno == EWOULDBLOCK) // There are no incoming connections in the queue.
      {
        break;
      }
      else if(errno == ECONNABORTED)
      {
        nlog_debug("An incoming connection was aborted.");
      }
      else
      {
        nlog_error("Could not accept incoming connection. (%s)", strerror(errno));
        continue;
      }
    }
    nlog_info("Accepted incoming connection from %s:%u (fd = %d)",
      sockaddr_ip_str((struct sockaddr *) &clientname, ip_str_buf, 128),
      (clientname.ss_family == AF_INET6) ?
        ntohs(((struct sockaddr_in *) &clientname)->sin_port) :
        ntohs(((struct sockaddr_in6 *) &clientname)->sin6_port), newfd);

    if(fcntl(newfd, F_SETFL, O_NONBLOCK) == -1)
    {
      nlog_fatal("Could not set O_NONBLOCK flag for incoming connection. (%s)", strerror(errno));
      if(close(newfd) == -1) nlog_error("Error closing socket after previous error. (%s)", strerror(errno));
      continue;
    }

    FILE *stream = fdopen(newfd, "r+");
    if(stream == NULL) { nlog_error("fdopen() failed (%s)", strerror(errno)); continue; }
    if(setvbuf(stream, NULL, _IONBF, 0) != 0) { nlog_error("setvbuf() failed (%s ?)", strerror(errno)); continue; }
    mcpr_connection *conn = mcpr_connection_new(stream);
    if(conn == NULL)
    {
      nlog_error("Could not create new connection object. (%s)", ninerr->message);
      fclose(stream);
      continue;
    }

    struct connection *conn2 = malloc(sizeof(struct connection));
    if(conn2 == NULL)
    {
      nlog_error("Could not allocate memory for connection. (%s)", strerror(errno));
      if(close(newfd) == -1) nlog_error("Error closing socket after memory allocation failure. (%s)", strerror(errno));
      //mcpr_connection_decref(conn);
      fclose(stream);
      continue;
    }
    conn2->player = NULL;
    conn2->conn = conn;
    conn2->fd = newfd;
    conn2->rawstream = stream;
    conn2->pktstream = mcpr_connection_get_stream(conn);
    conn2->server_address_used = NULL;
    conn2->tmp_present = false;
    conn2->client_address = clientname;

    if(slist_append(&clients, conn2) == NULL)
    {
      nlog_error("Could not add incoming connection to connection storage.");
      if(close(newfd) == -1) nlog_error("Error closing socket after previous error. (%s)", strerror(errno));
      //mcpr_connection_decref(conn);
      fclose(conn2->rawstream);
      free(conn2);
      continue;
    }
    client_count++;
    nlog_info("Client from %s:%u (fd = %d) connected successfully.", sockaddr_ip_str((struct sockaddr *) &clientname, ip_str_buf, 128),
    (clientname.ss_family == AF_INET6) ?
      ntohs(((struct sockaddr_in *) &clientname)->sin_port) :
      ntohs(((struct sockaddr_in6 *) &clientname)->sin6_port), newfd);
  }
}

static void update_client(struct connection *conn)
{
  struct player *player = conn->player; // Will be NULL if there is no player associated with this connection.
  if(player != NULL)
  {
    struct timespec now;
    server_get_internal_clock_time(&now);

    struct timespec diff;
    timespec_diff(&diff, &(player->last_keepalive_sent), &now);

    if(diff.tv_sec >= 10)
    {
      struct mcpr_packet keep_alive;
      keep_alive.id = MCPR_PKT_PL_CB_KEEP_ALIVE;
      keep_alive.state = MCPR_STATE_PLAY;
      keep_alive.data.play.clientbound.keep_alive.keep_alive_id = 0;


      if(fwrite(&keep_alive, sizeof(keep_alive), 1, conn->pktstream) == 0)
      {
        if(strcmp(ninerr->type, "ninerr_closed") == 0)
        {
          connection_close(conn, NULL);
          return;
        }
        else
        {
          nlog_error("Could not send keep alive packet. (%s)", ninerr->message);
        }
      }
      else
      {
        server_get_internal_clock_time(&(player->last_keepalive_sent));
      }
    }
  }

  struct mcpr_packet pkt;
  while(fread(&pkt, sizeof(pkt), 1, conn->pktstream) != EOF) packet_handler(&pkt, conn);

  if(player != NULL)
  {
    struct timespec now;
    server_get_internal_clock_time(&now);

    struct timespec diff;
    timespec_diff(&diff, &(player->last_keepalive_received), &now);

    if(diff.tv_sec >= 30)
    {
      char uuid[37];
      ninuuid_to_string(&(player->uuid), uuid, LOWERCASE, false);
      nlog_error("Player with UUID %s timed out. (server-side)", uuid);

      char *reason = mcpr_as_chat("Timed out. (server-side)");
      connection_close(conn, reason);
      free(reason);
    }
  }
}

static void serve_client_batch(void *arg)
{
  again: if(pthread_rwlock_rdlock(&clients_lock) != 0) { nlog_warn("Could not lock clients lock. Retrying.."); goto again; }

  SListEntry *first = *(SListEntry **) arg;
  unsigned int amount = *((unsigned int *) (arg + sizeof(SListEntry *)));

  struct connection *clients[amount];
  for(unsigned int i = 0; i < amount; i++)
  {
    clients[i] = slist_nth_data(first, i);
    if(clients[i] == SLIST_NULL) // Shouldn't happen
    {
      nlog_error("Bad slist index %i. (amount: %u)", i, amount);
      break;
    }
  }
  pthread_rwlock_unlock(&clients_lock);
  free(arg);

  for(unsigned int i = 0; i < amount; i++) update_client(clients[i]);
}

static void serve_clients(void)
{
  again: if(pthread_rwlock_rdlock(&clients_lock) != 0) { nlog_warn("Could not lock clients lock. Retrying.."); goto again; }
  unsigned int conns_per_thread = client_count / main_threadpool_threadcount;
  unsigned int rest = client_count % main_threadpool_threadcount;

  unsigned int index = 0;

  for(unsigned int i = 0; i < main_threadpool_threadcount; i++)
  {
    unsigned int amount = conns_per_thread;
    if(i == (main_threadpool_threadcount - 1)) amount += rest;

    SListEntry *arg1 = slist_nth_entry(clients, index);
    if(arg1 == NULL) continue;
    unsigned int arg2 = amount;
    void *args = malloc(sizeof(SListEntry *) + sizeof(unsigned int));
    if(args == NULL)
    {
      nlog_error("Could not serve client. Could not allocate memory. (%s)", strerror(errno));
      free(args);
      continue;
    }
    memcpy(args, &arg1, sizeof(arg1));
    memcpy(args + sizeof(arg1), &arg2, sizeof(arg2));
    thpool_add_work(main_threadpool, serve_client_batch, args);
    index += amount;
  }
  pthread_rwlock_unlock(&clients_lock);

  thpool_wait(main_threadpool);
}

unsigned int net_get_max_players(void)
{
  return 999; // TODO
}

const char *net_get_motd(void)
{
  return motd;
}

/*
    MIT License

    Copyright (c) 2016 Martijn Heil

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


static void accept_incoming_connections(void);
static int make_server_socket (uint16_t port);
static void serve_client_batch(void *arg);
static void serve_clients(void);


int net_init(void) {
    uint16_t port = 25565; // TODO configuration of port.

    nlog_info("Creating server socket on port %i..", port);
    server_socket = make_server_socket(port);
    if(server_socket < 0)
    {
        nlog_fatal("Could not create server socket. (%s ?)", strerror(errno));
        return -1;
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

    nlog_info("Setting backlog for server socket to 5..");
    if(listen(server_socket, 5) == -1)
    {
        nlog_fatal("Could not set backlog for server socket.");
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
    // TODO
}

void net_tick(void)
{
    accept_incoming_connections();
    serve_clients();
}


static int make_server_socket (uint16_t port)
{
    struct sockaddr_in name;

    // Create the socket.
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        nlog_error("Could not create socket. (%s)", strerror(errno));
        return -1;
    }

    int yes=1;
    //char yes='1'; // Solaris people use this
    // lose the pesky "Address already in use" error message
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1)
    {
        nlog_fatal("Could not setsockopt SO_REUSEADDR");
        return -1;
    } 

    // Give the socket a name.
    name.sin_family = AF_INET;
    name.sin_port = hton16(port);
    name.sin_addr.s_addr = hton32(INADDR_ANY);
    if (bind(sockfd, (struct sockaddr *) &name, sizeof (name)) < 0)
    {
        nlog_error("Could not bind socket to address. (%s)", strerror(errno));
        return -1;
    }

    return sockfd;
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
    if(close(conn->fd) == -1) nlog_warn("Error whilst closing a socket: %s", strerror(errno));
    bstream_decref(conn->iostream);
    mcpr_connection_decref(conn->conn);
    free(conn->server_address_used);

    nlog_info("Connection at address %p closed.", (void *) conn);
    free(conn);
}

static bool packet_handler(const struct mcpr_packet *pkt, mcpr_connection *conn)
{
    nlog_debug("Received a packet! at packet_handler");

    struct connection *conn2 = NULL;

    int lock_result;
    do {
        lock_result = pthread_rwlock_rdlock(&clients_lock);
        if(lock_result != 0) { nlog_warn("Could not lock clients lock. Retrying.."); }
    } while(lock_result != 0);

    for(unsigned int i = 0; i < client_count; i++)
    {
        struct connection *tmp = (struct connection *) slist_nth_data(clients, i);
        if(tmp == NULL)
        {
            nlog_fatal("Fatal error, aborting..");
            server_crash();
        }
        if(tmp->conn == conn)
        {
            conn2 = tmp;
            break;
        }
    }
    pthread_rwlock_unlock(&clients_lock);
    if(conn2 == NULL)
    {
        nlog_error("Could not find connection struct for given mcpr_connection.");
        return true;
    }

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
                case MCPR_PKT_ST_SB_REQUEST: { result = handle_st_request(pkt, conn2); goto finish; }
                case MCPR_PKT_ST_SB_PING: { result = handle_st_ping(pkt, conn2); goto finish; }
            }
            break;
        }

        case MCPR_STATE_LOGIN:
        {
            switch(pkt->id)
            {
                case MCPR_PKT_LG_SB_LOGIN_START: { result = handle_lg_login_start(pkt, conn2); goto finish; }
                case MCPR_PKT_LG_SB_ENCRYPTION_RESPONSE: { result = handle_lg_encryption_response(pkt, conn2); goto finish; }
            }
            break;
        }

        case MCPR_STATE_PLAY:
        {
            switch(pkt->id)
            {
                case MCPR_PKT_PL_SB_TELEPORT_CONFIRM:               { result = handle_pl_teleport_confirm(pkt, conn2);              goto finish; }
                case MCPR_PKT_PL_SB_TAB_COMPLETE:                   { result = handle_pl_tab_complete(pkt, conn2);                  goto finish; }
                case MCPR_PKT_PL_SB_CHAT_MESSAGE:                   { result = handle_pl_chat_message(pkt, conn2);                  goto finish; }
                case MCPR_PKT_PL_SB_CLIENT_STATUS:                  { result = handle_pl_client_status(pkt, conn2);                 goto finish; }
                case MCPR_PKT_PL_SB_CLIENT_SETTINGS:                { result = handle_pl_client_settings(pkt, conn2);               goto finish; }
                case MCPR_PKT_PL_SB_CONFIRM_TRANSACTION:            { result = handle_pl_confirm_transaction(pkt, conn2);           goto finish; }
                case MCPR_PKT_PL_SB_ENCHANT_ITEM:                   { result = handle_pl_enchant_item(pkt, conn2);                  goto finish; }
                case MCPR_PKT_PL_SB_CLICK_WINDOW:                   { result = handle_pl_click_window(pkt, conn2);                  goto finish; }
                case MCPR_PKT_PL_SB_CLOSE_WINDOW:                   { result = handle_pl_close_window(pkt, conn2);                  goto finish; }
                case MCPR_PKT_PL_SB_PLUGIN_MESSAGE:                 { result = handle_pl_plugin_message(pkt, conn2);                goto finish; }
                case MCPR_PKT_PL_SB_USE_ENTITY:                     { result = handle_pl_use_entity(pkt, conn2);                    goto finish; }
                case MCPR_PKT_PL_SB_KEEP_ALIVE:                     { result = handle_pl_keep_alive(pkt, conn2);                    goto finish; }
                case MCPR_PKT_PL_SB_PLAYER_POSITION:                { result = handle_pl_player_position(pkt, conn2);               goto finish; }
                case MCPR_PKT_PL_SB_PLAYER_POSITION_AND_LOOK:       { result = handle_pl_player_position_and_look(pkt, conn2);      goto finish; }
                case MCPR_PKT_PL_SB_PLAYER_LOOK:                    { result = handle_pl_player_look(pkt, conn2);                   goto finish; }
                case MCPR_PKT_PL_SB_PLAYER:                         { result = handle_pl_player(pkt, conn2);                        goto finish; }
                case MCPR_PKT_PL_SB_VEHICLE_MOVE:                   { result = handle_pl_vehicle_move(pkt, conn2);                  goto finish; }
                case MCPR_PKT_PL_SB_STEER_BOAT:                     { result = handle_pl_steer_boat(pkt, conn2);                    goto finish; }
                case MCPR_PKT_PL_SB_PLAYER_ABILITIES:               { result = handle_pl_player_abilities(pkt, conn2);              goto finish; }
                case MCPR_PKT_PL_SB_PLAYER_DIGGING:                 { result = handle_pl_player_digging(pkt, conn2);                goto finish; }
                case MCPR_PKT_PL_SB_ENTITY_ACTION:                  { result = handle_pl_entity_action(pkt, conn2);                 goto finish; }
                case MCPR_PKT_PL_SB_STEER_VEHICLE:                  { result = handle_pl_steer_vehicle(pkt, conn2);                 goto finish; }
                case MCPR_PKT_PL_SB_RESOURCE_PACK_STATUS:           { result = handle_pl_resource_pack_status(pkt, conn2);          goto finish; }
                case MCPR_PKT_PL_SB_HELD_ITEM_CHANGE:               { result = handle_pl_held_item_change(pkt, conn2);              goto finish; }
                case MCPR_PKT_PL_SB_CREATIVE_INVENTORY_ACTION:      { result = handle_pl_creative_inventory_action(pkt, conn2);     goto finish; }
                case MCPR_PKT_PL_SB_UPDATE_SIGN:                    { result = handle_pl_update_sign(pkt, conn2);                   goto finish; }
                case MCPR_PKT_PL_SB_ANIMATION:                      { result = handle_pl_animation(pkt, conn2);                     goto finish; }
                case MCPR_PKT_PL_SB_SPECTATE:                       { result = handle_pl_spectate(pkt, conn2);                      goto finish; }
                case MCPR_PKT_PL_SB_PLAYER_BLOCK_PLACEMENT:         { result = handle_pl_player_block_placement(pkt, conn2);        goto finish; }
                case MCPR_PKT_PL_SB_USE_ITEM:                       { result = handle_pl_use_item(pkt, conn2);                      goto finish; }
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
    while(true)
    {
        int newfd = accept(server_socket, NULL, NULL);
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

        if(fcntl(newfd, F_SETFL, O_NONBLOCK) == -1)
        {
            const char *err = strerror(errno);
            nlog_fatal("Could not set O_NONBLOCK flag for incoming connection. (%s)", err);
            if(close(newfd) == -1) nlog_error("Error closing socket after previous error. (%s)", err);
            continue;
        }

        int so_sndbuf;
        socklen_t so_sndbuf_len = sizeof(so_sndbuf);
        if(getsockopt(newfd, SOL_SOCKET, SO_SNDBUF, &so_sndbuf, &so_sndbuf_len) == -1)
        {
            const char *err = strerror(errno);
            nlog_fatal("Could not get SO_SNDBUF value for incoming connection. (%s)", err);
            if(close(newfd) == -1) nlog_error("Error closing socket after previous error. (%s)", err);
            continue;
        }

        struct bstream *rawstream = bstream_from_fd(newfd);
        if(rawstream == NULL)
        {
            if(ninerr != NULL && ninerr->message != NULL)
            {
                nlog_error("Could not create bstream from new file descriptor. (%s)", ninerr->message);
            }
            else
            {
                nlog_error("Could not create bstream from new file descriptor.");
            }
        }

        usize write_buf_step_size = 1024;
        usize write_buf_initial_size = write_buf_step_size;
        usize write_frame_size = (usize) so_sndbuf;
        struct bstream *stream = bstream_buffered(rawstream, write_buf_step_size, write_buf_initial_size, write_frame_size);
        nlog_debug("Using write buffering for incoming connection with frame size: %zu, step size: %zu, initial size: %zu", 
            write_frame_size, write_buf_step_size, write_buf_initial_size);

        mcpr_connection *conn = mcpr_connection_new(stream);
        if(conn == NULL)
        {
            nlog_error("Could not create new connection object. (%s)", ninerr->message);
            bstream_decref(stream);
            continue;
        }
        mcpr_connection_set_packet_handler(conn, packet_handler);

        struct connection *conn2 = malloc(sizeof(struct connection));
        if(conn2 == NULL)
        {
            nlog_error("Could not allocate memory for connection. (%s)", strerror(errno));
            if(close(newfd) == -1) nlog_error("Error closing socket after memory allocation failure. (%s)", strerror(errno));
            mcpr_connection_decref(conn);
            bstream_decref(stream);
            continue;
        }
        conn2->player = NULL;
        conn2->conn = conn;
        conn2->fd = newfd;
        conn2->iostream = stream;
        conn2->server_address_used = NULL;
        conn2->tmp_present = false;

        if(slist_append(&clients, conn2) == NULL)
        {
            nlog_error("Could not add incoming connection to connection storage.");
            if(close(newfd) == -1) nlog_error("Error closing socket after previous error. (%s)", strerror(errno));
            mcpr_connection_decref(conn);
            bstream_decref(stream);
            free(conn2);
            continue;
        }
        client_count++;
        nlog_info("Socket with fd %d connected.", newfd);
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


            if(!mcpr_connection_write_packet(conn->conn, &keep_alive))
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

    if(!mcpr_connection_update(conn->conn))
    {
        nlog_error("Error whilst updating connection.");
        ninerr_print(ninerr);
        if(mcpr_connection_is_closed(conn->conn))
        {
            nlog_info("Client is closed, disconnecting.");
            connection_close(conn, NULL);
        }

        return;
    }

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

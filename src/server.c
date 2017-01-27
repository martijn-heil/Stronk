#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#include <algo/hash-table.h>
#include <algo/hash-string.h>
#include <algo/compare-string.h>

#include <mcpr/mcpr.h>
#include <mcpr/abstract_packet.h>

#include "server.h"
#include "stronk.h"
#include "util.h"

// Why the hell isn't this nanosleep defined anyway in Ubuntu/WSL?
// Oh well this hack works for now, even though it's terrible. TODO find out!
#if !defined(__USE_POSIX199309) && defined(NANOSLEEP_HACKY_FIX)
    extern int nanosleep (const struct timespec *__requested_time, struct timespec *__remaining);
#endif

// Function prototypes.
static int make_socket (uint16_t port);
static void setup_server_socket(void);
static void accept_incoming_connections(void);
static void serve_non_player_connection_batch(void *arg);
static void serve_non_player_connections(void);


static void server_tick(void);
static const long tick_duration_ns = 50000000; // Delay in nanoseconds, equivalent to 50 milliseconds

static int server_socket;

static HashTable *players; // hash table indexed by strings of compressed UUIDs

static size_t non_player_connection_count = 0;
static SListEntry *non_player_connections = NULL;


void server_shutdown(void)
{
    nlog_info("Shutting down server..");


    exit(EXIT_SUCCESS);
}

void server_crash(void)
{
    nlog_fatal("The server has crashed!");
    server_shutdown();
}

static int make_socket (uint16_t port) {
    struct sockaddr_in name;

    /* Create the socket. */
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        nlog_error("Could not create socket. (%s)", strerror(errno));
        return -1
    }

    /* Give the socket a name. */
    name.sin_family = AF_INET;
    name.sin_port = hton16(port);
    name.sin_addr.s_addr = hton32(INADDR_ANY);
    if (bind(sockfd, (struct sockaddr *) &name, sizeof (name)) < 0)
    {
        nlog_error("Could not bind socket to address. (%s)", strerror(errno));
        return -1;
    }

    return sockfd
}

static void setup_server_socket(void)
{
    int port = 25565;

    nlog_info("Creating server socket on port %i..", port);
    server_socket = make_socket(port);
    if(server_socket < 0)
    {
        nlog_fatal("Could not create server socket. (%s ?)", strerror(errno));
        exit(EXIT_FAILURE);
    }


    if(fcntl(server_socket, F_SETFL, O_NONBLOCK) == -1)
    {
        nlog_fatal("Could not set O_NONBLOCK flag for server socket. (%s)", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if(fcntl(server_socket, F_SETFL, O_ASYNC) == -1)
    {
        nlog_fatal("Could not set O_ASYNC flag for server socket. (%s)", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void server_start(void)
{
   nlog_info("Starting server..");
   setup_server_socket();

   players = hash_table_new(string_hash, string_equal);
   if(players == NULL)
   {
       nlog_fatal("Could not create hash table for player storage.");
       nlog_info("Cleaning up..");
       if(close(server_socket) == -1) nlog_error("Could not close server socket.");
       exit(EXIT_FAILURE);
   }

   // Main thread loop.
   while(true)
   {
       struct timespec start; // TODO C11 timespec_get() isn't implemented anywhere but in glibc.
       if(unlikely(!timespec_get(&start, TIME_UTC)))
       {
           nlog_error("Could not get current time in main game loop!");
           server_crash();
       }


       // Execute main game loop logic.
       server_tick();



       struct timespec should_stop_at;
       timespec_addraw(&should_stop_at, &start, 0, tick_duration_ns_ns);

       struct timespec stop;
       if(unlikely(!timespec_get(&stop, TIME_UTC)))
       {
           nlog_error("Could not get current time in main game loop!");
           server_crash();
       }

       if(timespec_cmp(&should_stop_at, &stop) > 0)
       {
           struct timespec diff;
           timespec_diff(&diff, &stop, &should_stop_at);
           if(nanosleep(&diff, NULL) == -1)
           {
               nlog_error("Sleeping in main game loop failed. (%s)", strerror(errno));
           }
       }
   }
}





static void accept_incoming_connections(void)
{
    while(true)
    {
        int newfd = accept(server_socket, NULL, NULL)
        if(newfd == -1)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK) // There are no incoming connections in the queue.
            {
                break;
            }
            else
            {
                nlog_error("Could not accept incoming connection. (%s)", strerror(errno));
                continue;
            }
        }

        if(fcntl(newfd, F_SETFL, O_NONBLOCK) == -1)
        {
            nlog_fatal("Could not set O_NONBLOCK flag for incoming connection. (%s)", strerror(errno));
            if(close(newfd) == -1) nlog_error("Could not clean up socket after memory allocation failure. (%s)", strerror(errno));
            continue;
        }

        if(fcntl(newfd, F_SETFL, O_ASYNC) == -1)
        {
            nlog_fatal("Could not set O_ASYNC flag for incoming connection. (%s)", strerror(errno));
            if(close(newfd) == -1) nlog_error("Could not clean up socket after memory allocation failure. (%s)", strerror(errno));
            continue;
        }

        struct connection *conn = malloc(sizeof(struct connection));
        if(conn == NULL)
        {
            nlog_error("Could not allocate memory for connection. (%s)", strerror(errno));
            if(close(newfd) == -1) nlog_error("Could not clean up socket after memory allocation failure. (%s)", strerror(errno));
            continue;
        }

        conn->fd = newfd;
        conn->state = MCPR_STATE_HANDSHAKE;
        conn->use_compression = false;
        conn->use_encryption = false;

        if(slist_append(non_player_connections, conn) == NULL)
        {
            nlog_error("Could not add incoming connection to connection storage.");
            if(close(newfd) == -1) nlog_error("Could not clean up socket after error. (%s)", strerror(errno));
            free(conn);
            continue;
        }
        non_player_connection_count++;
    }
}

static void serve_non_player_connection_batch(void *arg)
{
    SListEntry *first = *(SListEntry **) arg;
    unsigned int amount = *((unsigned int *) (arg + sizeof(SListEntry *)));


    for(int i = 0; i < amount; i++)
    {
        struct connection *conn = slist_nth_data(first, i);
        if(current == NULL)
        {
            nlog_error("Bad slist index.");
            break;
        }

        // TODO read packet.
        struct mcpr_abstract_packet *pkt = mcpr_fd_read_abstract_packet(conn->fd, conn->use_compression, conn->use_encryption, conn->encryption_block_size, conn->ctx_decrypt);

        switch(conn->state)
        {
            case MCPR_STATE_HANDSHAKE:
            {
                switch(pkt->id)
                {
                    case MCPR_PKT_HS_SB_HANDSHAKE:
                    {
                        int32_t protocol_version = pkt->data.handshake.serverbound.handshake.protocol_version;
                        conn->state = pkt->data.handshake.serverbound.handshake.next_state;

                        if(protocol_version != MCPR_PROTOCOL_VERSION)
                        {
                            if(conn->state == MCPR_STATE_LOGIN)
                            {
                                struct mcpr_abstract_packet response;
                                response.id = MCPR_PKT_LG_CB_DISCONNECT;
                                response.data.login.clientbound.disconnect.reason = mcpr_as_chat("Protocol version " PRId32 " is not supported.", protocol_version);

                                ssize_t result = mcpr_fd_write_abstract_packet(conn->fd, &response, conn->use_compression, false, conn->use_encryption,
                                    conn->use_encryption ? conn->encryption_block_size : 0, conn->use_encryption ? &(conn->ctx_encrypt) : NULL);

                                free(response.data.login.clientbound.disconnect.reason);

                                if(result < 0)
                                {
                                    nlog_error("Could not write packet to connection (%s ?), "
                                                "unable to disconnect client using incorrect protocol version", strerror(errno));
                                }
                            }
                        }
                    }
                }
            }
        }

        mcpr_free_abstract_packet(pkt);
    }

    free(arg);
}

static void serve_non_player_connections(void)
{
    unsigned int conns_per_thread = non_player_connection_count / main_threadpool_threadcount;
    unsigned int rest = non_player_connection_count % main_threadpool_threadcount;

    int index = 1;

    for(unsigned int i = 0; i < main_threadpool_threadcount, i++)
    {
        unsigned int amount = conns_per_thread;
        if(i == (main_threadpool_threadcount - 1)) amount += rest;

        SListEntry *arg1 = slist_nth_entry(non_player_connections, index);
        unsigned int arg2 = amount;
        void *args = malloc(sizeof(SListEntry *) + sizeof(unsigned int));
        memcpy(args, &arg1, sizeof(arg1));
        memcpy(args + sizeof(arg1), &arg2, sizeof(arg2));
        thpool_add_work(main_threadpool, serve_non_player_connection_batch, args);
        index += amount;
    }

    thpool_wait(main_threadpool);
}

static void server_tick(void)
{
    accept_incoming_connections();
    serve_non_player_connections();


    // Read packets from connected players..
}

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
#include <signal.h>

#include <zlog.h>
#include <jansson/jansson.h>
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
static void accept_incoming_connections(void);
static void serve_non_player_connection_batch(void *arg);
static void serve_non_player_connections(void);
static void init_networking(void);
void init_thread_pooling(void);
void init_logging(void);
void cleanup_logging(void);
void cleanup_thread_pooling(void);
void cleanup_networking();
void cleanup(void);
static void *secure_malloc(size_t size);
static void secure_free(void *ptr);
static int count_cores(void);


static void server_tick(void);
static const long tick_duration_ns = 50000000; // Delay in nanoseconds, equivalent to 50 milliseconds

static int server_socket;

static HashTable *players; // hash table indexed by strings of compressed UUIDs

static size_t non_player_connection_count = 0;
static SListEntry *non_player_connections = NULL;

static bool logging_init = false;
static bool thread_pooling_init = false;
static bool networking_init = false;

threadpool main_threadpool;
zlog_category_t *zc;

// returns 0 if unable to detect.
static int count_cores(void)
{
    // See http://stackoverflow.com/questions/150355/programmatically-find-the-number-of-cores-on-a-machine

    // ----------------------WARNING--------------------
    // If you change any of the preprocessor if statements below, also make sure the conditional #include's
    // on top of the file are right.

    // Cygwin, Linux, Solaris, AIX and Mac OS X >=10.4 (i.e. Tiger onwards)
    #if defined(__linux__) || defined(__sun) || defined(_AIX) || defined(__CYGWIN__) || (defined(__APPLE__) && defined(__MACH__))
        long num = sysconf(_SC_NPROCESSORS_ONLN);
        if(num > INT_MAX) { abort(); }
        return (int) num;

    // // Windows (Win32)
    // #elif defined(_WIN32)
    //     SYSTEM_INFO sysinfo;
    //     GetSystemInfo(&sysinfo);
    //     return sysinfo.dwNumberOfProcessors;

    // FreeBSD, MacOS X, NetBSD, OpenBSD, etc.
    #elif defined(BSD)
        int mib[4];
        int numCPU;
        size_t len = sizeof(numCPU);

        /* set the mib for hw.ncpu */
        mib[0] = CTL_HW;
        mib[1] = HW_AVAILCPU;  // alternatively, try HW_NCPU;

        /* get the number of CPUs from the system */
        sysctl(mib, 2, &numCPU, &len, NULL, 0);

        if (numCPU < 1)
        {
            mib[1] = HW_NCPU;
            sysctl(mib, 2, &numCPU, &len, NULL, 0);
            if (numCPU < 1)
                numCPU = 1;
        }

        return numCPU;

    // HP-UX
    #elif defined(__hpux)
        return mpctl(MPC_GETNUMSPUS, NULL, NULL);

    #else
        return 0; // couldn't detect.
    #endif
}

// Used for jansson, as it is mainly used for sensitive things.
static void *secure_malloc(size_t size)
{
    /* Store the memory area size in the beginning of the block */
    void *ptr = malloc(size + 8);
    *((size_t *)ptr) = size;
    return ptr + 8;
}

// Used for jansson, as it is mainly used for sensitive things.
static void secure_free(void *ptr)
{
    size_t size;

    ptr -= 8;
    size = *((size_t *)ptr);

    memset(ptr, 0, size + 8);
    free(ptr);
}

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

    return sockfd;
}

static void init_networking(void)
{
    int port = 25565;

    nlog_info("Creating server socket on port %i..", port);
    server_socket = make_socket(port);
    if(server_socket < 0)
    {
        nlog_fatal("Could not create server socket. (%s ?)", strerror(errno));
        cleanup();
        exit(EXIT_FAILURE);
    }


    if(fcntl(server_socket, F_SETFL, O_NONBLOCK) == -1)
    {
        nlog_fatal("Could not set O_NONBLOCK flag for server socket. (%s)", strerror(errno));
        cleanup();
        exit(EXIT_FAILURE);
    }

    // Ignore broken pipe signals, has to do with sockets.
    struct sigaction new_actn, old_actn;
    new_actn.sa_handler = SIG_IGN;
    sigemptyset (&new_actn.sa_mask);
    new_actn.sa_flags = 0;
    sigaction (SIGPIPE, &new_actn, &old_actn);

    networking_init = true;
}

void init_thread_pooling(void)
{
    nlog_info("Setting up thread pools..");

    nlog_info("Setting up thread pool..");
    int cpu_core_count = count_cores(); // Set up thread pool. -2 because we already have a main thread and a network thread.
    if(cpu_core_count <= 0)
    {
        cpu_core_count = 4;
        nlog_info("Could not detect amount of CPU cores, using 4 as a default.");
    } else {
        nlog_info("Detected %i CPU cores", cpu_core_count);
    }
    int planned_thread_count = cpu_core_count - 2; // -2 because we already have two threads, a main one and a network thread.
    if(planned_thread_count <= 0)
    {
        planned_thread_count = 2; // If we simply don't have enough cores, use 2 worker threads.
    }
    nlog_info("Creating thread pool with %i threads..", planned_thread_count);
    main_threadpool = thpool_init(planned_thread_count);

    if(main_threadpool == NULL)
    {
        nlog_fatal("Failed to create thread pool. (%s)?", strerror(errno));
        cleanup();
        exit(EXIT_FAILURE);
    }

    thread_pooling_init = true;
}

void init_logging(void)
{
    int zlog_status = zlog_init("/etc/zlog.conf");
    if(zlog_status)
    {
        fprintf(stderr, "Could not initialize zlog with /etc/zlog.conf (%s ?)\n", strerror(errno));
        cleanup();
        exit(EXIT_FAILURE);
    }

    zc = zlog_get_category("stronk");
    if(!zc)
    {
        fprintf(stderr, "Could not get category 'stronk' for zlog from /etc/zlog.conf, if you have not yet defined this category, define it.");
        zlog_fini();
        cleanup();
        exit(EXIT_FAILURE);
    }

    nlog_info("Logging system was successfully initialized.");

    nlog_info("Setting application locale to make sure we use UTF-8..");
    if(setlocale(LC_ALL, "") == NULL) // important, make sure we can use UTF-8.
    {
        nlog_warn("Could not set application locale to make sure we use UTF-8.");
    }

    logging_init = true;
}

void cleanup_logging(void)
{
    nlog_info("Closing log..");
    zlog_fini();
}

void cleanup_thread_pooling(void)
{
    nlog_info("Destroying thread pool..");
    thpool_destroy(main_threadpool);
}

void cleanup_networking(void)
{
    // TODO
}

void server_start(void)
{
    init_logging();
    init_thread_pooling();
    init_networking();

    nlog_info("Setting Jansson memory allocation/freeing functions to extra-safe variants..");
    json_set_alloc_funcs(secure_malloc, secure_free);

   players = hash_table_new(string_hash, string_equal);
   if(players == NULL)
   {
       nlog_fatal("Could not create hash table for player storage.");
       cleanup();
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

void cleanup(void)
{
    if(networking_init) cleanup_networking();
    if(thread_pooling_init) cleanup_thread_pooling();


    if(logging_init) cleanup_logging(); // Logging alwas as last.
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
            nlog_fatal("Could not set O_NONBLOCK flag for incoming connection. (%s)", strerror(errno));
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

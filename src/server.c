#ifndef _GNU_SOURCE
    #define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include <zlog.h>
#include <jansson/jansson.h>

#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/evp.h>

#include <algo/hash-table.h>
#include <algo/hash-string.h>
#include <algo/compare-string.h>

#include <mcpr/mcpr.h>
#include <mcpr/abstract_packet.h>
#include <mcpr/fdstreams.h>

#include "server.h"
#include "stronk.h"
#include "util.h"

/*
    <quote>

    In the GNU C Library, stdin, stdout, and stderr are normal variables which you can set just like any others. For example,
    to redirect the standard output to a file, you could do:
        fclose (stdout);
        stdout = fopen ("standard-output-file", "w");

    </quote>

    Source: http://www.gnu.org/software/libc/manual/html_node/Standard-Streams.html
*/
// TODO improve detecting, some systems will probably define __GNU_LIBRARY__ when they really don't use glibc.
#ifdef __GNU_LIBRARY__
    #define STANDARD_STREAMS_ASSIGNABLE 1
#else
    #define STANDARD_STREAMS_ASSIGNABLE 0
#endif

#ifndef HAVE_SECURE_RANDOM
#define HAVE_SECURE_RANDOM
    static ssize_t secure_random(void *buf, size_t len) {
        int urandomfd = open("/dev/urandom", O_RDONLY);
        if(urandomfd == -1) {
            return -1;
        }

        ssize_t urandomread = read(urandomfd, buf, len);
        if(urandomread == -1) {
            return -1;
        }
        if(urandomread != len) {
            return -1;
        }

        close(urandomfd);

        return len;
    }
#endif

// Why the hell isn't this nanosleep defined anyway in Ubuntu/WSL?
// Oh well this hack works for now, even though it's terrible. TODO find out!
#if !defined(__USE_POSIX199309) && defined(NANOSLEEP_HACKY_FIX)
    extern int nanosleep (const struct timespec *__requested_time, struct timespec *__remaining);
#endif

// Function prototypes.
static int make_socket (uint16_t port);
static void accept_incoming_connections(void);
static void serve_client_batch(void *arg);
static void serve_clients(void);
static void init_networking(void);
static void init_thread_pooling(void);
static void init_logging(void);
static void cleanup_logging(void);
static void cleanup_thread_pooling(void);
static void cleanup_networking();
static void cleanup(void);
static void *secure_malloc(size_t size);
static void secure_free(void *ptr);
static int count_cores(void);
static void disconnect_connection(struct connection *conn);
static void server_tick(void);


static const long tick_duration_ns = 50000000; // Delay in nanoseconds, equivalent to 50 milliseconds
static int server_socket;
static char *motd = "A bloody stronk server.";
static unsigned int max_players;
static HashTable *players = NULL; // hash table indexed by strings of compressed UUIDs
static size_t client_count = 0;
static pthread_mutex_t *clients_delete_lock;
static SListEntry *clients = NULL; // Clients which do not have a player object associated with them.
static bool logging_init = false;
static bool thread_pooling_init = false;
static bool networking_init = false;
threadpool main_threadpool;
zlog_category_t *zc;



// returns 0 if unable to detect.
static int count_cores(void)
{
    // See http://stackoverflow.com/questions/150355/programmatically-find-the-number-of-cores-on-a-machine

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

// Used for jansson, as it is used alot for sensitive things.
static void *secure_malloc(size_t size)
{
    /* Store the memory area size in the beginning of the block */
    void *ptr = malloc(size + 8);
    *((size_t *)ptr) = size;
    return ptr + 8;
}

// Used for jansson, as it is used alot for sensitive things.
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
    int port = 25565; // TODO configuration of port.

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

static void init_thread_pooling(void)
{
    nlog_info("Setting up thread pools..");

    nlog_info("Setting up thread pool..");
    int cpu_core_count = count_cores(); // Set up thread pool.
    if(cpu_core_count <= 0)
    {
        cpu_core_count = 4;
        nlog_info("Could not detect amount of CPU cores, using 4 as a default.");
    }
    else
    {
        nlog_info("Detected %i CPU cores", cpu_core_count);
    }
    int planned_thread_count = cpu_core_count;
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

#if STANDARD_STREAMS_ASSIGNABLE
    static ssize_t new_stdout_write(void *cookie, char *buf, size_t size)
    {
        char *new_buf = malloc(size + 1);
        if(new_buf == NULL) return 0;
        memcpy(new_buf, buf, size);
        new_buf[size] = '\0';

        nlog_info(new_buf);
        return size;
    }

    static ssize_t new_stderr_write(void *cookie, char *buf, size_t size)
    {
        char *new_buf = malloc(size + 1);
        if(new_buf == NULL) return 0;
        memcpy(new_buf, buf, size);

        // Remove newline at the end, if there is one.
        if(new_buf[size - 1] == '\n')
        {
            new_buf[size - 1] = '\0'
        }
        else
        {
            new_buf[size] = '\0';
        }

        nlog_error(new_buf);
        return size;
    }
#endif

static void init_logging(void)
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

    #if STANDARD_STREAMS_ASSIGNABLE
        nlog_info("Redirecting stderr and stdout to log..");

        cookie_io_functions_t new_stdout_funcs;
        new_stdout_funcs.write = new_stdout_write;
        new_stdout_funcs.read = NULL;
        new_stdout_funcs.seek = NULL;
        new_stdout_funcs.close = NULL;
        FILE *new_stdout = fopencookie(NULL, "a", new_stdout_funcs);
        if(new_stdout == NULL)
        {
            nlog_error("Could not redirect stdout to log.");
        }
        if(setvbuf(new_stdout, NULL, _IOLBF, 0) != 0) // Ensure line buffering.
        {
            nlog_error("Could not redirect stdout to log.");
            fclose(new_stdout);
        }
        stdout = new_stdout;

        cookie_io_functions_t new_stderr_funcs;
        new_stderr_funcs.write = new_stderr_write;
        new_stderr_funcs.read = NULL;
        new_stderr_funcs.seek = NULL;
        new_stderr_funcs.close = NULL;
        FILE *new_stderr = fopencookie(NULL, "a", new_stderr_funcs);
        if(new_stderr == NULL)
        {
            nlog_error("Could not redirect stderr to log.");
        }
        if(setvbuf(new_stderr, NULL, _IOLBF, 0) != 0) // Ensure line buffering.
        {
            nlog_error("Could not redirect stderr to log.");
            fclose(new_stderr);
        }
        stderr = new_stderr;
    #endif

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
    nlog_info("Cleaning up networking..");
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

   tmp_encryption_states = hash_table_new(pointer_hash, pointer_equal);
   if(tmp_encryption_states == NULL)
   {
       nlog_fatal("Could not create hash table.");
       cleanup();
       exit(EXIT_FAILURE);
   }

   if(pthread_mutex_init(clients_delete_lock, NULL) != 0)
   {
       nlog_fatal("Could not initialize mutex.");
       hash_table_free(players);
       cleanup();
       exit(EXIT_FAILURE);
   }



   nlog_info("Startup done!");

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
        conn->ctx_encrypt = NULL;
        conn->ctx_decrypt = NULL;

        if(slist_append(clients, conn) == NULL)
        {
            nlog_error("Could not add incoming connection to connection storage.");
            if(close(newfd) == -1) nlog_error("Could not clean up socket after error. (%s)", strerror(errno));
            free(conn);
            continue;
        }
        client_count++;
    }
}


static HashTable *tmp_encryption_states = NULL;
static struct tmp_encryption_state
{
    int32_t verify_token_length;
    void *verify_token;
    RSA *rsa;
};

static void serve_client_batch(void *arg)
{
    SListEntry *first = *(SListEntry **) arg;
    unsigned int amount = *((unsigned int *) (arg + sizeof(SListEntry *)));


    for(int i = 0; i < amount; i++)
    {
        SListEntry *current_entry = slist_nth_entry(first, i);
        struct connection *conn = slist_data(current_entry);
        if(current == NULL)
        {
            nlog_error("Bad slist index.");
            break;
        }


        // Can be called multiple times in the scope.
        // Returns ssize_t less than 0 upon error.
        // Sets mcpr_errno upon error.
        // May be used like a function.
        #define WRITE_PACKET(pkt_123)
            mcpr_fd_write_abstract_packet(conn->fd, pkt_123, conn->use_compression, false, conn->use_compression ? conn->compression_treshold : 0, \
                conn->use_encryption, conn->use_encryption ? conn->encryption_block_size : 0, conn->use_encryption ? conn->ctx_encrypt : NULL); \


        // Should only be called once.
        // Should not be used like a normal function call.
        #define CLOSE_CURRENT_CONN() \
            if(close(conn->fd) == -1) nlog_error("Error whilst closing socket. (%s)", mcpr_strerror(errno)); \
            pthread_mutex_lock(clients_delete_lock); \
            slist_remove_entry(&first, current_entry); \
            pthread_mutex_unlock(clients_delete_lock); \
            if(conn->ctx_encrypt != NULL) EVP_CIPHER_CTX_cleanup(conn->ctx_encrypt); free(conn->ctx_encrypt); \
            if(conn->ctx_decrypt != NULL) EVP_CIPHER_CTX_cleanup(conn->ctx_decrypt); free(conn->ctx_decrypt); \
            free(conn);

        // Should only be called once.
        // Should not be used like a normal function call.
        #define DISCONNECT_CURRENT_CONN(_reason) \
            if(conn->state == MCPR_STATE_LOGIN || conn->state == MCPR_STATE_PLAY) \
            { \
                struct mcpr_abstract_packet _response; \
                _response.id = ((conn->state == MCPR_STATE_LOGIN) ? MCPR_PKT_LG_CB_DISCONNECT : MCPR_PKT_PL_CB_DISCONNECT); \
                _response.data.login.clientbound.disconnect.reason = _reason; \
                if(WRITE_PACKET(&_response) < 0) \
                { \
                    nlog_error("Could not write packet to connection (%s ?), " \
                                "unable to properly disconnect client.", mcpr_strerror(mcpr_errno)); \
                } \
            } \
            CLOSE_CURRENT_CONN();



        struct mcpr_abstract_packet *pkt = mcpr_fd_read_abstract_packet(conn->fd, conn->use_compression, conn->use_compression ? conn->compression_treshold : 0,
             conn->use_encryption, conn->encryption_block_size, conn->ctx_decrypt);

        if(pkt == NULL)
        {
            if(mcpr_errno == MCPR_ECOMPRESSTHRESHOLD)
            {
                char *reason = mcpr_as_chat("Compression treshold was violated.");
                DISCONNECT_CURRENT_CONN();
                free(reason);
            }

            nlog_error("Could not read packet from client. (%s ?)", strerror(errno));
        }



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
                                char *reason = mcpr_as_chat("Protocol version " PRId32 " is not supported! I'm on protocol version %i (MC %s)", protocol_version, MCPR_PROTOCOL_VERSION, MCPR_MINECRAFT_VERSION);
                                DISCONNECT_CURRENT_CONN(reason);
                                free(reason);
                            }
                        }
                    }
                }
            }

            case MCPR_STATE_STATUS:
            {
                switch(pkt->id)
                {
                    case MCPR_PK_ST_SB_REQUEST:
                    {
                        struct mcpr_abstract_packet response;
                        response.id = MCPR_PKT_ST_SB_RESPONSE;
                        response.data.status.clientbound.response.version_name = MCPR_MINECRAFT_VERSION;
                        response.data.status.clientbound.response.protocol_version = MCPR_PROTOCOL_VERSION;
                        response.data.status.clientbound.response.max_players = max_players;
                        response.data.status.clientbound.response.online_players_size = hash_table_num_entries(players);
                        response.data.status.clientbound.response.description = motd;
                        response.data.status.clientbound.response.online_players = NULL;
                        response.data.status.clientbound.response.favicon = NULL;

                        ssize_t result = mcpr_fd_write_abstract_packet(conn->fd, &response, conn->use_compression, false, conn->use_encryption,\
                            conn->use_encryption ? conn->encryption_block_size : 0, conn->use_encryption ? &(conn->ctx_encrypt) : NULL);\

                        if(result < 0)
                        {
                            if(mcpr_errno == ECONNRESET)
                            {
                                CLOSE_CURRENT_CONN();
                                break;
                            }
                            else
                            {
                                nlog_error("Could not write packet to connection (%s ?)", mcpr_strerror(errno));
                                break;
                            }
                        }
                    }

                    case MCPR_PKT_ST_SB_PING:
                    {
                        struct mcpr_abstract_packet response;
                        response.id = MCPR_PKT_ST_CB_PONG;
                        response.data.status.clientbound.pong.payload = pkt.data.status.serverbound.ping.payload;

                        if(WRITE_PACKET(&response) < 0)
                        {
                            if(mcpr_errno == ECONNRESET)
                            {
                                CLOSE_CURRENT_CONN();
                                break;
                            }
                            else
                            {
                                nlog_error("Could not write packet to connection (%s ?)", mcpr_strerror(errno));
                                break;
                            }
                        }
                    }
                }
            }

            case MCPR_STATE_LOGIN:
            {
                case MCPR_PKT_LG_SB_LOGIN_START:
                {
                    RSA *rsa = RSA_generate_key(1024, 3, 0, 0); // TODO this is deprecated in OpenSSL 1.0.2? But the alternative is not there in 1.0.1

                    unsigned char *buf = NULL;
                    int buflen = i2d_RSA_PUBKEY(rsa, &buf);
                    if(buflen < 0)
                    {
                        // Error occured.
                        nlog_error("Ermg ze openssl errorz!!"); // TODO proper error handling.
                        break;
                    }
                    if(buflen > INT32_MAX || buflen < INT32_LEAST) // TODO is INT32_LEAST a thing?
                    {
                        nlog_error("Integer overflow.");
                        RSA_free(rsa);
                        break;
                    }

                    int32_t verify_token_length = 16;
                    uint8_t *verify_token = malloc(verify_token_length * sizeof(uint8_t)); // 128 bit verify token.
                    if(verify_token == NULL)
                    {
                        nlog_error("Could not allocate memory. (%s)", strerror(errno));
                        RSA_free(rsa);
                        break;
                    }

                    if(secure_random(verify_token, verify_token_length) < 0)
                    {
                        nlog_error("Could not get random data.");
                        RSA_free(rsa);
                        break;
                    }


                    struct mcpr_abstract_packet response;
                    response.id = MCPR_PKT_LG_CB_ENCRYPTION_REQUEST;
                    response.data.login.clientbound.encryption_request.server_id = ""; // Yes that's supposed to be an empty string.
                    response.data.login.clientbound.encryption_request.public_key_length = (int32_t) buflen;
                    response.data.login.clientbound.encryption_request.public_key = buf;
                    response.data.login.clientbound.encryption_request.verify_token_length = verify_token_length;
                    response.data.login.clientbound.encryption_request.verify_token = verify_token;


                    if(WRITE_PACKET(&response) < 0)
                    {
                        if(mcpr_errno == ECONNRESET)
                        {
                            CLOSE_CURRENT_CONN();
                            RSA_free(rsa);
                            break;
                        }
                        else
                        {
                            nlog_error("Could not write packet to connection (%s ?)", mcpr_strerror(errno));
                            RSA_free(rsa);
                            break;
                        }
                    }

                    struct tmp_encryption_state *tmp_state = malloc(sizeof(struct tmp_encryption_state))
                    if(tmp_state == NULL)
                    {
                        nlog_error("Could not allocate memory (%s)", strerror(errno));
                        RSA_free(rsa);
                        break;
                    }

                    tmp_state->rsa = rsa;
                    tmp_state->verify_token_length = verify_token_length;
                    tmp_state->verify_token = verify_token;


                    if(hash_table_insert(tmp_encryption_states, conn, tmp_state) == 0)
                    {
                        nlog_error("Could not insert entry into hash table. (%s ?)", strerror(errno));
                        RSA_free(rsa);
                        break;
                    }
                }

                case MCPR_PKT_LG_SB_ENCRYPTION_RESPONSE:
                {
                    struct tmp_encryption_state *tmp_state = NULL;
                    void *decrypted_shared_secret = NULL;


                    tmp_state = hash_table_lookup(tmp_encryption_states, conn);
                    if(tmp_state == HASH_TABLE_NULL) // Shouldn't happen.
                    {
                        nlog_error("Could not find entry in hash table.");
                        char *reason = mcpr_as_chat("An error occurred whilst setting up encryption.");
                        DISCONNECT_CURRENT_CONN(reason);
                        free(reason);
                    }
                    RSA *rsa = tmp_state->rsa;

                    int32_t shared_secret_length = pkt->data.login.serverbound.encryption_response.shared_secret_length;
                    void *shared_secret = pkt->data.login.serverbound.encryption_response.shared_secret;

                    if(shared_secret_length > INT_MAX)
                    {
                        nlog_error("Integer overflow.");
                        goto err;
                    }

                    decrypted_shared_secret = malloc(RSA_size(rsa));
                    if(decrypted_shared_secret == NULL)
                    {
                        nlog_error("Could not allocate memory. (%s)", strerror(errno));
                        goto err;
                    }

                    int size = RSA_private_decrypt((int) shared_secret_length, (unsigned char *) shared_secret, (unsigned char *) decrypted_shared_secret, rsa, RSA_PKCS1_PADDING);
                    if(size < 0)
                    {
                        nlog_error("Could not decrypt shared secret."); // TODO proper error handling.
                        goto err;
                    }

                    conn->ctx_encrypt = malloc(sizeof(conn->ctx_encrypt));
                    if(conn->ctx_encrypt == NULL)
                    {
                        nlog_error("Could not allocate memory. (%s)", strerror(errno));
                        goto err;
                    }
                    EVP_CIPHER_CTX_init(conn->ctx_encrypt);
                    if(EVP_EncryptInit_ex(&(conn->ctx_encrypt), EVP_aes_128_cfb8(),
                     NULL, (unsigned char *) decrypted_shared_secret, (unsigned char *) decrypted_shared_secret) == 0) // TODO Should those 2 pointers be seperate buffers of shared secret?
                     {
                        nlog_error("Error upon EVP_EncryptInit_ex()."); // TODO proper error handling.
                        goto err;
                    }

                    conn->ctx_decrypt = malloc(sizeof(conn->ctx_decrypt));
                    if(conn->ctx_decrypt == NULL)
                    {
                        nlog_error("Could not allocate memory. (%s)", strerror(errno));
                        goto err;
                    }
                    EVP_CIPHER_CTX_init(conn->ctx_decrypt);
                    if(EVP_DecryptInit_ex(conn->ctx_decrypt), EVP_aes_128_cfb8(),
                     NULL, (unsigned char *) decrypted_shared_secret, (unsigned char *) decrypted_shared_secret) == 0) // TODO Should those 2 pointers be seperate buffers of shared secret?
                     {
                        nlog_error("Error upon EVP_DecryptInit_ex()."); // TODO proper error handling.
                        goto err;
                    }


                    hash_table_remove(tmp_encryption_states, conn);
                    RSA_free(rsa);
                    free(tmp_state->verify_token);
                    free(tmp_state);
                    free(decrypted_shared_secret);
                    break;

                    err:
                        char *reason = mcpr_as_chat("An error occurred whilst setting up encryption.");
                        DISCONNECT_CURRENT_CONN(reason);
                        free(reason);
                        hash_table_remove(tmp_encryption_states, conn);
                        RSA_free(rsa);
                        free(tmp_state->verify_token);
                        free(tmp_state);
                        free(decrypted_shared_secret);
                        break;
                }
            }
        }

        mcpr_free_abstract_packet(pkt);
    }

    free(arg);

    #undef CLOSE_CURRENT_CONN
    #undef DISCONNECT_CURRENT_CONN
    #undef WRITE_PACKET
}

static void serve_clients(void)
{
    unsigned int conns_per_thread = client_count / main_threadpool_threadcount;
    unsigned int rest = client_count % main_threadpool_threadcount;

    int index = 1;

    for(unsigned int i = 0; i < main_threadpool_threadcount, i++)
    {
        unsigned int amount = conns_per_thread;
        if(i == (main_threadpool_threadcount - 1)) amount += rest;

        SListEntry *arg1 = slist_nth_entry(clients, index);
        unsigned int arg2 = amount;
        void *args = malloc(sizeof(SListEntry *) + sizeof(unsigned int));
        memcpy(args, &arg1, sizeof(arg1));
        memcpy(args + sizeof(arg1), &arg2, sizeof(arg2));
        thpool_add_work(main_threadpool, serve_client_batch, args);
        index += amount;
    }

    thpool_wait(main_threadpool);
}

static void server_tick(void)
{
    accept_incoming_connections();
    serve_clients();


    // Read packets from connected players..
}

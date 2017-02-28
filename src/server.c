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

#include <jansson/jansson.h>

#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

#include <algo/hash-table.h>
#include <algo/hash-string.h>
#include <algo/compare-string.h>

#include <mcpr/mcpr.h>
#include <mcpr/abstract_packet.h>
#include <mcpr/fdstreams.h>

#include <logging/logging.h>

#include "server.h"
#include "stronk.h"
#include "util.h"


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
static void init_thread_pooling(void);
static void cleanup_thread_pooling(void);
static void cleanup(void);
static void *secure_malloc(size_t size);
static void secure_free(void *ptr);
static int count_cores(void);
static void server_tick(void);
static void init(void);


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
static struct timespec internal_clock; // Every time the clock ticks 50ms is added



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

static void init_thread_pooling(void)
{
    nlog_info("Setting up thread pools..");

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

static void init(void)
{
    if(logging_init() < 0) { cleanup(); exit(EXIT_FAILURE); }
    init_thread_pooling();
    if(net_init() < 0) { cleanup(); exit(EXIT_FAILURE); }


    nlog_info("Setting Jansson memory allocation/freeing functions to extra-safe variants..");
    json_set_alloc_funcs(secure_malloc, secure_free);

    players = hash_table_new(string_hash, string_equal);
    if(players == NULL)
    {
        nlog_fatal("Could not create hash table for player storage.");
        cleanup();
        exit(EXIT_FAILURE);
    }
}

void server_start(void)
{
   init();
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
    if(networking_init) net_cleanup();
    if(thread_pooling_init) cleanup_thread_pooling();


    if(logging_init) logging_cleanup(); // Logging alwas as last.
}

static void server_tick(void)
{
    net_tick();


    // Do this last.
    struct timespec addend;
    addend.tv_sec = 0;
    addend.tv_nsec = tick_duration_ns;
    timespec_add(&internal_clock, &internal_clock, &addend);
}

struct timespec server_get_internal_clock_time(struct timespec *out)
{
    out->tv_sec = internal_clock.tv_sec;
    out->tv_nsec = internal_clock.tv_nsec;
}

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

#ifndef _GNU_SOURCE
    #define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

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

#include <openssl/ssl.h>
#include <openssl/opensslv.h>

#include <algo/hash-table.h>
#include <algo/hash-string.h>
#include <algo/compare-string.h>

#include <curl/curl.h>

#include <ninerr/ninerr.h>
#include <ninio/logging.h>
#include <mapi/mapi.h>
#include <mcpr/mcpr.h>

#include <logging/logging.h>
#include <network/network.h>
#include <world/world.h>

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
static bool world_manager_init_done = false;
static bool logging_init_done = false;
static bool thread_pooling_init_done = false;
static bool networking_init_done = false;
static bool curl_init_done = false;
static bool openssl_init_done = false;

static bool internal_clock_init_done = false;
pthread_rwlock_t internal_clock_lock;
static struct timespec internal_clock; // Every time the clock ticks 50ms is added

unsigned int main_threadpool_threadcount;
threadpool main_threadpool;

unsigned int async_threadpool_threadcount;
threadpool async_threadpool;


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
// Even here at Stronk, security is more important than performance :P
#ifdef __GNUC__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpointer-arith"
#endif
static void *secure_malloc(size_t size)
{
    /* Store the memory area size in the beginning of the block */
    void *ptr = malloc(size + sizeof(size_t));
    *((size_t *)ptr) = size;
    return ptr + sizeof(size_t);
}

// Used for jansson, as it is used alot for sensitive things.
// Even here at Stronk, security is more important than performance :P
static void secure_free(void *ptr)
{
    size_t size;

    ptr -= sizeof(size_t);
    size = *((size_t *)ptr);

    memset(ptr, 0, size + sizeof(size_t));
    free(ptr);
}
#ifdef __GNUC__
    #pragma GCC diagnostic pop
#endif

void server_shutdown(int status)
{
    nlog_info("Shutting down server..");
    exit(status);
}

void server_crash(void)
{
    nlog_fatal("The server has crashed!");
    server_shutdown(EXIT_FAILURE);
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
    main_threadpool_threadcount = planned_thread_count;

    thread_pooling_init_done = true;
}

void cleanup_thread_pooling(void)
{
    nlog_info("Destroying thread pool..");
    thpool_destroy(main_threadpool);
}

void cleanup(void)
{
    if(networking_init_done) net_cleanup();
    if(thread_pooling_init_done) cleanup_thread_pooling();
    if(world_manager_init_done) world_manager_cleanup();

    if(internal_clock_init_done) pthread_rwlock_destroy(&internal_clock_lock);
    if(curl_init_done) { nlog_info("Cleaning up CURL.."); curl_global_cleanup(); }
    if(openssl_init_done) { nlog_info("Cleaning up OpenSSL.."); EVP_cleanup(); } // make sure to do this after CURL cleanup.

    if(logging_init_done) logging_cleanup(); // Logging alwas as last, apart from ninerr
    ninerr_finish();
}

static void init(void)
{
    if(!ninerr_init()) { fprintf(stderr, "Could not initialize ninerr.\n"); exit(EXIT_FAILURE); }
    if(logging_init() < 0) { cleanup(); ninerr_finish(); fprintf(stderr, "Could not initialize logging module.\n"); exit(EXIT_FAILURE); }
    mapi_logger = nlogger;
    mcpr_logger = nlogger;

    nlog_info("Starting Stronk (Minecraft version: %s, Protocol version: %u)", MCPR_MINECRAFT_VERSION, MCPR_PROTOCOL_VERSION);

    // TODO catch SIGINT signals and make sure cleanup is performed.
    if(atexit(cleanup) != 0) nlog_warn("Could not register atexit cleanup function, shutdown and/or crashing may not be graceful, and may cause data loss!");

    nlog_info("Intializing OpenSSL..");\
    OpenSSL_add_all_algorithms();
	SSL_load_error_strings();
	SSL_library_init();
    openssl_init_done = true;

    // Important we initialize CURL before we start more threads!
    nlog_info("Initializing CURL..");
    if(curl_global_init(CURL_GLOBAL_ALL) != 0)
    {
        nlog_fatal("Could not initialize CURL!");
        exit(EXIT_FAILURE);
    }
    curl_init_done = true;

    nlog_info("Initializing some synchronization locks..");
    if(pthread_rwlock_init(&internal_clock_lock, NULL) != 0)
    {
        nlog_fatal("Could not initialize internal clock lock.");
        cleanup();
        exit(EXIT_FAILURE);
    }
    internal_clock_init_done = true;

    init_thread_pooling();
    if(net_init() < 0)              { exit(EXIT_FAILURE); } networking_init_done = true;
    if(world_manager_init() < 0)    { exit(EXIT_FAILURE); } world_manager_init_done = true;


    nlog_info("Setting Jansson memory allocation/freeing functions to extra-safe variants..");
    json_set_alloc_funcs(secure_malloc, secure_free);
}

void server_start(void)
{
   init();
   nlog_info("Startup done!");

   // Main thread loop.
   while(true)
   {
       struct timespec start; // TODO C11 timespec_get() isn't implemented anywhere but in glibc.
       if(!timespec_get(&start, TIME_UTC))
       {
           nlog_error("Could not get current time in main game loop!");
           server_crash();
       }


       // Execute main game loop logic.
       server_tick();



       struct timespec should_stop_at;
       timespec_addraw(&should_stop_at, &start, 0, tick_duration_ns);

       struct timespec stop;
       if(!timespec_get(&stop, TIME_UTC))
       {
           nlog_error("Could not get current time in main game loop!");
           server_crash();
       }

       if(timespec_cmp(&should_stop_at, &stop) > 0)
       {
           struct timespec diff;
           timespec_diff(&diff, &stop, &should_stop_at);
           //nlog_debug("Sleeping for %lld.%.9ld", (long long) diff.tv_sec, diff.tv_nsec);
           if(nanosleep(&diff, NULL) == -1)
           {
               nlog_error("Sleeping in main game loop failed. (%s)", strerror(errno));
           }
       }
   }
}

static void server_tick(void)
{
    //nlog_debug("Current internal clock: %lu : %lu", internal_clock.tv_sec, internal_clock.tv_nsec);
    net_tick();


    // Do this last.
    struct timespec addend;
    addend.tv_sec = 0;
    addend.tv_nsec = tick_duration_ns;
    timespec_add(&internal_clock, &internal_clock, &addend);
}

void server_get_internal_clock_time(struct timespec *out)
{
    int result;
    again:
    result = pthread_rwlock_rdlock(&internal_clock_lock);
    if(result != 0) { nlog_debug("Could not lock internal clock lock! Retrying.."); goto again; }

    out->tv_sec = internal_clock.tv_sec;
    out->tv_nsec = internal_clock.tv_nsec;

    pthread_rwlock_unlock(&internal_clock_lock);
}

#ifdef _WIN32
    #include <windows.h>
#endif

#if defined(__linux__) || defined(__sun) || defined(_AIX) || defined(__CYGWIN__) || (defined(__APPLE__) && defined(__MACH__))
    #include <unistd.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <errno.h>
#include <string.h>

#include <zlog.h>
#include <jansson/jansson.h>

#include "stronk.h"
#include "util.h"
#include "mcpr/mcpr.h"
#include "server.h"

threadpool thpool;
zlog_category_t *zc;


/*
    System & architecture requirements:

    C11 (including variable length arrays)
    POSIX (mainly the C library though)
    Pthreads must be available.
    Preferrably GCC, but other compilers should work too.

    2's complement integers.
    char and unsigned char should be exactly 8 bits wide.

    endianness has to be either little endian or big endian, no middle endian.
*/

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

// returns 0 if unable to detect.
int count_cores(void) {
    // See http://stackoverflow.com/questions/150355/programmatically-find-the-number-of-cores-on-a-machine

    // ----------------------WARNING--------------------
    // If you change any of the preprocessor if statements below, also make sure the conditional #include's
    // on top of the file are right.

    // Cygwin, Linux, Solaris, AIX and Mac OS X >=10.4 (i.e. Tiger onwards)
    #if defined(__linux__) || defined(__sun) || defined(_AIX) || defined(__CYGWIN__) || (defined(__APPLE__) && defined(__MACH__))
        long num = sysconf(_SC_NPROCESSORS_ONLN);
        if(num > INT_MAX) { abort(); }
        return (int) num;

    // Windows (Win32)
    #elif defined(_WIN32)
        SYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo);
        return sysinfo.dwNumberOfProcessors;

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

void cleanup(void) {
    nlog_info("Cleaning up..");

    nlog_info("Destroying thread pool..");
    thpool_destroy(thpool);

    nlog_info("Closing zlog..");
    zlog_fini();
}

int main(void) {
    // TODO test safe_math.h functions..
    int zlog_status = zlog_init("/etc/zlog.conf");
    if(zlog_status) {
        fprintf(stderr, "Could not initialize zlog with /etc/zlog.conf (%s ?)\n", strerror(errno));
        return EXIT_FAILURE;
    }

    zc = zlog_get_category("stronk");
    if(!zc) {
        fprintf(stderr, "Could not get category 'stronk' for zlog from /etc/zlog.conf, if you have not yet defined this category, define it.");
        return EXIT_FAILURE;
    }

    nlog_info("Setting application locale to make sure we use UTF-8..");
    setlocale(LC_ALL, ""); // important, make sure we can use UTF-8.


    nlog_info("Setting up thread pool..");
    int cpu_core_count = count_cores(); // Set up thread pool. -2 because we already have a main thread and a network thread.
    if(cpu_core_count <= 0) {
        cpu_core_count = 4;
        nlog_info("Could not detect amount of CPU cores, using 4 as a default.");
    } else {
        nlog_info("Detected %i CPU cores", cpu_core_count);
    }
    int planned_thread_count = cpu_core_count - 2; // -2 because we already have two threads, a main one and a network thread.
    if(planned_thread_count <= 0) {
        planned_thread_count = 2; // If we simply don't have enough cores, use 2 worker threads.
    }
    nlog_info("Creating thread pool with %i threads..", planned_thread_count);
    thpool = thpool_init(planned_thread_count);

    if(thpool == NULL) {
        nlog_fatal("Failed to create thread pool. (%s)?", strerror(errno));
        return EXIT_FAILURE;
    }

    nlog_info("Setting Jansson memory allocation/freeing functions to extra-safe variants..");
    json_set_alloc_funcs(secure_malloc, secure_free);

    server_start();

    cleanup();
    return EXIT_SUCCESS;
}

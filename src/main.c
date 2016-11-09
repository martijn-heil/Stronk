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

#include "stronk.h"
#include "util.h"
#include "mcpr.h"

threadpool thpool;
zlog_category_t *zc;


/*
    System requirements:
    C11 (including variable length arrays)
    POSIX
*/

// returns 0 if unable to detect.
int count_cores(void) {
    // See http://stackoverflow.com/questions/150355/programmatically-find-the-number-of-cores-on-a-machine

    // ----------------------WARNING--------------------
    // If you change any of the preprocessor if statements below, also make sure the conditional #include's
    // on top of the file are right.

    // Cygwin, Linux, Solaris, AIX and Mac OS X >=10.4 (i.e. Tiger onwards)
    #if defined(__linux__) || defined(__sun) || defined(_AIX) || defined(__CYGWIN__) || (defined(__APPLE__) && defined(__MACH__))
        return sysconf(_SC_NPROCESSORS_ONLN);

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
    nlog_info("Test nlog %i", 1);
    int zlog_status = zlog_init("/etc/zlog.conf");
    if(zlog_status) {
        fprintf(stderr, "Could not initialize zlog with /etc/zlog.conf (%s ?)\n", strerror(errno));
        return EXIT_FAILURE;
    }

    zc = zlog_get_category("stronk");
    if(!zc) {
        fprintf(stderr, "Could not get category 'stronk' for zlog from /etc/zlog.conf");
        return EXIT_FAILURE;
    }

    nlog_info("Setting application locale to make sure we use UTF-8..");
    setlocale(LC_ALL, ""); // important, make sure we can use UTF-8.


    nlog_info("Setting up thread pool..");
    int numcores = count_cores(); // Set up thread pool. -2 because we already have a main thread and a network thread.
    if(numcores <= 0) {
        numcores = 4;
        nlog_info("Could not detect amount of CPU cores, using 4 as a default.");
    } else {
        nlog_info("Detected %i CPU cores", numcores);
    }
    int numthreads = numcores - 2; // -2 because we already have two threads, a main one and a network thread.
    if(numthreads <= 0) {
        numthreads = 2; // If we simply don't have enough cores, use 2 worker threads.
    }
    nlog_info("Creating thread pool with %i threads..", numthreads);
    thpool = thpool_init(numthreads);

    if(thpool == NULL) {
        nlog_fatal("Failed to create thread pool. (%s)?", strerror(errno));
        return EXIT_FAILURE;
    }

    cleanup();
    return EXIT_SUCCESS;
    // char *test = u8"♥";
    // puts(test);
    // printf("%zu\n", strlen(test));
    //
    // pthread_t thread1;
    // int  iret1;
    //
    // /* Create independent threads each of which will execute function */
    //
    // iret1 = pthread_create(&thread1, NULL, doFancyStuffs, NULL);
    // if(iret1)
    // {
    //     fprintf(stderr,"Error - pthread_create() return code: %d\n",iret1);
    //     exit(EXIT_FAILURE);
    // }
    //
    // /* Wait till threads are complete before main continues. Unless we  */
    // /* wait we run the risk of executing an exit which will terminate   */
    // /* the process and all threads before the threads have completed.   */
    //
    // pthread_join(thread1, NULL);
}

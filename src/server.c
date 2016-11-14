#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#include "server.h"
#include "stronk.h"
#include "util.h"

// Why the hell isn't this nanosleep defined anyway in Ubuntu/WSL?
// Oh well this hack works for now, even though it's terrible. TODO find out!
#if !defined(__USE_POSIX199309) && defined(NANOSLEEP_HACKY_FIX)
    extern int nanosleep (const struct timespec *__requested_time, struct timespec *__remaining);
#endif


bool shouldShutdown = false;
static long ticktime = 50000000; // Delay in nanoseconds.
int tmp = 0;


void server_shutdown() {
    nlog_info("Shutting down server..");
}

void server_crash() {
    server_shutdown();
    exit(EXIT_FAILURE);
}


void server_tick() {

}


void server_start() {
    nlog_info("Starting server..");

    // Main thread loop.
    while(!shouldShutdown) {
        struct timespec start;
        if(unlikely(!timespec_get(&start, TIME_UTC))) {
            nlog_error("Could not get current time in main game loop!");
            server_crash();
        }


        // Execute main game loop logic.
        server_tick();



        struct timespec should_stop_at;
        timespec_addraw(&should_stop_at, &start, 0, ticktime);

        struct timespec stop;
        if(unlikely(!timespec_get(&stop, TIME_UTC))) {
            nlog_error("Could not get current time in main game loop!");
            server_crash();
        }

        if(timespec_cmp_g(&should_stop_at, &stop)) {
            struct timespec diff;
            timespec_diff(&diff, &stop, &should_stop_at);
            nanosleep(&diff, NULL); // TODO error checking.
        }
    }

    server_shutdown();
}

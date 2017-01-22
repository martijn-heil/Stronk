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

static void void server_tick(void);


static const long tick_duration = 50000000; // Delay in nanoseconds, equivalent to 50 milliseconds


void server_shutdown()
{
    nlog_info("Shutting down server..");


    exit(EXIT_SUCCESS);
}

void server_crash(void)
{
    nlog_fatal("The server has crashed!");
    server_shutdown();
}


void server_start(void)
{
   nlog_info("Starting server..");

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
       timespec_addraw(&should_stop_at, &start, 0, tick_duration);

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







void server_tick(void)
{

}

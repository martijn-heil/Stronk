#include <stdbool.h>
#include <time.h>
#include <syslog.h>

#include "server.h"

// bool shouldShutdown = false;
//
// void server_start() {
//     syslog(LOG_INFO, "Starting server..");
//
//     // Main thread loop.
//     while(!shouldShutdown) {
//         struct timespec tms;
//         if(!timespec_get(&tms, TIME_UTC)) {
//             syslog(LOG_CRIT, "Could not get current time in main game loop!");
//             abort();
//         }
//     }
//
//     server_shutdown();
// }
//
// void server_shutdown() {
//     shouldShutdown = true;
//     syslog(LOG_INFO, "Shutting down server..");
//
// }
//
// void server_crash() {
//     server_shutdown();
// }

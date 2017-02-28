#ifndef STRONK_SERVER_H
#define STRONK_SERVER_H

#include <time.h>

void server_start(void);
struct timespec server_get_internal_clock_time(struct timespec *out);

#endif

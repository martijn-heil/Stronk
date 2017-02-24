#ifndef STRONK_ENTITY_H
#define STRONK_ENTITY_H

#include <pthread.h>
#include <stdint.h>

extern pthread_mutex_t *entity_id_counter_lock;
extern int32_t entity_id_counter;

#endif

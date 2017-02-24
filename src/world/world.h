#ifndef WORLD_H
#define WORLD_H

typedef void * world;
typedef void * block;

#include <network/player.h>

void world_do_tick(world *world);
size_t world_manager_get_world_count();
world **world_manager_get_worlds();
struct fposition world_manager_get_init_spawn_pos(void);
int world_send_chunk_data(player *p);

#endif

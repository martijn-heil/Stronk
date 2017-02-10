#ifndef WORLD_H
#define WORLD_H

typedef void * world;
typedef void * block;



void world_do_tick(world *world);
size_t world_manager_get_world_count();
world **world_manager_get_worlds();

#endif

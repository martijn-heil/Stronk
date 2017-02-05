#ifndef WORLD_H
#define WORLD_H

typedef void * world;



void world_do_tick(world *world);
void world_generate_chunk(world *world, int chunkX, int chunkZ);

world *world_get_worlds();

#endif

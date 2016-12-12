#ifndef world_H
#define world_H

typedef void * world;



void world_do_tick(world *world);
void world_generate_chunk(world *world, int chunkX, int chunkZ);

struct world *world_get_worlds();

#endif

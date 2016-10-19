#ifndef world_H
#define world_H


struct world {

};



void world_dotick(struct world *world);
void world_generate_chunk(struct world *world, int chunkX, int chunkZ);

struct world *world_getworlds();

#endif

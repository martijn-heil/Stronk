#ifndef world_H
#define world_H

typedef struct World World;

struct World {

};



void world_dotick(World *world);
void world_generateChunk(World *world, int chunkX, int chunkZ);

World *world_getworlds();

#endif

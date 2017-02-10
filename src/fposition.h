#ifndef LOCATION_H
#define LOCATION_H

#include "world.h"

struct fposition {
    world *world;
    float x, y, z;
    unsigned char yaw;
    unsigned char pitch;
};

#endif

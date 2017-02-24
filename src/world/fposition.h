#ifndef LOCATION_H
#define LOCATION_H

#include "world.h"

struct fposition {
    world *world;
    double x, y, z;
    float yaw;
    float pitch;
};

#endif

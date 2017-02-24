#ifndef STRONK_LOCATION_H
#define STRONK_LOCATION_H

#include <mcpr/mcpr.h>

#include "world/world.h"

struct location
{
    world *w;
    struct mcpr_position coordinates;
};

#endif

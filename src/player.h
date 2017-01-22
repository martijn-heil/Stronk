#ifndef PLAYER_H
#define PLAYER_H

#include <stdio.h>

#include <ninuuid/ninuuid.h>

struct player {
    FILE* fd;
    struct ninuuid uuid;
    struct location location;
};

#endif

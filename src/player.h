#ifndef PLAYER_H
#define PLAYER_H

#include <stdio.h>

#include <ninuuid/ninuuid.h>

struct player {
    struct ninuuid uuid;
    struct location location;

    struct connection connection;
};

#endif

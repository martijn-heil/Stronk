/*
    MIT License

    Copyright (c) 2016 Martijn Heil

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#ifndef STRONK_WORLD_WORLD_H
#define STRONK_WORLD_WORLD_H

typedef void world;
typedef void block;

#include <stdlib.h>
#include <stdbool.h>

#include <world/positions.h>
#include "../network/player.h"

struct player; // TODO.. what the hell? super strange bug, why is this required??

int world_manager_init(void);
void world_manager_cleanup(void);
void world_do_tick(world *world);
size_t world_manager_get_world_count();
world **world_manager_get_worlds();
struct entitypos world_manager_get_init_spawn_pos(void);
bool world_send_chunk_data1(const struct player *p);
//void testerino(struct testerino *t);


#endif

#include <stdlib.h>

#include "server.h"


/*
    System & architecture requirements:

    C11 (including variable length arrays)
    POSIX (mainly the C library though)
    Pthreads must be available.
    Preferrably GCC, but other compilers should work too.

    2's complement integers.
    char and unsigned char should be exactly 8 bits wide.

    endianness has to be either little endian or big endian, no middle endian.
*/

int main(void)
{
    server_start();
    return EXIT_SUCCESS;
}

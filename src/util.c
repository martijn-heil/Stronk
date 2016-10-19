#include <string.h>

#include "util.h"

enum endianness get_endianness() {

    // Non portable way, works with GCC. We keep it in here for optimization.
    #if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
        #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            return S_LITTLE_ENDIAN;
        #elif
            return S_BIG_ENDIAN;
        #endif
    #endif

    // Portable way to detect endianness below. This unfortainly happens at runtime, hence the non portable
    // optimizations above.
    int n = 1;
    // little endian if true
    return (*(char *)&n == 1) ? S_LITTLE_ENDIAN : S_BIG_ENDIAN;
}

void hton(void *what, size_t n) {
    if(get_endianness() == S_LITTLE_ENDIAN) {
        bswap(what, n);
    }
}

void ntoh(void *what, size_t n) {
    if(get_endianness() == S_LITTLE_ENDIAN) {
        bswap(what, n);
    }
}

void bswap(void *what, size_t n) {
    uint8_t tmp[n];
    memcpy(&tmp, what, n);

    for(size_t i = 0; i < n; i++) {
        ((uint8_t *) what)[i] = tmp[n - 1 - i];
    }
}

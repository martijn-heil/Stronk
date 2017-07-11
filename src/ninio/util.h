#ifndef MCPR_UTIL_H
#define MCPR_UTIL_H

#ifndef DO_PRAGMA
    #ifdef __GNUC__
        #define DO_PRAGMA(x) _Pragma (#x)
    #else
        #define DO_PRAGMA(x)
    #endif
#endif

#ifdef __GNUC__
    #ifndef likely
        #define likely(x)       __builtin_expect(!!(x), 1)
    #endif

    #ifndef unlikely
        #define unlikely(x)     __builtin_expect(!!(x), 0)
    #endif
#else
    #ifndef likely
        #define likely(x) (x)
    #endif

    #ifndef unlikely
        #define unlikely(x) (x)
    #endif
#endif

#ifdef __GNUC__
    #ifndef DO_GCC_PRAGMA
        #define DO_GCC_PRAGMA(x) DO_PRAGMA(x)
    #endif
#else
    #ifndef DO_GCC_PRAGMA
        #define DO_GCC_PRAGMA(x)
    #endif
#endif

#ifdef __GNUC__
    #ifndef IF_GCC
        #define IF_GCC(x) (x)
    #endif
#else
    #ifndef IF_GCC
        #define IF_GCC(x)
    #endif
#endif

#define IGNORE(x) \
    DO_GCC_PRAGMA(GCC diagnostic push) \
    DO_GCC_PRAGMA(GCC diagnostic ignored x)

#define END_IGNORE() \
    DO_GCC_PRAGMA(GCC diagnostic pop)

#endif

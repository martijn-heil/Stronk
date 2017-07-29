/*
    MIT License

    Copyright (c) 2017 Martijn Heil

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

#ifndef NINIO_UTIL_H
#define NINIO_UTIL_H

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

#ifndef __FILENAME__
    #if defined(WIN32) || defined(_WIN32) || defined(_WIN64) && !defined(__CYGWIN__)
        #define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
    #else
        #define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
    #endif
#endif

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L
# if defined __GNUC__ && __GNUC__ >= 2
#  define __func__ __FUNCTION__
# else
#  define __func__ "<unknown>"
# endif
#endif

#endif

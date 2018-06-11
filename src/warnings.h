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

#ifndef STRONK_WARNINGS_H
#define STRONK_WARNINGS_H

#ifndef DO_PRAGMA
    #ifdef __GNUC__
        #define DO_PRAGMA(x) _Pragma (#x)
    #else
        #define DO_PRAGMA(x)
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

#define IGNORE(x) \
    DO_GCC_PRAGMA(GCC diagnostic push) \
    DO_GCC_PRAGMA(GCC diagnostic ignored x)

#define END_IGNORE() \
    DO_GCC_PRAGMA(GCC diagnostic pop)

#endif

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

#ifndef NINUUID_H
#define NINUUID_H

#include <stdbool.h>

struct ninuuid
{
    unsigned char bytes[16];
};

bool ninuuid_generate(struct ninuuid *out, unsigned char version);
bool ninuuid_equals(const struct ninuuid *first, const struct ninuuid *second);
void ninuuid_to_string(const struct ninuuid *self, char *out);
void ninuuid_to_compressed_string(const struct ninuuid *self, char *out);
void ninuuid_from_string(struct ninuuid *out, const char *in);
void ninuuid_from_compressed_string(struct ninuuid *out, const char *in);
unsigned char ninuuid_get_version(struct ninuuid *uuid);
void ninuuid_copy(struct ninuuid *out, struct ninuuid *in);

#endif

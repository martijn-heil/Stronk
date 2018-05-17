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

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <ninuuid/ninuuid.h>

// Returns <0 upon error.
#ifndef HAVE_SECURE_RANDOM
#define HAVE_SECURE_RANDOM
static int secure_random(void *buf, size_t len) {
    int urandomfd = open("/dev/urandom", O_RDONLY);
    if(urandomfd == -1) {
        return -1;
    }

    ssize_t urandomread = read(urandomfd, buf, len);
    if(urandomread == -1) {
        return -1;
    }
    if(((size_t) urandomread) != len) {
        return -1;
    }

    close(urandomfd);
    return 0;
}
#endif

void ninuuid_load(struct ninuuid *out, const char *in)
{
    for(int i = 0; i < 16; i++) out->bytes[i] = in[i];
}

void ninuuid_unload(const struct ninuuid *self, char *out)
{
    for(int i = 0; i < 16; i++) out[i] = self->bytes[i];
}

bool ninuuid_generate(struct ninuuid *out, unsigned char version)
{
    switch(version)
    {
        case 4: // random UUID
        {
            struct ninuuid tmp;
            if(secure_random(tmp.bytes, 16) == -1) return false;
            tmp.bytes[8] = (tmp.bytes[8] & 0x0F) | 0x10;
            ninuuid_copy(out, &tmp);
            return true;
        }

        default: { fprintf(stderr, "ninuuid.c:%i: Version %i UUID's are not supported. ", __LINE__, version); abort(); }
    }
}

bool ninuuid_equals(const struct ninuuid *first, const struct ninuuid *second)
{
    for(int i = 0; i < 16; i++) if(first->bytes[i] != second->bytes[i]) return false;
    return true;
}

void ninuuid_to_string(const struct ninuuid *self, char *out, enum ccase ccase, bool compressed)
{
    const char *fmt;
    if(compressed)
    {
        fmt = "%hhX%hhX%hhX%hhX%hhX%hhX%hhX%hhX%hhX%hhX%hhX%hhX%hhX%hhX%hhX%hhX";
    }
    else
    {
        fmt = "%hhX%hhX%hhX%hhX-%hhX%hhX-%hhX%hhX-%hhX%hhX-%hhX%hhX%hhX%hhX%hhX%hhX";
    }
    sprintf(out, fmt,
        self->bytes[0],
        self->bytes[1],
        self->bytes[2],
        self->bytes[3],
        self->bytes[4],
        self->bytes[5],
        self->bytes[6],
        self->bytes[7],
        self->bytes[8],
        self->bytes[9],
        self->bytes[10],
        self->bytes[11],
        self->bytes[12],
        self->bytes[13],
        self->bytes[14],
        self->bytes[15]
    );

    if(ccase == UPPERCASE)
    {
        for(int i = 0; i < 35; i++) if(out[i] != '-') out[i] = toupper(out[i]);
    }
}

bool ninuuid_from_string(struct ninuuid *out, const char *in)
{
    bool compressed = in[8] != '-';

    const char *fmt;
    if(compressed)
    {
        fmt = "%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX";
    }
    else
    {
        fmt = "%02hhX%02hhX%02hhX%02hhX-%02hhX%02hhX-%02hhX%02hhX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX";
    }

    int result = sscanf(in, fmt,
        out->bytes + 0,
        out->bytes + 1,
        out->bytes + 2,
        out->bytes + 3,
        out->bytes + 4,
        out->bytes + 5,
        out->bytes + 6,
        out->bytes + 7,
        out->bytes + 8,
        out->bytes + 9,
        out->bytes + 10,
        out->bytes + 11,
        out->bytes + 12,
        out->bytes + 13,
        out->bytes + 14,
        out->bytes + 15
    );
    if(result == EOF || result < 16) return false;

    return true;
}

unsigned char ninuuid_get_version(struct ninuuid *uuid)
{
    return uuid->bytes[6] & 0xF0;
}

void ninuuid_copy(struct ninuuid *out, struct ninuuid *in)
{
    for(int i = 0; i < 16; i++) out->bytes[i] = in->bytes[i];
}

bool ninuuid_is_null(const struct ninuuid *in)
{
    for(int i = 0; i < 16; i++) if(in->bytes[i] != 0) return false;
    return true;
}

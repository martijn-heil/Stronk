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

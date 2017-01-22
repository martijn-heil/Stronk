#ifndef NINUUID_H
#define NINUUID_H

struct ninuuid {
    unsigned char bytes[16];
};

bool ninuuid_generate(struct ninuuid *out, unsigned char version);
bool ninuuid_equals(struct ninuuid *first, struct ninuuid *second);
void ninuuid_to_string(struct ninuuid *self, char *out);
void ninuuid_to_compressed_string(struct ninuuid *self, char *out);
void ninuuid_from_string(struct ninuuid *out, char *in);
void ninuuid_from_compressed_string(struct ninuuid *out, char *in);
unsigned char ninuuid_get_version(struct ninuuid *uuid);
void ninuuid_copy(struct ninuuid *out, struct ninuuid *in);

#endif

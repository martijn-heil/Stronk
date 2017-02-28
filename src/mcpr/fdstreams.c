#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>

#include <safe_math.h>
#include <openssl/evp.h>
#include <ninuuid/ninuuid.h>

#include "mcpr/mcpr.h"
#include "mcpr/fdstreams.h"

#define htonll(x) ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) ((1==ntohl(1)) ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))

#define hton16(x) htons(x)
#define hton32(x) htonl(x)
#define hton64(x) htonll(x)

#define ntoh16(x) ntohs(x)
#define ntoh32(x) ntohl(x)
#define ntoh64(x) ntohll(x)


ssize_t mcpr_fd_read_byte(int8_t *out, int in)
{
    if(read(in, out, 1) == -1)
    {
        mcpr_errno = errno;
        return -1;
    }

    return 1;
}

ssize_t mcpr_fd_read_ubyte(uint8_t *out, int in)
{
    if(read(in, out, 1) == -1)
    {
        mcpr_errno = errno;
        return -1;
    }

    return 1;
}

ssize_t mcpr_fd_read_short(int16_t *out, int in)
{
    int16_t tmp;
    if(read(in, &tmp, 2) == -1)
    {
        mcpr_errno = errno;
        return -1;
    }
    *out = ntoh16(tmp);

    return 2;
}

ssize_t mcpr_fd_read_ushort(uint16_t *out, int in)
{
    uint16_t tmp;
    if(read(in, &tmp, 2) == -1)
    {
        mcpr_errno = errno;
        return -1;
    }
    *out = ntoh16(tmp);

    return 2;
}

ssize_t mcpr_fd_read_int(int32_t *out, int in)
{
    int32_t tmp;
    if(read(in, &tmp, 4) == -1)
    {
        mcpr_errno = errno;
        return -1;
    }
    *out = ntoh32(tmp);

    return 4;
}

ssize_t mcpr_fd_read_long(int64_t *out, int in)
{
    int64_t tmp;
    if(read(in, &tmp, 8) == -1)
    {
        mcpr_errno = errno;
        return -1;
    }
    *out = ntoh64(tmp);

    return 2;
}

ssize_t mcpr_fd_read_float(float *out, int in)
{
    if(read(in, out, sizeof(float)) == -1)
    {
        mcpr_errno = errno;
        return -1;
    }

    return sizeof(float);
}

ssize_t mcpr_fd_read_double(double *out, int in)
{
    if(read(in, out, sizeof(double)) == -1)
    {
        mcpr_errno = errno;
        return -1;
    }

    return sizeof(double);
}

ssize_t mcpr_fd_read_string(char **out, int in, size_t maxlen)
{
    int32_t len;
    ssize_t tmp_result = mcpr_fd_read_varint(&len, in);
    if(tmp_result < 0) return -1;
    if(len < 0) { mcpr_errno = MCPR_EDECODE; return -1; }
    if(len > maxlen) { mcpr_errno = MCPR_EDECODE; return -1; }
    if(len > SIZE_MAX) { mcpr_errno = MCPR_EINTERNAL; return -1; }

    *out = malloc(len);
    if(*out == NULL) { return -1; }

    if(read(in, *out, len) == -1) { free(*out); return -1; }

    ssize_t result;
    if(!safe_add(&result, len, tmp_result)) { free(*out); mcpr_errno = MCPR_EINTERNAL; return -1; }
    return result;
}

ssize_t mcpr_fd_read_chat(char **out, int in, size_t maxlen)
{
    return mcpr_fd_read_string(out, in, maxlen);
}

ssize_t mcpr_fd_read_varint(int32_t *out, int in)
{

}

ssize_t mcpr_fd_read_varlong(int64_t *out, int in);
ssize_t mcpr_fd_read_position(struct mcpr_position *out, int in);
ssize_t mcpr_fd_read_angle(int8_t *out, int in);
ssize_t mcpr_fd_read_uuid(struct ninuuid *out, int in);
ssize_t mcpr_fd_read_byte_array(void *out, int in);


ssize_t mcpr_fd_write_byte(int out, int8_t in);
ssize_t mcpr_fd_write_ubyte(int out, uint8_t in);
ssize_t mcpr_fd_write_short(int out, int16_t in);
ssize_t mcpr_fd_write_ushort(int out, uint16_t in);
ssize_t mcpr_fd_write_int(int out, int32_t in);
ssize_t mcpr_fd_write_long(int out, int64_t in);
ssize_t mcpr_fd_write_float(int out, float in);
ssize_t mcpr_fd_write_double(int out, double in);
ssize_t mcpr_fd_write_string(int out, const char *in);
ssize_t mcpr_fd_write_chat(int out, const char *json_in);
ssize_t mcpr_fd_write_varint(int out, int32_t in);
ssize_t mcpr_fd_write_varlong(int out, int64_t in);
ssize_t mcpr_fd_write_position(int out, const struct mcpr_position *in);
ssize_t mcpr_fd_write_angle(int out, int8_t in);
ssize_t mcpr_fd_write_uuid(int out, struct ninuuid *in);
ssize_t mcpr_fd_write_byte_array(int out, void *in, size_t len);

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <arpa/inet.h>
#include <uuid/uuid.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <jansson/jansson.h>
#include <safe_math.h>

#include "mcpr.h"
#include "util.h"


int mcpr_encode_bool(void *out, bool b) {
    uint8_t tmp = (b ? 0x01 : 0x00);
    memcpy(out, &tmp, sizeof(uint8_t));
    return sizeof(uint8_t);
}

int mcpr_encode_byte(void *out, int8_t byte) {
    memcpy(out, &byte, sizeof(byte));
    return sizeof(byte);
}

int mcpr_encode_ubyte(void *out, uint8_t byte) {
    memcpy(out, &byte, sizeof(byte));
    return sizeof(byte);
}

int mcpr_encode_short(void *out, int16_t i) {
    hton(&i, sizeof(i));
    memcpy(out, &i, sizeof(i));
    return sizeof(i);
}

int mcpr_encode_ushort(void *out, uint16_t i) {
    hton(&i, sizeof(i));
    memcpy(out, &i, sizeof(i));
    return sizeof(i);
}

int mcpr_encode_int(void *out, int32_t i) {
    hton(&i, sizeof(i));
    memcpy(out, &i, sizeof(i));
    return sizeof(i);
}

int mcpr_encode_long(void *out, int64_t i) {
    hton(&i, sizeof(i));
    memcpy(out, &i, sizeof(i));
    return sizeof(i);
}

int mcpr_encode_float(void *out, float f) {
    hton(&f, sizeof(f));
    memcpy(out, &f, sizeof(f));
    return sizeof(f);
}

int mcpr_encode_double(void *out, double d) {
    hton(&d, sizeof(d));
    memcpy(out, &d, sizeof(d));
    return sizeof(d);
}

int mcpr_encode_string(void *out, const char *utf8Str) {
    size_t len = strlen(utf8Str);
    char *tmp = malloc(len * sizeof(char) + 1);
    if(tmp == NULL) { return MCPR_ERR_MALLOC_FAILURE; }
    strcpy(tmp, utf8Str);
    size_t bytes_written = mcpr_encode_varint(out, len);

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpointer-arith"
        memcpy(out + bytes_written, tmp, len); // doesn't copy the NUL byte, on purpose.
    #pragma GCC diagnostic pop

    free(tmp);
    bytes_written += len;
    return bytes_written;
}

int mcpr_encode_chat(void *out, const json_t *root) {
    char *chat = json_dumps(root, 0); // this should be free'd
    if(chat == NULL) { return -1; }
    size_t bytes_written = mcpr_encode_string(out, chat);
    free(chat);
    return bytes_written;
}

int mcpr_encode_varint(void *out, int32_t value) {
    size_t i = 0;
    hton(&value, sizeof(value));

    do {
        uint8_t temp = (uint8_t)(value & 0x7F); // 0x7F == 0b01111111

        value = ((uint32_t) value)>>7;
        if (value != 0) {
            temp |= 0x80; // 0x80 == 0b10000000
        }
        //((uint8_t *) out)[i] = temp;
        //CAST(uint8_t*, out)[i] = temp;

        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wpointer-arith"
            memcpy((out + i), &temp, sizeof(temp));
        #pragma GCC diagnostic pop

        i++;
    } while (value != 0);

    return i;
}

int mcpr_encode_varlong(void *out, int64_t value) {
    size_t i = 0;
    hton(&value, sizeof(value));

    do {
        uint8_t temp = (uint8_t)(value & 0x7F); // 0x7F == 0b01111111

        value = ((uint32_t) value)>>7;
        if (value != 0) {
            temp |= 0x80; // 0x80 == 0b10000000
        }
        ((uint8_t *) out)[i] = temp;
        i++;
    } while (value != 0);

    return i;
}

int mcpr_encode_position(void *out, const struct mcpr_position *in) {

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wshift-count-overflow"
        int64_t pos = ((((*in).x & 0x3FFFFFF) << 38) | (((*in).y & 0xFFF) << 26) | ((*in).z & 0x3FFFFFF));
    #pragma GCC diagnostic pop

    return mcpr_encode_long(out, pos);
}

int mcpr_encode_angle(void *out, uint8_t angle) {
    return mcpr_encode_ubyte(out, angle);
}

int mcpr_encode_uuid(void *out, uuid_t in) {
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wsizeof-array-argument"

        memcpy(out, in, 16 * sizeof(in));
        hton(out, 16 * sizeof(in));
        return 16 * sizeof(in);

    #pragma GCC diagnostic pop
}





int mcpr_decode_bool(bool *out, const void *in) {
    uint8_t b;
    memcpy(&b, in, sizeof(b));
    *out = (b == 0x01 ? true : false);
    return sizeof(b);
}

int mcpr_decode_byte(int8_t *out, const void *in) {
    memcpy(out, in, sizeof(int8_t));
    return sizeof(int8_t);
}

int mcpr_decode_ubyte(uint8_t *out, const void *in) {
    memcpy(out, in, sizeof(uint8_t));
    return sizeof(uint8_t);
}

int mcpr_decode_short(int16_t *out, const void *in) {
    memcpy(out, in, sizeof(int16_t));
    ntoh(out, sizeof(int16_t));
    return sizeof(int16_t);
}

int mcpr_decode_ushort(uint16_t *out, const void *in) {
    memcpy(out, in, sizeof(uint16_t));
    ntoh(out, sizeof(uint16_t));
    return sizeof(uint16_t);
}

int mcpr_decode_int(int32_t *out, const void *in) {
    memcpy(out, in, sizeof(int32_t));
    ntoh(out, sizeof(int32_t));
    return sizeof(int32_t);
}

int mcpr_decode_long(int64_t *out, const void *in) {
    memcpy(out, in, sizeof(int64_t));
    ntoh(out, sizeof(int64_t));
    return sizeof(int64_t);
}

int mcpr_decode_float(float *out, const void *in) {
    memcpy(out, in, sizeof(float));
    ntoh(out, sizeof(float));
    return sizeof(float);
}

int mcpr_decode_double(double *out, const void *in) {
    memcpy(out, in, sizeof(double));
    ntoh(out, sizeof(double));
    return sizeof(double);
}

int mcpr_decode_string(char *out, const void *in, int32_t len) {
    // 2147483652 is the max string size allowed by the Minecraft Protocol.

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wtype-limits"
        if(len > MCPR_STR_MAX || ((uint32_t) len) >= SIZE_MAX) { return -1; }
    #pragma GCC diagnostic pop

    memcpy(out, in, len);
    ntoh(out, len * sizeof(char));
    out[len] = '\0';

    return len;
}

int mcpr_decode_chat(json_t **out, const void *in) {
    int32_t len;
    int bytes_read = mcpr_decode_varint(&len, in);
    if(bytes_read < 0) { return bytes_read; }

    // Do lot's of safety checks, arithmetic overflow and such things.
    if(len < 1) { return MCPR_ERR_ARITH; }
    uint32_t ulen = (uint32_t) len; // len is guaranteed to be positive at this point.
    if(ulen == UINT32_MAX) { return MCPR_ERR_ARITH_OVERFLOW; } // Adding the extra 1 for a NUL byte would overflow.

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wtype-limits"
        if((ulen + 1) > SIZE_MAX) { return MCPR_ERR_ARITH_OVERFLOW; } // The result would not fit in size_t, so we can't malloc it.
    #pragma GCC diagnostic pop

    size_t bufsize;
    if(safe_mul(&bufsize, (size_t) (ulen + 1), sizeof(char))) { return MCPR_ERR_ARITH_OVERFLOW; }



    char *buf = malloc(bufsize);
    if(buf == NULL) { return MCPR_ERR_MALLOC_FAILURE; }

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpointer-arith"
        int status = mcpr_decode_string(buf, (in + bytes_read), len);
    #pragma GCC diagnostic pop

    if(status < 0) {
        free(buf);
        return status;
    }

    json_error_t err;
    *out = json_loads(buf, 0, &err);
    free(buf); // important we do this exactly here, we don't need buf anymore, but it has to be free'd in case of error return on the next line.
    if(*out == NULL) { return -1; }

    int result;
    if(safe_add(&result, bytes_read, (int) len)) { return MCPR_ERR_ARITH_OVERFLOW; }
    return result;
}

int mcpr_decode_varint(int32_t *out, const void *in) {
    uint8_t *bytes = (uint8_t *) in;

    unsigned int num_read = 0;
    int32_t result = 0;
    uint8_t read;
    do {
        read = bytes[num_read];
        int value = (read & 0x7F); // 0x7F == 0b01111111
        result |= (value << (7 * num_read));

        num_read++;
        if (num_read > 5) {
            return -1;
        }
    } while ((read & 0x80) != 0); // 0x80 == 0b10000000

    ntoh(&result, sizeof(result));
    *out = result;
    return num_read;
}

int mcpr_decode_varlong(int64_t *out, const void *in) {
    uint8_t *bytes = (uint8_t *) in;

    unsigned int num_read = 0;
    int64_t result = 0;
    uint8_t read;
    do {
        read = bytes[num_read];
        int64_t value = (read & 0x7F); // 0x7F == 0b01111111
        result |= (value << (7 * num_read));

        num_read++;
        if (num_read > 10) {
            return -1;
        }
    } while ((read & 0x80) != 0); // 0x80 == 0b10000000

    *out = result;
    return num_read;
}

int mcpr_decode_position(struct mcpr_position *out, const void *in) {
    int64_t iin;
    memcpy(&iin, in, sizeof(int64_t));
    ntoh(&iin, sizeof(int64_t));

    int64_t x = iin >> 38;
    int64_t y = (iin >> 26) & 0xFFF;
    int64_t z = iin << 38 >> 38;

    out->x = x;
    out->y = y;
    out->z = z;
    return sizeof(int64_t);
}

int mcpr_decode_angle(uint8_t *out, const void *in) {
    return mcpr_decode_ubyte(out, in);
}

int mcpr_decode_uuid(uuid_t out, const void *in) {
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wsizeof-array-argument"

    uuid_t uuid;
    memcpy(uuid, in, 16 * sizeof(uuid_t));
    ntoh(uuid, 16 * sizeof(uuid_t));
    uuid_copy(out, uuid);
    return 16 * sizeof(uuid_t);

    #pragma GCC diagnostic pop
}



static int init_socket(int *returned_sockfd, char *host, int port) {
    int sockfd;
    int portno = port;
    int n;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname = host;


    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return -1;
    }

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL)
    {
        return -2;
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
	  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

    /* connect: create a connection with the server */
    if (connect(sockfd, &serveraddr, sizeof(serveraddr)) < 0) {
        return -3;
    }

    return 0;
}

int mcpr_init_client_sess(struct mcpr_client_sess *sess, const char *host, int port) {
    int tmpsockfd;
    int status = init_socket(&tmpsockfd, host, port);
    if(status < 0) { return -1; }
    sess->sockfd = tmpsockfd;
    sess->state = MCPR_STATE_HANDSHAKE;

    //
    // Send Handshake state 2 packet.
    //
    // Prepare data for being stored in the packet buffer.
    //

    uint8_t pkt_id[5];
    if(pkt_id == NULL) { return -1; }
    int bytes_written_1 = mcpr_encode_varint(&pkt_id, 0x00);
    if(bytes_written_1 < 0) { return -1; }


    uint8_t protocol_version[5];
    int bytes_written_2 = mcpr_encode_varint(&protocol_version, MCPR_PROTOCOL_VERSION);
    if(bytes_written_2 < 0) { return -1; }


    uint8_t *server_adress = malloc(strlen(host) + 5);
    if(server_adress == NULL) { return -1; }
    int bytes_written_3 = mcpr_encode_string(server_adress, host);
    if(bytes_written_3 < 0) { free(server_adress); return -1; }


    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wtype-limits"
        if(port > UINT16_MAX) { free(server_adress); return -1; } // Integer overflow protection.
    #pragma GCC diagnostic pop
    uint8_t server_port[2];
    int bytes_written_4 = mcpr_encode_ushort(&server_port, (uint16_t) port);
    if(bytes_written_4 < 0) { free(server_adress); return -1; }


    uint8_t next_state[5];
    int bytes_written_5 = mcpr_encode_varint(&next_state, MCPR_STATE_LOGIN);
    if(bytes_written_5 < 0) { free(server_adress); return -1; }

    int32_t pkt_lengthi = bytes_written_1 + bytes_written_2 + bytes_written_3 + bytes_written_4 + bytes_written_5;

    uint8_t pkt_length[5];
    int bytes_written_6 = mcpr_encode_varint(&pkt_length, pkt_lengthi);
    if(bytes_written_6 < 0) { free(server_adress); return -1 }


    // Put data in the packet buffer. (This packet should be sent uncompressed.)
    size_t pktbuflen = pkt_lengthi + bytes_written_6;
    uint8_t *pktbuf = malloc(pktbuflen);
    unsigned int pktbufpointer = 0;

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpointer-arith"
        memcpy(pktbuf + pktbufpointer, &pkt_length, bytes_written_6);       pktbufpointer += bytes_written_6;
        memcpy(pktbuf + pktbufpointer, &pkt_id, bytes_written_1);           pktbufpointer += bytes_written_1;
        memcpy(pktbuf + pktbufpointer, &protocol_version, bytes_written_2); pktbufpointer += bytes_written_2;
        memcpy(pktbuf + pktbufpointer, server_adress, bytes_written_3);     pktbufpointer += bytes_written_3;
        memcpy(pktbuf + pktbufpointer, &server_port, bytes_written_4);      pktbufpointer += bytes_written_4;
        memcpy(pktbuf + pktbufpointer, &next_state, bytes_written_5);       pktbufpointer += bytes_written_5;
    #pragma GCC diagnostic pop

    // Send the packet.
    write(sess->sockfd, pktbuf, pktbuflen);
    TODO("Finish this function.")




    free(server_adress);
    return 0;
}

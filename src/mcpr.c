#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <arpa/inet.h>
#include <uuid/uuid.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>

#ifdef MCPR_DO_LOGGING
    #include <stronk.h>
#endif

#include <jansson/jansson.h>
#include <safe_math.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/sha.h>

#include "mcpr.h"
#include "util.h"


static void *(*malloc_func)(size_t size) = malloc;
static void (*free_func)(void *ptr) = free;


void mcpr_set_malloc_func(void *(*new_malloc_func)(size_t size)) {
    malloc_func = new_malloc_func;
}

void mcpr_set_free_func(void (*new_free_func)(void *ptr)) {
    free_func = new_free_func;
}

int mcpr_encrypt(void *out, const void *data, EVP_CIPHER_CTX ctx_encrypt, size_t len) {
    int writtenlen;
    if(unlikely(EVP_EncryptUpdate(&ctx_encrypt, (unsigned char *) out, &writtenlen, (unsigned char *) data, len) == 0)) {
        return -1;
    }
    return writtenlen;
}

int mcpr_decrypt(void *out, const void *data, EVP_CIPHER_CTX ctx_decrypt, size_t len) {
    int writtenlen;
    if(unlikely(EVP_DecryptUpdate(&ctx_decrypt, (unsigned char *) out, &writtenlen, (unsigned char *) data, len) == 0)) {
        return -1;
    }
    return writtenlen;
}


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
    char *tmp = malloc_func(len * sizeof(char) + 1);
    if(unlikely(tmp == NULL)) {
        #ifdef MCPR_DO_LOGGING
            nlog_error("Could not allocate memory (%s ?)", strerror(errno));
        #endif
        return MCPR_ERR_MALLOC_FAILURE;
    }
    strcpy(tmp, utf8Str);
    size_t bytes_written = mcpr_encode_varint(out, len);

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpointer-arith"
        memcpy(out + bytes_written, tmp, len); // doesn't copy the NUL byte, on purpose.
    #pragma GCC diagnostic pop

    free_func(tmp);
    bytes_written += len;
    return bytes_written;
}

int mcpr_encode_chat(void *out, const json_t *root) {
    char *chat = json_dumps(root, 0); // this should be free'd
    if(unlikely(chat == NULL)) {
        #ifdef MCPR_DO_LOGGING
            nlog_error("Could not dump JSON to string (%s ?)", strerror(errno));
        #endif
        return -1;
    }
    size_t bytes_written = mcpr_encode_string(out, chat);
    free_func(chat);
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
    if(len <= 0) {
        #ifdef MCPR_DO_LOGGING
            nlog_error("len for mcpr_decode_string() is less than or equal to 0.");
            return -1;
        #endif
    }

    memcpy(out, in, len);
    out[len] = '\0';

    return len;
}

int mcpr_decode_chat(json_t **out, const void *in) {
    int32_t len;
    int bytes_read = mcpr_decode_varint(&len, in, 5);
    if(bytes_read < 0) {
        #ifdef MCPR_DO_LOGGING
            nlog_error("Error whilst decoding varint.");
        #endif
        return bytes_read;
    }

    // Do lot's of safety checks, arithmetic overflow and such things.
    if(len < 1) {
        #ifdef MCPR_DO_LOGGING
            nlog_error("Length of string is less than 1.");
        #endif
        return MCPR_ERR_ARITH;
    }
    uint32_t ulen = (uint32_t) len; // len is guaranteed to be positive at this point.
    if(ulen == UINT32_MAX) {
        #ifdef MCPR_DO_LOGGING
            nlog_error("uint32 arithmetic overflow error.");
        #endif
        return MCPR_ERR_ARITH_OVERFLOW;
    } // Adding the extra 1 for a NUL byte would overflow.

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wtype-limits"
        if(unlikely((ulen + 1) > SIZE_MAX)) {
            #ifdef MCPR_DO_LOGGING
                nlog_error("Length of string does not fit iside size_t, arithmetic overflow error.");
            #endif

            return MCPR_ERR_ARITH_OVERFLOW;
        } // The result would not fit in size_t, so we can't malloc it.
    #pragma GCC diagnostic pop

    size_t bufsize;
    if(unlikely(safe_mul(&bufsize, (size_t) (ulen + 1), sizeof(char)))) { return MCPR_ERR_ARITH_OVERFLOW; }



    char *buf = malloc_func(bufsize);
    if(unlikely(buf == NULL)) {
        #ifdef MCPR_DO_LOGGING
            nlog_error("Could not allocate memory for buffer. (%s ?)", strerror(errno));
        #endif
        return MCPR_ERR_MALLOC_FAILURE;
    }

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpointer-arith"
        int status = mcpr_decode_string(buf, (in + bytes_read), len);
    #pragma GCC diagnostic pop

    if(unlikely(status < 0)) {
        free_func(buf);
        return status;
    }

    json_error_t err;
    *out = json_loads(buf, 0, &err);
    free_func(buf); // important we do this exactly here, we don't need buf anymore, but it has to be free'd in case of error return on the next line.
    if(unlikely(*out == NULL)) {
        #ifdef MCPR_DO_LOGGING
            nlog_error("Could not load JSON string. (%s ?)", strerror(errno));
        #endif

        return -1;
    }

    int result;
    if(unlikely(safe_add(&result, bytes_read, (int) len))) {
        #ifdef MCPR_DO_LOGGING
            nlog_error("Safe add (result = bytes_read + len) failed. Arithmetic overflow error.");
        #endif
        return MCPR_ERR_ARITH_OVERFLOW;
    }
    return result;
}

int mcpr_decode_varint(int32_t *out, const void *in, size_t max_len) { // TODO add max read
    unsigned int num_read = 0;
    int32_t result = 0;
    uint8_t read;
    do {
        DO_GCC_PRAGMA(GCC diagnostic push)
        DO_GCC_PRAGMA(GCC diagnostic ignored "-Wpointer-arith")
            memcpy(&read, in + num_read, 1);
        DO_GCC_PRAGMA(GCC diagnostic pop)

        int value = (read & 0x7F); // 0x7F == 0b01111111
        result |= (value << (7 * num_read));

        num_read++;
        if (unlikely(num_read > 5)) {
            #ifdef MCPR_DO_LOGGING
                nlog_error("VarInt is longer than 5 bytes!");
            #endif
            return -1;
        } else if (unlikely((num_read - 1) >= max_len)) {
            #ifdef MCPR_DO_LOGGING
                nlog_error("Max length (%zu) exceeded whilst decoding VarInt", max_len);
            #endif
            return -1;
        }
    } while ((read & 0x80) != 0); // 0x80 == 0b10000000

    ntoh(&result, sizeof(result));
    *out = result;
    return num_read;
}

int mcpr_decode_varlong(int64_t *out, const void *in, size_t max_len) {
    unsigned int num_read = 0;
    int64_t result = 0;
    uint8_t read;
    do {
        DO_GCC_PRAGMA(GCC diagnostic push)
        DO_GCC_PRAGMA(GCC diagnostic ignored "-Wpointer-arith")
            memcpy(&read, in + num_read, 1);
        DO_GCC_PRAGMA(GCC diagnostic pop)

        int64_t value = (read & 0x7F); // 0x7F == 0b01111111
        result |= (value << (7 * num_read));

        num_read++;
        if (unlikely(num_read > 10)) {
            #ifdef MCPR_DO_LOGGING
                nlog_error("VarLong is longer than 10 bytes!");
            #endif
            return -1;
        } else if (unlikely((num_read - 1) >= max_len)) {
            #ifdef MCPR_DO_LOGGING
                nlog_error("Max length (%zu) exceeded whilst decoding VarLong", max_len);
            #endif
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
    DO_GCC_PRAGMA(GCC diagnostic push)
    DO_GCC_PRAGMA(GCC diagnostic ignored "-Wsizeof-array-argument")

        // This is kinda hacky.. uuid_t is USUALLY a typedef'd unsigned char raw[16]
        // If there is an implementation which does not define uuid_t as unsigned char raw[16] we have a bit of an issue here.
        uuid_t uuid;
        memcpy(uuid, in, 16 * sizeof(uuid_t));
        ntoh(uuid, 16 * sizeof(uuid_t));
        uuid_copy(out, uuid);
        return 16 * sizeof(uuid_t);

    DO_GCC_PRAGMA(GCC diagnostic pop)
}



static int init_socket(int *returned_sockfd, const char *host, int port) {
    int sockfd;
    int portno = port;
    struct sockaddr_in serveraddr;
    struct hostent *server;


    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (unlikely(sockfd < 0)) {
        #ifdef MCPR_DO_LOGGING
            nlog_error("Could not create socket. (%s ?)", strerror(errno));
        #endif
        return -1;
    }

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(host);
    if (unlikely(server == NULL))
    {
        #ifdef MCPR_DO_LOGGING
            nlog_error("Could not get by name. (host: %s).", host);
        #endif
        return -2;
    }

    /* build the server's Internet address */
    //bzero((char *) &serveraddr, sizeof(serveraddr));
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    //bcopy((char *) server->h_addr, (char *) &serveraddr.sin_addr.s_addr, server->h_length);
    memcpy(server->h_addr_list[0], &(serveraddr.sin_addr.s_addr), server->h_length);
    serveraddr.sin_port = htons(portno);

    /* connect: create a connection with the server */
    if (unlikely(connect(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)) {
        #ifdef MCPR_DO_LOGGING
            nlog_error("Could not connect to server. (%s ?)", strerror(errno));
        #endif
        return -3;
    }

    *returned_sockfd = sockfd;
    return 0;
}

static int minecraft_stringify_sha1(char *stringified_hash, const unsigned char *hash1) {
    unsigned char uhash[SHA_DIGEST_LENGTH];
    for(int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        uhash[i] = hash1[i];
    }

    char *hash = (char *) uhash;
    char *stringified_hashp = stringified_hash + 1;

    bool is_negative = *hash < 0;
    if(is_negative) {
        bool carry = true;
        int i;
        unsigned char new_byte;
        unsigned char value;

        for(i = SHA_DIGEST_LENGTH - 1; i >= 0; --i) {
            value = uhash[i];
            new_byte = ~value & 0xFF;
            if(carry) {
                carry = new_byte == 0xFF;
                uhash[i] = new_byte + 1;
            } else {
                uhash[i] = new_byte;
            }
        }
    }

    // Write it as a hex string.
    for(int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        sprintf(stringified_hashp, "%02x", uhash[i]);
        stringified_hashp += 2;
    }
    *stringified_hashp = '\0';

    // Trim leading zeros
    stringified_hashp = stringified_hash + 1;
    for(int i = 0; i < SHA_DIGEST_LENGTH * 2; i++) {
        if(*stringified_hashp != '0') { break; }
        stringified_hashp++;
    }


    if(is_negative) { // Hash is negative.
        *stringified_hash = '-';
    }


    return 0;
}

int mcpr_init_client_sess(struct mcpr_client_sess *sess, const char *host, int port, int timeout, const char *username, bool is_loopback) {
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpointer-arith"

    int tmpsockfd;
    int status = init_socket(&tmpsockfd, host, port);
    if(unlikely(status < 0)) {
        #ifdef MCPR_DO_LOGGING
            nlog_error("Could not initialize socket. (%s ?)", strerror(errno));
        #endif
        return -1;
    }

    // Set socket timeout.
    struct timeval timeoutval;
    timeoutval.tv_sec = timeout;
    timeoutval.tv_usec = 0;
    int setsockopt_status1 = setsockopt(tmpsockfd, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeoutval, sizeof(timeoutval));
    if(unlikely(setsockopt_status1 == -1)) {
        #ifdef MCPR_DO_LOGGING
            nlog_error("Could not set socket receive timeout. (%s ?)", strerror(errno));
        #endif
        return -1;
    }
    int setsockopt_status2 = setsockopt(tmpsockfd, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeoutval, sizeof(timeoutval));
    if(unlikely(setsockopt_status2 == -1)) {
        #ifdef MCPR_DO_LOGGING
            nlog_error("Could not set socket receive timeout. (%s ?)", strerror(errno));
        #endif
        return -1;
    }


    sess->sockfd = tmpsockfd;
    sess->use_compression = false;
    sess->use_encryption = !is_loopback;
    sess->state = MCPR_STATE_HANDSHAKE;

    //
    // Send Handshake state 2 packet.
    //
    {
        uint8_t *buf = malloc_func(strlen(host) + 27);
        if(unlikely(buf == NULL)) return -1;
        size_t offset = 5; // Skip the first 5 bytes to leave room for the length varint.

        // Writes 5 bytes at most
        // write Packet ID
        int bytes_written_1 = mcpr_encode_varint(buf + offset, 0x00);
        if(unlikely(bytes_written_1 < 0)) { free_func(buf); return -1; }
        offset += bytes_written_1;

        // Writes 5 bytes at most
        // write Protocol version
        int bytes_written_2 = mcpr_encode_varint(buf + offset, MCPR_PROTOCOL_VERSION);
        if(unlikely(bytes_written_2 < 0)) { free_func(buf); return -1; }
        offset += bytes_written_2;

        // Writes strlen(host) + 5 bytes at most
        // write server host used to connect
        int bytes_written_3 = mcpr_encode_string(buf + offset, host);
        if(unlikely(bytes_written_3 < 0)) { free_func(buf); return -1; }
        offset += bytes_written_3;

        // writes 2 bytes
        // write server port used to connect
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wtype-limits"
            if(port > UINT16_MAX) { free_func(buf); return -1; } // Integer overflow protection.
        #pragma GCC diagnostic pop
        int bytes_written_4 = mcpr_encode_ushort(buf + offset, (uint16_t) port);
        if(unlikely(bytes_written_4 < 0)) { free_func(buf); return -1; }
        offset += bytes_written_4;

        // writes 5 bytes at most
        // write next state
        int bytes_written_5 = mcpr_encode_varint(buf + offset, MCPR_STATE_LOGIN);
        if(unlikely(bytes_written_5 < 0)) { free_func(buf); return -1; }
        offset += bytes_written_5;


        // We've written all the data and packet id, now prefix it all with the length.
        uint8_t tmplenbuf[5];
        size_t data_len = offset - 5;
        int bytes_written_6 = mcpr_encode_varint(&tmplenbuf, data_len); // -5 because of the initial offset.
        if(unlikely(bytes_written_6 < 0)) { free_func(buf); return -1; }
        uint8_t *new_start_of_pkt = buf + (5 - bytes_written_6);
        memcpy(new_start_of_pkt, &tmplenbuf, bytes_written_6);

        // Send the packet.
        ssize_t status = write(sess->sockfd, new_start_of_pkt, data_len + bytes_written_6);
        if(unlikely(status == -1)) {
            #ifdef MCPR_DO_LOGGING
                nlog_error("Error writing to socket. (%s ?)", strerror(errno));
            #endif
            return -1;
        }
        free_func(buf);
    }


    sess->state = MCPR_STATE_LOGIN;


    //
    // Send login start packet.
    //
    {
        uint8_t *buf = malloc(strlen(username) + 15);
        if(unlikely(buf == NULL)) return -1;
        size_t offset = 5; // Skip the first 5 bytes to leave room for the length varint.

        // Writes 5 bytes at most
        // write Packet ID
        int bytes_written_1 = mcpr_encode_varint(buf + offset, 0x00);
        if(unlikely(bytes_written_1 < 0)) { free_func(buf); return -1; }
        offset += bytes_written_1;

        // Writes strlen(username) + 5 bytes at most.
        int bytes_written_2 = mcpr_encode_string(buf + offset, username); // TODO: We need username information.
        if(unlikely(bytes_written_2 < 0)) { free_func(buf); return -1; }
        offset += bytes_written_2;

        // Prefix data with length of packet.
        size_t data_len = offset - 5;
        uint8_t tmplenbuf[5];
        int bytes_written_3 = mcpr_encode_varint(&tmplenbuf, data_len);
        if(unlikely(bytes_written_3 < 0)) { free_func(buf); return -1; }
        uint8_t *pkt_start = buf + (5 - bytes_written_3);
        memcpy(pkt_start, &tmplenbuf, bytes_written_3);

        // Send the packet.
        ssize_t write_status = write(sess->sockfd, pkt_start, data_len + bytes_written_3);
        if(unlikely(write_status == -1)) {
            #ifdef MCPR_DO_LOGGING
                nlog_error("Error writing to socket. (%s ?)", strerror(errno));
            #endif
            return -1;
            free_func(buf);
        }
        free_func(buf);
    }

    // Encryption is not used for loopback connections.
    if(sess->use_encryption) {
        // We will get this data from the Encryption Request packet, we need it after that.
        int shared_secret_len = 16;
        unsigned char shared_secret[shared_secret_len];

        unsigned char* encrypted_shared_secret_buf;
        int encrypted_shared_secret_len;

        int32_t verify_token_len;
        int8_t *verify_token;
        unsigned char *encrypted_verify_token;
        int encrypted_verify_token_len;

        char *server_id;
        int8_t *server_pubkey;
        size_t server_pubkey_len;


        //
        // Read encryption request packet
        //
        {
            uint8_t pkt_len_raw[5];

            ssize_t bytes_read = read(sess->sockfd, &pkt_len_raw, 1);
            if(unlikely(bytes_read == -1)) {
                #ifdef MCPR_DO_LOGGING
                    nlog_error("Error reading from socket. (%s ?)", strerror(errno));
                #endif
                return -1;
            }

            int32_t pktlen;
            int bytes_read_2 = mcpr_decode_varint(&pktlen, &pkt_len_raw, 5);
            if(bytes_read_2 < 0) { return -1; }
            if(pktlen < 0) { return -1; }
            DO_GCC_PRAGMA(GCC diagnostic push)
            DO_GCC_PRAGMA(GCC diagnostic ignored "-Wtype-limits")
                if(unlikely((uint32_t) pktlen > SIZE_MAX)) { return -1; }
            DO_GCC_PRAGMA(GCC diagnostic pop)
            if(unlikely(pktlen < 1)) { return -1; }

            uint8_t *buf = malloc_func((size_t) pktlen);
            if(unlikely(buf == NULL)) {
                #ifdef MCPR_DO_LOGGING
                    nlog_error("Could not allocate memory. (%s ?)", strerror(errno));
                #endif
                return -1;
            }
            ssize_t bytes_read_3 = read(sess->sockfd, buf, (size_t) pktlen);
            if(bytes_read_3 == -1) { free_func(buf); return -1; }
            if(bytes_read_3 != pktlen) { free_func(buf); return -1; }
            size_t len_left = pktlen;


            uint8_t *bufpointer = buf;


            // Read packet ID
            int32_t pkt_id;
            int bytes_read_4 = mcpr_decode_varint(&pkt_id, bufpointer, len_left);
            if(unlikely(bytes_read_4 < 0)) { free_func(buf); return -1; }
            if(unlikely(pkt_id != 0x01)) { free_func(buf); return -1; } // Ensure that this packet is actually an encryption request packet..
            bufpointer += bytes_read_4;
            len_left -= bytes_read_4;

            // Read server ID string length
            if(unlikely(len_left < 1)) { free_func(buf); return -1; }
            int32_t str_len;
            int bytes_read_5 = mcpr_decode_varint(&str_len, bufpointer, len_left);
            if(unlikely(bytes_read_5 < 0)) { free_func(buf); return -1; }
            if(unlikely(str_len < 0)) { free_func(buf); return -1; }
            DO_GCC_PRAGMA(GCC diagnostic push)
            DO_GCC_PRAGMA(GCC diagnostic ignored "-Wtype-limits")
                if(unlikely(((uint32_t) str_len) > SIZE_MAX)) { free_func(buf); return -1; }
            DO_GCC_PRAGMA(GCC diagnostic pop)
            bufpointer += bytes_read_5;
            len_left -= bytes_read_5;

            // Read server ID
            if(len_left < (uint32_t) str_len) { free_func(buf); return -1; }
            server_id = malloc_func((str_len + 1) * sizeof(char));
            if(unlikely(server_id == NULL)) { free_func(buf); return -1; }
            int bytes_read_6 = mcpr_decode_string(server_id, bufpointer, len_left);
            if(unlikely(bytes_read_6 < 0)) { free_func(server_id); free_func(buf); return -1; }
            bufpointer += bytes_read_6;
            len_left -= bytes_read_6;

            // Read public key length
            if(len_left < 1) { free_func(server_id); free_func(buf); return -1; }
            int32_t pubkeylen;
            int bytes_read_7 = mcpr_decode_varint(&pubkeylen, bufpointer, len_left);
            if(unlikely(bytes_read_7 < 0)) { free_func(server_id); free_func(buf); return -1; }
            if(unlikely(pubkeylen < 0)) { free_func(server_id); free_func(buf); return -1; }
            DO_GCC_PRAGMA(GCC diagnostic push)
            DO_GCC_PRAGMA(GCC diagnostic ignored "-Wtype-limits")
                if(unlikely(((uint32_t) pubkeylen) > SIZE_MAX)) { free_func(server_id); free_func(buf); return -1; }
            DO_GCC_PRAGMA(GCC diagnostic pop)
            bufpointer += bytes_read_7;
            len_left -= bytes_read_6;

            // Read public key
            if(len_left < ((uint32_t) pubkeylen)) { free_func(server_id); free_func(buf); return -1; }
            int8_t *pubkey = malloc_func((size_t) pubkeylen);
            if(unlikely(pubkey == NULL)) {
                #ifdef MCPR_DO_LOGGING
                    nlog_error("Could not allocate memory. (%s ?)", strerror(errno));
                #endif
                return -1;
            }
            int8_t *pubkeyp = pubkey;
            for(int32_t i = 0; i < pubkeylen; i++) {
                int bytes_read_8 = mcpr_decode_byte(pubkeyp, bufpointer);
                if(unlikely(bytes_read_8 < 0)) { free_func(server_id); free_func(buf); free_func(pubkey); return -1; }
                pubkey++;
                bufpointer++;
                len_left--;
            }

            // Read verify token length
            if(unlikely(len_left < 1)) { free_func(server_id); free_func(buf); free_func(pubkey); return -1; }
            int bytes_read_9 = mcpr_decode_varint(&verify_token_len, bufpointer, len_left);
            if(unlikely(bytes_read_9 < 0)) { free_func(server_id); free_func(buf); free_func(pubkey); return -1; }
            if(unlikely(verify_token_len < 0)) { free_func(server_id); free_func(buf); free_func(pubkey); return -1; }
            DO_GCC_PRAGMA(GCC diagnostic push)
            DO_GCC_PRAGMA(GCC diagnostic ignored "-Wtype-limits")
                if(unlikely(((uint32_t) verify_token_len) > SIZE_MAX)) { free_func(server_id); free_func(buf); free_func(pubkey); return -1; }
            DO_GCC_PRAGMA(GCC diagnostic pop)
            bufpointer += bytes_read_9;
            len_left -= bytes_read_9;

            if(len_left < ((uint32_t) verify_token_len)) { free_func(server_id); free_func(buf); free_func(pubkey); return -1; }
            verify_token = malloc_func((size_t) verify_token_len);
            int8_t *verify_tokenp = verify_token;
            for(int32_t i = 0; i < verify_token_len; i++) {
                int bytes_read_10 = mcpr_decode_byte(verify_tokenp, bufpointer);
                if(unlikely(bytes_read_10 < 0)) { free_func(server_id); free_func(buf); free_func(pubkey); free_func(verify_token); return -1; }
                verify_tokenp++;
                bufpointer++;
            }


            RSA *rsa = d2i_RSA_PUBKEY(NULL, (const unsigned char **) &pubkey, pubkeylen);


            // Generate shared secret.
            int urandom = open("/dev/urandom", O_RDONLY);
            if(unlikely(urandom == -1)) {
                #ifdef MCPR_DO_LOGGING
                    nlog_error("Error opening /dev/urandom (%s ?)", strerror(errno));
                    RSA_free(rsa);
                    free_func(buf);
                    free_func(pubkey);
                    free_func(verify_token);
                    free_func(server_id);
                #endif
                return -1;
            }

            ssize_t urandom_read_status = read(urandom, shared_secret, 16);
            if(unlikely(urandom_read_status == -1)) {
                #ifdef MCPR_DO_LOGGING
                    nlog_error("Error reading from /dev/urandom (%s ?)", strerror(errno));
                    RSA_free(rsa);
                    free_func(buf);
                    free_func(pubkey);
                    free_func(verify_token);
                    free_func(server_id);
                #endif
                return -1;
            }

            if(unlikely(urandom_read_status < 16)) { // This shouldn't happen, but let's keep this in here for now.
                #ifdef MCPR_DO_LOGGING
                    nlog_error("Reading from /dev/urandom did read less than the required 16 bytes!");
                    RSA_free(rsa);
                    free_func(buf);
                    free_func(pubkey);
                    free_func(verify_token);
                    free_func(server_id);
                #endif
                return -1;
            }

            int urandom_close_status = close(urandom);
            if(unlikely(urandom_close_status == -1)) {
                #ifdef MCPR_DO_LOGGING
                    nlog_error("Could not close /dev/urandom (%s ?)", strerror(errno));
                    RSA_free(rsa);
                    free_func(buf);
                    free_func(pubkey);
                    free_func(verify_token);
                    free_func(server_id);
                #endif
                return -1;
            }


            // Encrypt shared secret.
            encrypted_shared_secret_len = RSA_size(rsa);
            encrypted_shared_secret_buf = malloc_func(encrypted_shared_secret_len);
            if(unlikely(encrypted_shared_secret_buf == NULL)) {
                RSA_free(rsa);
                free_func(buf);
                free_func(pubkey);
                free_func(verify_token);
                free_func(server_id);
                return -1;
            }
            RSA_public_encrypt(shared_secret_len, shared_secret, encrypted_shared_secret_buf, rsa, RSA_PKCS1_PADDING);

            encrypted_verify_token_len = RSA_size(rsa);
            encrypted_verify_token = malloc_func(encrypted_verify_token_len);
            if(unlikely(encrypted_verify_token == NULL)) {
                RSA_free(rsa);
                free_func(buf);
                free_func(pubkey);
                free_func(verify_token);
                free_func(server_id);
                return -1;
            }
            RSA_public_encrypt(verify_token_len, (unsigned char *) verify_token, encrypted_verify_token, rsa, RSA_PKCS1_PADDING);
            RSA_free(rsa);
            free_func(buf);
            free_func(verify_token);
            server_pubkey = pubkey;
            server_pubkey_len = pubkeylen;
        }

        /*
         * Do client authentication with the Mojang servers.
         * TODO make this optional for offline mode?
         */

        unsigned char *client_auth_hash = malloc_func(SHA_DIGEST_LENGTH);
        if(unlikely(client_auth_hash == NULL)) {
            #ifdef MCPR_DO_LOGGING
                nlog_error("Could not allocate memory. (%s ?)", strerror(errno));
            #endif

            free_func(encrypted_shared_secret_buf);
            free_func(encrypted_verify_token);
            free_func(server_pubkey);
            free_func(server_id);
            return -1;
        }

        SHA_CTX sha_ctx;
        if(unlikely(SHA1_Init(&sha_ctx) == 0)) {
            #ifdef MCPR_DO_LOGGING
                nlog_error("Could not initialize Sha1 hash.");
            #endif
            return -1;
        }

        if(server_id != '\0') { // Make sure the string isn't empty.
            if(unlikely(SHA1_Update(&sha_ctx, server_id, strlen(server_id)) == 0)) {
                #ifdef MCPR_DO_LOGGING
                    nlog_error("Could not update Sha1 hash.");
                #endif
                return -1;
            }
        }
        if(unlikely(SHA1_Update(&sha_ctx, shared_secret, shared_secret_len) == 0)) {
            #ifdef MCPR_DO_LOGGING
                nlog_error("Could not update Sha1 hash.");
            #endif
            return -1;
        }

        if(unlikely(SHA1_Update(&sha_ctx, server_pubkey, server_pubkey_len) == 0)) {
            #ifdef MCPR_DO_LOGGING
                nlog_error("Could not update Sha1 hash.");
            #endif
            return -1;
        }

        if(unlikely(SHA1_Final(client_auth_hash, &sha_ctx) == 0)) {
            #ifdef MCPR_DO_LOGGING
                nlog_error("Could not finalize Sha1 hash.");
            #endif
            return -1;
        }


        char *stringified_client_auth_hash = malloc_func(SHA_DIGEST_LENGTH * 2 + 2);
        if(unlikely(stringified_client_auth_hash == NULL)) {
            #ifdef MCPR_DO_LOGGING
                nlog_error("Could not allocate memory. (%s ?)", strerror(errno));
            #endif
            free_func(encrypted_shared_secret_buf);
            free_func(encrypted_verify_token);
            free_func(server_pubkey);
            free_func(server_id);
            free_func(client_auth_hash);
            return -1;
        }
        int mc_stringify_status = minecraft_stringify_sha1(stringified_client_auth_hash, client_auth_hash);
        if(unlikely(mc_stringify_status < 0)) {
            #ifdef MCPR_DO_LOGGING
                nlog_error("Error stringifying SHA1 hash for client authentication.");
            #endif
            free_func(encrypted_shared_secret_buf);
            free_func(encrypted_verify_token);
            free_func(server_pubkey);
            free_func(server_id);
            free_func(client_auth_hash);
            free_func(stringified_client_auth_hash);
            return -1;
        }


        // TODO send post request with data to Mojang servers.


        free_func(client_auth_hash);
        free_func(stringified_client_auth_hash);



        // Send Encryption response packet.
        {
            uint8_t *buf = malloc_func(15 + shared_secret_len + verify_token_len);
            uint8_t *bufpointer = buf + 5; // Skip the first 5 bytes to leave space for the packet length varint.

            // Write packet ID.
            int bytes_written_1 = mcpr_encode_varint(bufpointer, 0x01);
            if(unlikely(bytes_written_1 < 0)) { free_func(buf); return -1; }
            bufpointer += bytes_written_1;

            // Write shared secret length.
            int bytes_written_2 = mcpr_encode_varint(bufpointer, (int32_t) shared_secret_len);
            if(unlikely(bytes_written_2 < 0)) { free_func(buf); return -1; }
            bufpointer += bytes_written_1;

            // Write shared secret. (Encrypted using the server's public key)
            unsigned char *encrypted_shared_secret_pointer = encrypted_shared_secret_buf;
            for(int i = 0; i < encrypted_shared_secret_len; i++) {
                int bytes_written_3 = mcpr_encode_byte(bufpointer, (int8_t)(*encrypted_shared_secret_pointer));
                if(unlikely(bytes_written_3 < 0)) { free_func(buf); return -1; }
                bufpointer++;
                encrypted_shared_secret_pointer++;
            }

            // Write verify token length.
            int bytes_written_4 = mcpr_encode_varint(bufpointer, verify_token_len);
            if(unlikely(bytes_written_4 < 0)) { free_func(buf); return -1; }
            bufpointer += bytes_written_4;

            // Write verify token. (Encrypted using the server's public key)
            uint8_t *encrypted_verify_token_pointer = encrypted_verify_token;
            for(int i = 0; i < encrypted_verify_token_len; i++) {
                int bytes_written_5 = mcpr_encode_byte(bufpointer, *((int8_t *) encrypted_verify_token_pointer));
                if(unlikely(bytes_written_5 < 0)) { free_func(buf); return -1; }
                bufpointer++;
                encrypted_verify_token_pointer++;
            }

            // Prefix the packet by the length of it all.
            int32_t pktlen = bufpointer - (buf + 5);
            int8_t tmppktlen[5];
            int bytes_written_6 = mcpr_encode_varint(&tmppktlen, pktlen);
            if(unlikely(bytes_written_6 < 0)) { free_func(buf); return -1; }
            memcpy(buf + (5 - bytes_written_6), &tmppktlen, bytes_written_6);

            ssize_t status = write(sess->sockfd, buf + (5 - bytes_written_6), pktlen);
            free_func(buf);
            if(status == -1) {
                #ifdef MCPR_DO_LOGGING
                    nlog_error("Error writing to socket. (%s ?)", strerror(errno));
                #endif
                return -1;
            }
        }



        //
        //  Enable & initialize encryption
        //  See https://gist.github.com/Dav1dde/3900517
        //
        //unsigned char *key = shared_secret;
        //unsigned char *iv = shared_secret; // TODO why this weirdness in the example?

        // Initialize this stuff..
        EVP_CIPHER_CTX_init(&(sess->ctx_encrypt));
        if(unlikely(EVP_EncryptInit_ex(&(sess->ctx_encrypt), EVP_aes_128_cfb8(), NULL, shared_secret, shared_secret) == 0)) { // Should engine be NULL?? I don't know
            #ifdef MCPR_DO_LOGGING
                nlog_error("Error upon EVP_EncryptInit_ex().");
            #endif
            return -1;
        }

        EVP_CIPHER_CTX_init(&(sess->ctx_decrypt));
        if(unlikely(EVP_DecryptInit_ex(&(sess->ctx_decrypt), EVP_aes_128_cfb8(), NULL, shared_secret, shared_secret) == 0)) { // Should engine be NULL?? I don't know
            #ifdef MCPR_DO_LOGGING
                nlog_error("Error upon EVP_DecryptInit_ex().");
            #endif
            return -1;
        }

        sess->encryption_block_size = EVP_CIPHER_block_size(EVP_aes_128_cfb8());

        free_func(encrypted_shared_secret_buf);
        free_func(encrypted_verify_token);
        free_func(server_pubkey);
        free_func(server_id);
    } // end if(!is_localhost)

    // Read Login Success packet OR set compression packet.
    {
        bool set_compression_received = false;

        for(int i = 0; i < 2; i++) {
            uint8_t pkt_len_raw[MCPR_VARINT_SIZE_MAX];
            ssize_t read_status_1 = read(sess->sockfd, &pkt_len_raw, 5);
            if(unlikely(read_status_1 == -1)) {
                #ifdef MCPR_DO_LOGGING
                    nlog_error("Error reading from socket. (%s ?)", strerror(errno));
                #endif
                return -1;
            }

            int32_t pktleni;
            int bytes_read_1 = mcpr_decode_varint(&pktleni, pkt_len_raw, MCPR_VARINT_SIZE_MAX);
            if(unlikely(bytes_read_1 < 0)) {
                return -1;
            }

            if(unlikely(pktleni < 0)) {
                #ifdef MCPR_DO_LOGGING
                    nlog_error("Read packet length is negative!");
                #endif
                return -1;
            }

            DO_GCC_PRAGMA(GCC diagnostic push)
            DO_GCC_PRAGMA(GCC diagnostic ignored "-Wtype-limits")
                if(unlikely((uint32_t) pktleni > SIZE_MAX)) {
                    #ifdef MCPR_DO_LOGGING
                        nlog_error("Read packet length is greater than SIZE_MAX! Arithmetic overflow error.");
                    #endif
                    return -1;
                }
            DO_GCC_PRAGMA(GCC diagnostic pop)

            // pktleni is guaranteed to be non-negative and <= SIZE_MAX at this point.
            size_t pktlen = pktleni;

            uint8_t *pktbuf = malloc_func(pktlen);
            if(unlikely(pktbuf == NULL)) {
                #ifdef MCPR_DO_LOGGING
                    nlog_error("Could not allocate memory. (%s ?)", strerror(errno));
                #endif
                return -1;
            }

            ssize_t read_status_2 = read(sess->sockfd, pktbuf, pktlen);
            if(unlikely(read_status_2 == -1)) {
                #ifdef MCPR_DO_LOGGING
                    nlog_error("Error reading from socket. (%s ?)", strerror(errno));
                #endif
                free_func(pktbuf);
                return -1;
            }

            if(unlikely((unsigned int) read_status_2 < pktlen)) {
                #ifdef MCPR_DO_LOGGING
                    nlog_error("Was not able to read expected packet length! Only read %zd bytes.", read_status_2);
                #endif
                free_func(pktbuf);
                return -1;
            }

            size_t len_left = pktlen;


            int32_t pktid;
            int bytes_read_4 = mcpr_decode_varint(&pktid, pktbuf, len_left);
            if(unlikely(bytes_read_4 < 0)) {
                free_func(pktbuf);
                return -1;
            }

            if(set_compression_received && pktid == 0x02) { // It's the login success packet.

            } else if(pktid == 0x03) { // It's the set compression packet.
                len_left -= bytes_read_4;
                set_compression_received = true;

                // Handle set compression packet.
                int32_t treshold;
                int bytes_read_5 = mcpr_decode_varint(&treshold, pktbuf, len_left);
                if(unlikely(bytes_read_5 < 0)) {
                    free_func(pktbuf);
                    return -1;
                }

                // Enable compression..
                if(treshold >= 0) {
                    sess->compression_treshold = treshold;
                    sess->use_compression = true;
                }
            } else {
                free_func(pktbuf);
                return -1;
            }

            free_func(pktbuf);
        }

        sess->state = MCPR_STATE_PLAY;
    }

    return 0;

    #pragma GCC diagnostic pop
}

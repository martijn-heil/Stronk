#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <poll.h>
#include <unistd.h>

#include <mcpr/mcpr.h>
#include <mcpr/streams.h>

#include "util.h"
#include "stronk.h"

struct pollfd *fds

#define MAX_FD 1024

static struct connected_client {
    FILE *fp;
    bool use_encryption;

    enum mcpr_state state;
    bool use_compression;
    unsigned int compression_treshold; // Not guaranteed to be initialized if compression is set to false.

    bool use_encryption;
    EVP_CIPHER_CTX ctx_encrypt;  // Not guaranteed to be initialized if use_encryption is set to false
    EVP_CIPHER_CTX ctx_decrypt;  // Not guaranteed to be initialized if use_encryption is set to false
    int encryption_block_size;  // Not guaranteed to be initialized if use_encryption is set to false
};

// An array of 1024 pointers to connected_client's.
static struct connected_client **clients;


static int make_socket(uint16_t port);
static int read_from_client(int sockfd);
static bool handle_handshake_handshake(struct connected_client *client, FILE *in);
static bool handle_login_login_start(struct connected_client *client, FILE *in);


#ifndef HAVE_SECURE_RANDOM
#define HAVE_SECURE_RANDOM
static ssize_t secure_random(void *buf, size_t len) {
    int urandomfd = open("/dev/urandom", O_RDONLY);
    if(urandomfd == -1) {
        return -1;
    }

    ssize_t urandomread = read(urandomfd, buf, len);
    if(urandomread == -1) {
        return -1;
    }
    if(urandomread != len) {
        return -1;
    }

    close(urandomfd);

    return len;
}
#endif

// TODO
void net_on_packet(enum mcpr_state state, uint8_t pkt_id, /* function pointer.. */) {

}



void network_thread(void *arg) {
    nlog_info("Network thread started.");

    clients = malloc(MAX_FD * sizeof(clients));
    if(unlikely(clients == NULL)) {
        nlog_fatal("Could not allocate memory for connected clients array. (%s)");
        exit(EXIT_FAILURE);
    }


    uint16_t port = *((uint16_t *) arg);


    int sock;
    fd_set active_fd_set, read_fd_set;
    int i;
    struct sockaddr_in clientname;
    size_t size;

    /* Create the socket and set it up to accept connections. */
    nlog_info("Creating server socket..");
    sock = make_socket(port);
    if(sock < 0) {
        nlog_fatal("Could not create server socket. (%s ?)", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if(listen(sock, 1) < 0) {
        nlog_fatal("Could not listen on server socket. (%s ?)", strerror(errno));
        exit (EXIT_FAILURE);
    }

    /* Initialize the set of active sockets. */
    FD_ZERO(&active_fd_set);
    FD_SET(sock, &active_fd_set);

    while (true) {
        /* Block until input arrives on one or more active sockets. */
        read_fd_set = active_fd_set;
        if(select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
            nlog_warning("Select failed. (%s ?)", strerror(errno));
            // TODO should we crash here or not?
        }

        /* Service all the sockets with input pending. */
        for (i = 0; i < FD_SETSIZE; ++i) {
            if(FD_ISSET(i, &read_fd_set)) {
                if (i == sock) {
                    /* Connection request on original socket. */
                    int new;
                    size = sizeof(clientname);
                    new = accept(sock, (struct sockaddr *) &clientname, &size);
                    if (new < 0) {
                        nlog_warning("Could not accept incoming connection. (%s ?)", strerror(errno));
                    }
                    if(new > (MAX_FD - 1)) {
                        nlog_error("Maximum file descriptor count reached! Unable to accept incoming connection!");
                        close(new);
                    }

                    nlog_info("Server: connect from host %s, port %hd.\n", inet_ntoa(clientname.sin_addr), ntohs(clientname.sin_port));
                    FD_SET(new, &active_fd_set);
                    struct connected_client *client = malloc(sizeof(struct connected_client));
                    if(unlikely(client == NULL)) {
                        nlog_error("Could not alloate memory, unable to accept incoming connection.");

                        if(close(new) == -1) {
                            nlog_warning("Could not close file descriptor %i. (%s)", i, strerror(errno));
                        }
                    }
                    // TODO populate connected_client

                    clients[new] = client;
                }
                else {
                    /* Data arriving on an already-connected socket. */
                    if(read_from_client(i) < 0) {
                        if(close(i) == -1) {
                            nlog_warning("Could not close file descriptor %i. (%s)", i, strerror(errno));
                        }
                        FD_CLR(i, &active_fd_set);
                        free(clients[i]);
                    }
                }
            }
        }
    }
}

static int make_socket (uint16_t port) {
    struct sockaddr_in name;

    /* Create the socket. */
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        nlog_error("Could not create socket. (%s)", strerror(errno));
        return -1
    }

    /* Give the socket a name. */
    name.sin_family = AF_INET;
    name.sin_port = htons(port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sockfd, (struct sockaddr *) &name, sizeof (name)) < 0)
    {
        nlog_error("Could not bind socket to address. (%s)", strerror(errno));
        return -1;
    }

    return sockfd
}


static int read_from_client(int sockfd) {
    read_packet_from_client(connected_clients[sockfd])
}


static ssize_t read_packet_from_client(struct connected_client *client) {
    struct mcpr_packet *pkt = mcpr_read_packet(client->fd, client->use_compression, client->use_encryption, client->use_encryption ? client->encryption_block_size : 0, client->use_encryption ? &(client->ctx_encrypt) : NULL);
    if(pkt == NULL) goto err1;

    FILE *in = mcpr_open_packet(pkt);
    if(stream == NULL) { goto err2; }

    switch(pkt->state) {
        case MCPR_STATE_HANDSHAKE: {
            switch(pkt->id) {
                case 0x00: { if(!handle_handshake_handshake(client, in)) goto err3; }   // Handshake
                case 0xFE: { /* TODO handle Legacy server list ping */ }                // Legacy server list ping
            }
        }

        case MCPR_STATE_LOGIN: {
            switch(pkt->id) {
                case 0x00: { if(!handle_login_login_start(client, in)) goto err3; }     // Login start
            }
        }
    }



    if(fclose(in) == EOF) nlog_warning("Couldn't close file descriptor. (%s)", strerror());
    free(pkt->data);
    free(pkt);
    return 0;


    err3:
        if(fclose(in) == EOF) nlog_warning("Couldn't close file descriptor. (%s)", strerror());
    err2:
        free(pkt->data);
        free(pkt);
    err1:
        return -1;
}

static bool handle_handshake_handshake(struct connected_client *client, FILE *in) {
    int32_t protocol_version;
    uint16_t port;
    char **server_address;
    int32_t next_state;


    if(mcpr_read_varint(&protocol_version, in) < 0) goto err1;
    if(mcpr_read_string(&server_address, in) < 0) goto err1;
    if(mcpr_read_ushort(&port, in) < 0) goto err2;
    if(mcpr_read_varint(&next_state, in) < 0) goto err2;


    switch(next_state) {
        case 1: { client->state = MCPR_STATE_STATUS; }
        case 2: { client->state = MCPR_STATE_LOGIN; }
    }

    free(*server_address);
    return true;


    err2:
        free(*server_address);
    err1:
        return false;
}

static bool handle_login_login_start(struct connected_client *client, FILE *in) {
    bool is_localhost = true;
    bool is_unauthenticated = true;

    if(is_localhost || is_unauthenticated) {
        char *player_name;
        if(mcpr_read_string(&player_name, in) < 0) goto err1;

        char uuid[36];
        unsigned char uuid_version = is_unauthenticated ? 3 : 4;

        struct ninuuid tmpuuid;
        if(!ninuuid_genreate(&tmpuuid, uuid_version)) goto err1;
        ninuuid_to_string(&tmpuuid, uuid);


        struct mcpr_packet pkt;
        pkt->id = 0x02;

        FILE *out = open_memstream(&((char *) pkt->data), &(pkt->data_size));
        if(out == NULL) { goto err2; }

        if(mcpr_write_string(out, uuid) < 0) goto err3;
        if(mcpr_write_string(out, player_name) < 0) goto err3;
        if(fclose(out) == EOF) nlog_warning("Couldn't close file descriptor. (%s)", strerror());


        if(mcpr_write_packet(client->fd, &pkt, client->use_compression, client->force_no_compression,
            client->use_encryption, client->use_encryption ? client->encryption_block_size : 0,
            client->use_encryption ? &(client->ctx_encrypt) : NULL) < 0) { goto err2; }

        client->state = MCPR_STATE_PLAY;
        free(player_name);
        return true;


        err3:
            if(fclose(out) == EOF) nlog_warning("Couldn't close file descriptor. (%s)", strerror());
        err2:
            free(player_name);
        err1:
            return false;
    } else {
        struct mcpr_packet pkt;
        pkt->id = 0x01 // Encryption request


        char *server_id = "";
        int32_t pubkey_len 16;
        uint8_t pubkey[pubkey_len];
        int32_t verify_token_len = 16;
        uint8_t verify_token[verify_token_len];

        void *encoded_pubkey;
        int32_t encoded_pubkey_len;

        if(secure_random(pubkey, pubkey_len) < 0) goto err3;
        if(secure_random(verify_token, verify_token_len) < 0) goto err3;

        // TODO do public key generation and such. Populate the variables


        FILE *out = open_memstream(&((char *) pkt->data), &(pkt->data_size));
        if(out == NULL) { return false; }

        if(mcpr_write_string(out, server_id) < 0)                                                   { goto err1; }
        if(mcpr_write_varint(out, pubkey_len) < 0)                                                  { goto err1; }
        if(mcpr_write_byte_array(out, encoded_pubkey, (size_t) encoded_pubkey_len) < 0)             { goto err1; }
        if(mcpr_write_varint(out, verify_token_len) < 0)                                            { goto err1; }
        if(mcpr_write_byte_array(out, verfiy_token, (size_t) verify_token_len) < 0)                 { goto err1; }

        if(fclose(out) == EOF) nlog_warning("Couldn't close file descriptor. (%s)", strerror());


        if(mcpr_write_packet(client->fd, &pkt, client->use_compression, client->force_no_compression,
            client->use_encryption, client->use_encryption ? client->encryption_block_size : 0,
            client->use_encryption ? &(client->ctx_encrypt) : NULL) < 0) { goto err2; }


        free(pkt->data);
        return true;

        err1:
            if(fclose(out) == EOF) nlog_warning("Couldn't close file descriptor. (%s)", strerror());
        err2:
            free(pkt->data);
        err3:
            return false;
    }
}

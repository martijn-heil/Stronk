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

#include "util.h"
#include "stronk.h"

struct pollfd *fds

#define MAX_FD 1024

static struct connected_client {
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

    if(listen(sock, 1) < 0)
    {
        nlog_fatal("Could not listen on server socket. (%s ?)", strerror(errno));
        exit (EXIT_FAILURE);
    }

    /* Initialize the set of active sockets. */
    FD_ZERO(&active_fd_set);
    FD_SET(sock, &active_fd_set);

    while (true)
    {
        /* Block until input arrives on one or more active sockets. */
        read_fd_set = active_fd_set;
        if(select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0)
        {
            nlog_warning("Select failed. (%s ?)", strerror(errno));
            // TODO should we crash here or not?
        }

        /* Service all the sockets with input pending. */
        for (i = 0; i < FD_SETSIZE; ++i) {
            if(FD_ISSET(i, &read_fd_set))
            {
                if (i == sock)
                {
                    /* Connection request on original socket. */
                    int new;
                    size = sizeof(clientname);
                    new = accept(sock, (struct sockaddr *) &clientname, &size);
                    if (new < 0)
                    {
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

                    clients[new] = client;
                }
                else
                {
                    /* Data arriving on an already-connected socket. */
                    if(read_from_client(i) < 0)
                    {
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

}

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



    client.h - Client specific functions
*/
#ifndef MCPR_CLIENT_H
#define MCPR_CLIENT_H

#include <stdlib.h>
#include <stdbool.h>

#include "mcpr.h"

struct mcpr_client {
    bool is_connected;
    struct mcpr_connection conn; // Should be destroyed upon disconnection.


    bool is_online_mode;
    char *client_token; // Should be free'd using the free function specified with mcpr_set_free_func()
    char *access_token; // Should be free'd using the free function specified with mcpr_set_free_func()
    char *player_name; // Should be free'd.
    char *account_name; // Should be free'd.
};
/*
 * Frees the client's contents and the structure itself.
 */
void mcpr_client_destroy(struct mcpr_client *client);

/*
 * This function just a convenient way of allocating a mcpr_client structure.
 * Will return NULL upon error, or a malloc'ed mcpr_client struct.
 */
struct mcpr_client *mcpr_client_create(const char *client_token, const char *access_token, const char *account_name, bool online_mode);

/*
 * Generates a cryptographically secure client token string of length token_len.
 * buf should be at least the size of token_len
 */
int mcpr_client_generate_token(char *buf, size_t token_len);

/*
 * Connect a client to a server.
 *
 * Returns a negative integer upon error.
 */
int mcpr_client_connect(struct mcpr_client *client, const char *host, int port, unsigned int sock_timeout);


int mcpr_client_disconnect(struct mcpr_client *client);

#endif

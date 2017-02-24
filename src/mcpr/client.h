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

// #include <uuid/uuid.h>
//
// #include "mcpr.h"
//
// typedef void * mcpr_client;
//
// /*
//  * Frees the client's contents and the structure itself.
//  */
// void mcpr_client_destroy(mcpr_client client);
//
// /**
//  * Create a client.
//  *
//  * @param [in] player_name Player name. Must not be any longer than (SIZE_MAX - 1) (not taking the NULL byte into account).
//  *
//  * @param [in] account_name Account name. Must not be any longer than (SIZE_MAX - 1) (not taking the NULL byte into account).
//  * May be NULL if online_mode is false. May be NULL if legacy is true
//  *
//  * @param [in] access_token Valid access token. Must not be any longer than (SIZE_MAX - 1) (not taking the NULL byte into account). May be NULL if online_mode is false.
//  *
//  * @param [in] client_token Client token. Must not be any longer than (SIZE_MAX - 1) (not taking the NULL byte into account).
//  *
//  * @param [in] online_mode True if
//  * @returns NULL upon error, or a malloc'ed mcpr_client struct.
//  */
// mcpr_client mcpr_client_create(const char *client_token, const char *access_token, const char *account_name, const char *player_name, bool online_mode, bool legacy);
//
// /**
//  * Connect a client to a server.
//  *
//  * @param [in] host Hostname of server.
//  * @param [in] port Port of server.
//  * @param [in] sock_timeout Timeout for individual socket reads/writes. Note that this is not an overall timeout for this function.
//  * @param [in, out] client Client to connect to a server.
//  * @returns A negative integer upon error.
//  */
// int mcpr_client_connect(mcpr_client client, const char *host, short port, unsigned int sock_timeout);
//
//
// int mcpr_client_disconnect(mcpr_client client);
//
//
// bool mcpr_client_is_connected(mcpr_client client);
//
// bool mcpr_client_is_legacy(mcpr_client client);
// bool mcpr_client_is_online_mode(mcpr_client client);
// void mcpr_client_get_uuid(uuid_t out, mcpr_client client);
//
//
// ssize_t mcpr_client_write_packet(mcpr_client client, struct mcpr_packet *pkt, bool force_no_compression);
//
// /**
//  * Writes the contents of data to the connection.
//  * Does encryption if encryption is enabled for the specified connection.
//  *
//  * @param [in] conn The connection to write the raw data over, may not be NULL.
//  * @param [in] data The raw binary data to write over the connection. May not be NULL. Should be at least the size of len.
//  * @param [in] len The amount of bytes to write.
//  *
//  * @returns the amount of bytes written or a negative integer upon error.
//  */
// ssize_t mcpr_client_write_raw(mcpr_client client, const void *data, size_t len);
//
// struct mcpr_packet *mcpr_client_read_packet(mcpr_client client);

#endif

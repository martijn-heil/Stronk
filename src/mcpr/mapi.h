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



    mapi.h - C interface to Mojang's web API. Short for *M*ojang *API*
*/
#ifndef MCPR_MOJANG_API_H
#define MCPR_MOJANG_API_H

#include <stdbool.h>
#include <stdlib.h>

#include <uuid/uuid.h>

unsigned int *mapi_get_errno();
/**
 * mapi_errno is guaranteed to be thread local.
 */
#define mapi_errno (*mapi_get_errno())


struct mapi_auth_response {
    char *access_token; // Should be free'd
    char *client_token; // Should be free'd

    size_t available_profiles_amount;
    struct {
        char *id;       // Should be free'd
        char *name;     // Should be free'd
        bool legacy;
    } *available_profiles; // Array of available profiles, the structs themselves should not be free'd, may not be NULL.

    struct {
        char *id;       // Should be free'd
        char *name;     // Should be free'd
        bool legacy;
    } selected_profile;

    bool user_present;
    struct {
        char *id; // should be free'd.

        char *preferred_language; // might be NULL. Should be free'd.
        char *twitch_access_token; // might be NULL. Should be free'd.
    } user; // only present if request_user was specified
};
void mapi_auth_response_destroy(struct mapi_auth_response *response);


struct mapi_refresh_response {
    char *access_token;
    char *client_token;

    struct {
        char *id;
        char *name;
    } selected_profile;

    bool user_present;
    struct {
        char *id;

        char *preferred_language; // Might be NULL.
        char *twitch_access_token; // Might be NULL.
    } user;
};
void mapi_refresh_response_destroy(struct mapi_refresh_response *response);

enum mapi_agent {
    MAPI_AGENT_MINECRAFT,
    MAPI_AGENT_SCROLLS,
};


/**
 * Authenticates a user using their password.
 *
 * Endpoint: /authenticate
 * @see http://wiki.vg/Authentication#Authenticate
 * @note This function does I/O, be aware that this could take some time.
 *
 * @param account_name Can be either an email address or player name for unmigrated accounts.
 * @param client_token Client token, may be NULL.
 * @returns The response from the authentication server, or NULL upon error.
 *
 *
 * If an error occurs, mapi_errno will be set to either one of the values below. Please take note that error conditions are not 100% guaranteed to be accurate.
 *
 * 0. Unknown error occurred, this could be anything, including but not limited to the errors listed below.
 *
 * 1. Invalid credentials. Account migrated, use e-mail as username.
 *
 * 2. Invalid credentials.
 *
 * 3. Remote host could not be resolved.
 *
 * 4. Could not connect to remote host.
 *
 * 5. Out of memory, errno is not guaranteed to be set.
 *
 * 6. Malloc failure, errno will be set.
 *
 * 7. Request timeout.
 *
 * 8. Could not send data to authentication server.
 *
 * 9. Error receiving data from the authentication server.
 */
struct mapi_auth_response *mapi_auth_authenticate(enum mapi_agent, int version, const char *restrict account_name, const char *restrict password, const char *restrict client_token, bool request_user);

/**
 * Refreshes a valid access token.
 * It can be used to keep a user logged in between gaming sessions and is preferred over storing the user's password.
 *
 * Endpoint: /refresh
 * @see http://wiki.vg/Authentication#Refresh
 * @note This function does I/O, be aware that this could take some time.
 *
 * @param [in] access_token Access token, may not be NULL.
 * @param [in] client_token Client token, may not be NULL.
 * @param [in] request_user True if you want the user object in the response.
 *
 * Note that the provided access_token will be invalidated.
 * @returns The response from the authentication server, or NULL upon error.
 */
struct mapi_refresh_response *mapi_auth_refresh(const char *restrict access_token, const char *restrict client_token, bool request_user);

/**
 * Checks if an access token is usable for authentication with a Minecraft server.
 *
 *
 * Endpoint: /validate
 * @see http://wiki.vg/Authentication#Validate
 * @note This function does I/O, be aware that this could take some time.
 *
 * @param [in] access_token Access token, may not be NULL.
 * @param [in] client_token Client token, may be NULL.
 *
 * @retval Negative integer upon error.
 * @retval 0 if not valid.
 * @retval 1 if valid.
 */
int mapi_auth_validate(const char *restrict access_token, const char *restrict client_token);

/**
 * Invalidates accessTokens using a client/access token pair.
 *
 * Endpoint: /invalidate
 * @see http://wiki.vg/Authentication#Invalidate
 * @note This function does I/O, be aware that this could take some time.
 *
 * @param [in] access_token Access token. May not be NUlL.
 * @param [in] client_token Client token. May not be NULL.
 *
 * @returns Negative integer upon error.
 */
int mapi_auth_invalidate(const char *restrict access_token, const char *restrict client_token);


/**
 * Convenient function for generating a cryptographically secure random client token.
 * Will write an alphanumerical NULL-terminated ASCII string of token_len characters (plus 1 NULL byte) to buf.
 *
 * @param [in] token_len Token length, should be an even number.
 * @param [out] buf Output buffer, should be at least the size of token_len + 1
 *
 * @returns Negative integer upon error.
 */
int mapi_generate_client_token(char *restrict buf, size_t token_len);

/**
 * Get the UUID associated with a player name at this point in time.
 *
 * @note This function does I/O, be aware that this could take some time.
 *
 * @param player_name
 * @returns Negative integer upon error. mapi_errno will be set to either one of the following values if an error occurs;
 *
 * 0: Unknown error occurred.
 * 1: Player not found.
 */
int mapi_username_to_uuid(uuid_t output, const char *restrict player_name);

#endif

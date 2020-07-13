/*
  MIT License

  Copyright (c) 2016-2020 Martijn Heil

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

#include <ninuuid/ninuuid.h>
#include <ninerr/ninerr.h>
#include <ninio/bstream.h>
#include <ninio/logging.h>

extern struct logger *mapi_logger; // set to non null value to enable logging, set to null value to disable logging.

struct mapi_profile
{
  struct ninuuid uuid;     // Should be free'd
  char *name;   // Should be free'd
  bool legacy;
};

struct mapi_auth_response
{
  char *access_token; // Should be free'd
  char *client_token; // Should be free'd

  size_t available_profiles_amount;
  struct mapi_profile *available_profiles; // Array of available profiles, the structs themselves should be free'd, may not be NULL.
  struct mapi_profile selected_profile;

  bool user_present;
  struct
  {
    char *id; // should be free'd.

    char *preferred_language; // might be NULL. Should be free'd.
    char *twitch_access_token; // might be NULL. Should be free'd.
  } user; // only present if request_user was specified
};
void mapi_auth_response_destroy(struct mapi_auth_response *response);


struct mapi_refresh_response
{
  char *access_token;
  char *client_token;

  struct
  {
    char *id;
    char *name;
  } selected_profile;

  bool user_present;
  struct
  {
    char *id;

    char *preferred_language; // Might be NULL.
    char *twitch_access_token; // Might be NULL.
  } user;
};
void mapi_refresh_response_destroy(struct mapi_refresh_response *response);

struct mapi_minecraft_has_joined_response
{
  struct ninuuid id;
  char *player_name;
  struct
  {
    char *skin_blob_base64;
    char *signature;
  } properties;
};
void mapi_minecraft_has_joined_response_destroy(struct mapi_minecraft_has_joined_response *response);

enum mapi_agent
{
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
 * @returns The response from the authentication server, or NULL upon error. If an error occurs, ninerr will be set.
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
 * @returns The response from the authentication server, or NULL upon error. If an error occurs, ninerr will be set.
 */
struct mapi_refresh_response *mapi_auth_refresh(const char *restrict access_token, const char *restrict client_token, bool request_user);

enum mapi_auth_validate_result
{
  MAPI_AUTH_VALIDATE_RESULT_SUCCESS,
  MAPI_AUTH_VALIDATE_RESULT_ERROR,
  MAPI_AUTH_VALIDATE_RESULT_FAILED
};
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
 * @returns false upon error, will set ninerr.
 */
 enum mapi_auth_validate_result mapi_auth_validate(const char *restrict access_token, const char *restrict client_token);

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
 * @returns false upon error, will set ninerr.
 */
bool mapi_auth_invalidate(const char *restrict access_token, const char *restrict client_token);


/**
 * Convenient function for generating a cryptographically secure random client token.
 * Will write an alphanumerical NULL-terminated ASCII string of token_len characters (plus 1 NULL byte) to buf.
 *
 * @param [in] token_len Token length, should be an even number.
 * @param [out] buf Output buffer, should be at least the size of token_len + 1
 *
 * @returns false upon error, will set ninerr.
 */
bool mapi_generate_client_token(char *restrict buf, size_t token_len);

/**
 * Get the UUID associated with a player name at this point in time.
 *
 * @note This function does I/O, be aware that this could take some time.
 *
 * @param [in] player_name The username.
 * @param [out] out ninuuid to write to, will be corrupted if an error occurs.
 * @returns False upon error, ninerr will be set.
 */
bool mapi_username_to_uuid(struct ninuuid *out, const char *restrict player_name);


struct mapi_err_authserver_err
{
  struct ninerr super;
  char *error;
  char *friendly_error_message;
  char *cause_message; // may be NULL.
  int http_code;
};
struct mapi_err_authserver_err *mapi_err_authserver_err_new(const char *error, const char *friendly_error_message, const char *cause_message, int http_code);


struct mapi_minecraft_has_joined_response *mapi_minecraft_has_joined(const char *username, const char *server_id_hash, const char *ip);

#endif

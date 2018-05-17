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



    mapi.c - C interface to Mojang's web API. Short for *M*ojang *API*
*/

#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>

#include <unistd.h>
#include <sys/fcntl.h>

#include <ninerr/ninerr.h>
#include <curl/curl.h>
#include <jansson/jansson.h>
#include <c11threads.h>

#include "mapi.h"
#include "util.h"

#ifndef __FILENAME__
    #if defined(WIN32) || defined(_WIN32) || defined(_WIN64) && !defined(__CYGWIN__)
        #define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
    #else
        #define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
    #endif
#endif

#ifdef __GNUC__
    #ifndef likely
        #define likely(x)       __builtin_expect(!!(x), 1)
    #endif

    #ifndef unlikely
        #define unlikely(x)     __builtin_expect(!!(x), 0)
    #endif
#else
    #ifndef likely
        #define likely(x) (x)
    #endif

    #ifndef unlikely
        #define unlikely(x) (x)
    #endif
#endif

struct logger *mapi_logger = NULL;

#include <stdarg.h>
static void debug_print(const char *filename, size_t filename_len, const char *func, size_t func_len, int line, ...)
{
    if(mapi_logger == NULL) return;
    va_list ap;
    va_start(ap, line);
    const char *fmt = va_arg(ap, const char *);
    char final_fmt[strlen(fmt) + strlen("\n") + 1];
    if(sprintf(final_fmt, "%s\n", fmt) < 0) { va_end(ap); return; }
    logger_vwrite(mapi_logger, filename, filename_len, func, func_len, line, LOG_LEVEL_DEBUG, final_fmt, ap);
    va_end(ap);
}

#define DEBUG_PRINT(...) debug_print(__FILENAME__, sizeof(__FILENAME__)-1, __func__, sizeof(__func__)-1, __LINE__, __VA_ARGS__);

#ifndef HAVE_SECURE_RANDOM
#define HAVE_SECURE_RANDOM
static ssize_t secure_random(void *buf, size_t len) {
    int urandomfd = open("/dev/urandom", O_RDONLY);
    if(urandomfd < 0) {
        return -1;
    }

    ssize_t urandomread = read(urandomfd, buf, len);
    if(urandomread < 0) {
        return -1;
    }
    if((size_t) urandomread != len) {
        return -1;
    }

    close(urandomfd);

    return len;
}
#endif

/**
 * Converts a "compressed" UUID in string representation without hyphens,
 * to it's "uncompressed" (official/standardized) representation with hyphens.
 *
 * @param [out] output Output buffer. Should be at least of size 37. Will be NUL terminated by this function.
 * @param [in] input Input buffer. Should be at least of size 32. Does not have to be NUL terminated.
 */
static void mapi_uuid_decompress_string(char *restrict output, const char *restrict input) {

    // Not using memory copy functions for a good reason, I think the following is a lot clearer than some pointer & copy magic.
    output[0] = input[0];
    output[1] = input[1];
    output[2] = input[2];
    output[3] = input[3];
    output[4] = input[4];
    output[5] = input[5];
    output[6] = input[6];
    output[7] = input[7];

    output[8] = '-';

    output[9] = input[8];
    output[10] = input[9];
    output[11] = input[10];
    output[12] = input[11];

    output[13] = '-';

    output[14] = input[12];
    output[15] = input[13];
    output[16] = input[14];
    output[17] = input[15];

    output[18] = '-';

    output[19] = input[16];
    output[20] = input[17];
    output[21] = input[18];
    output[22] = input[19];

    output[23] = '-';

    output[24] = input[20];
    output[25] = input[21];
    output[26] = input[22];
    output[27] = input[23];
    output[28] = input[24];
    output[29] = input[25];
    output[30] = input[26];
    output[31] = input[27];
    output[32] = input[28];
    output[33] = input[29];
    output[34] = input[30];
    output[35] = input[31];

    output[36] = '\0';
}

static void mapi_uuid_compress_string(char *restrict output, const char *restrict input) {
    output[0] = input[0];
    output[1] = input[1];
    output[2] = input[2];
    output[3] = input[3];
    output[4] = input[4];
    output[5] = input[5];
    output[6] = input[6];
    output[7] = input[7];

    output[8] = input[9];
    output[9] = input[10];
    output[10] = input[11];
    output[11] = input[12];

    output[12] = input[14];
    output[13] = input[15];
    output[14] = input[16];
    output[15] = input[17];

    output[16] = input[19];
    output[17] = input[20];
    output[18] = input[21];
    output[19] = input[22];

    output[20] = input[24];
    output[21] = input[25];
    output[22] = input[26];
    output[23] = input[27];
    output[24] = input[28];
    output[25] = input[29];
    output[26] = input[30];
    output[27] = input[31];
    output[28] = input[32];
    output[29] = input[33];
    output[30] = input[34];
    output[31] = input[35];

    output[32] = '\0';
}

enum mapi_http_method {
    MAPI_HTTP_GET,
    MAPI_HTTP_HEAD,
    MAPI_HTTP_POST,
    MAPI_HTTP_PUT,
    MAPI_HTTP_DELETE,
};
IGNORE("-Wreturn-type")
const char *mapi_http_method_to_string(enum mapi_http_method method)
{
    switch(method)
    {
        case MAPI_HTTP_GET: return "GET";
        case MAPI_HTTP_HEAD: return "HEAD";
        case MAPI_HTTP_POST: return "POST";
        case MAPI_HTTP_PUT: return "PUT";
        case MAPI_HTTP_DELETE: return "DELETE";
    }
}
END_IGNORE()

static size_t mapi_curl_write_callback(void *contents, size_t size, size_t nmemb, void *arg);

// Returns NULL on error or a malloc'ed mcpr_auth_response, which should be free'd
struct mapi_auth_response *json_to_auth_response(json_t *json);
struct mapi_refresh_response *json_to_refresh_response(json_t *json);
static int make_authserver_request(json_t **response, const char *endpoint, const char *payload);
static int mapi_make_api_request(json_t **output, const char *url, enum mapi_http_method http_method, char **headers, size_t header_count, const char *payload);
static struct mapi_err_authserver_err *mapi_err_authserver_err_from_json(json_t *json, int http_code);
static void mapi_err_authserver_err_free(struct ninerr *err);
static struct mapi_minecraft_has_joined_response *mapi_minecraft_has_joined_response_from_json(json_t *json);

struct mapi_curl_buffer {
    char *content;
    size_t size;
};

struct mapi_authserver_error {
    char *error;
    char *error_message;
    char *cause; // May be NULL.
};
static struct mapi_authserver_error *json_to_authserver_error(json_t *json);
static void mapi_authserver_error_destroy(struct mapi_authserver_error *error);




 enum mapi_auth_validate_result mapi_auth_validate(const char *restrict access_token, const char *restrict client_token) {
    char *fmt;
    int required;
    if(client_token != NULL) {
        fmt = "{\"accessToken\":\"%s\",\"clientToken\":\"%s\"}";
        char tmpbuf;
        required = snprintf(&tmpbuf, 1, fmt, access_token, client_token);
        ninerr_set_err(ninerr_new("snprintf failed.", false));
        if(required < 0) { return MAPI_AUTH_VALIDATE_RESULT_ERROR; }
    } else {
        fmt = "{\"accessToken\":\"%s\"}";
        char tmpbuf;
        required = snprintf(&tmpbuf, 1, fmt, access_token);
        ninerr_set_err(ninerr_new("snprintf failed.", false));
        if(required < 0) { return MAPI_AUTH_VALIDATE_RESULT_ERROR; }
    }
    char payload[required + 1];
    if(client_token != NULL) {
        int result = sprintf(&payload[0], fmt, access_token, client_token);
        ninerr_set_err(ninerr_new("sprintf failed.", false));
        if(result < 0) { return MAPI_AUTH_VALIDATE_RESULT_ERROR; }
    } else {
        int result = sprintf(&payload[0], fmt, access_token);
        ninerr_set_err(ninerr_new("sprintf failed.", false));
        if(result < 0) { return MAPI_AUTH_VALIDATE_RESULT_ERROR; }
    }

    json_t *json_response = NULL;
    int request_result = make_authserver_request(&json_response, "validate", payload);
    if(request_result < 0)
    {
        if(ninerr != NULL && strcmp(ninerr->type, "mapi_err_authserver_err") == 0 && ((struct mapi_err_authserver_err *) ninerr->child)->http_code == 403)
        {
            return MAPI_AUTH_VALIDATE_RESULT_FAILED;
        }
        else
        {
            return MAPI_AUTH_VALIDATE_RESULT_ERROR;
        }
    }
    json_decref(json_response);
    return MAPI_AUTH_VALIDATE_RESULT_SUCCESS; // Valid! :D
}

struct mapi_refresh_response *mapi_auth_refresh(const char *restrict access_token, const char *restrict client_token, bool request_user) {

    // This is inefficient, but the safest way to determine how much space we need.
    // Safety above all right? :P
    char *fmt = "{\"accessToken\":\"%s\",\"clientToken\":\"%s\",\"requestUser\":%s}";
    char tmpbuf;
    int required = snprintf(&tmpbuf, 1, fmt, access_token, client_token, request_user?"true":"false");
    if(required < 0) { return NULL; }
    char payload[required + 1];
    int result = sprintf(&payload[0], fmt, access_token, client_token, request_user?"true":"false");
    if(result < 0) { return NULL; }

    json_t *json_response;
    int request_result = make_authserver_request(&json_response, "refresh", payload);
    if(request_result < 0) { return NULL; }

    struct mapi_refresh_response *res = json_to_refresh_response(json_response);
    if(res == NULL) { return NULL; }
    return res;
}

struct mapi_auth_response *mapi_auth_authenticate(enum mapi_agent agent, int version, const char *restrict account_name, const char *restrict password, const char *restrict client_token, bool request_user) {
    char *agent_name;
    switch(agent) {
        case MAPI_AGENT_MINECRAFT:
            agent_name = "Minecraft";
            break;

        case MAPI_AGENT_SCROLLS:
            agent_name = "Scrolls";
            break;

        default:
            fprintf(stderr, "Aborting at %s:%i", __FILENAME__, __LINE__);
            abort(); // This shouldn't ever be reached!
            break;
    }

    // This is inefficient, but the safest way to determine how much space we need.
    // Safety above all right? :P
    char *fmt;
    int required;
    if(client_token == NULL) {
        char tmpbuf;
        fmt = "{\"agent\":{\"name\":\"%s\",\"version\":%d},\"username\":\"%s\",\"password\":\"%s\",\"requestUser\":%s}";
        required = snprintf(&tmpbuf, 1, fmt, agent_name, 1, account_name, password, request_user?"true":"false");
        if(required < 0) { ninerr_set_err(ninerr_new("snprintf failed.", false)); return NULL; }
    } else {
        char tmpbuf;
        fmt = "{\"agent\":{\"name\":\"%s\",\"version\":%d},\"username\":\"%s\",\"password\":\"%s\",\"clientToken\":\"%s\",\"requestUser\":%s}";
        required = snprintf(&tmpbuf, 1, fmt, agent_name, 1, account_name, password, client_token, request_user?"true":"false");
        if(required < 0) { ninerr_set_err(ninerr_new("snprintf failed.", false)); return NULL; }
    }

    char payload[required + 1];
    if(client_token != NULL) {
        int result = sprintf(&payload[0], fmt, agent_name, 1, account_name, password, client_token, request_user?"true":"false");
        if(result < 0) {
            ninerr_set_err(ninerr_new("sprintf failed.", false));
            return NULL;
        }
    } else {
        int result = sprintf(&payload[0], fmt, agent_name, 1, account_name, password, request_user?"true":"false");
        if(result < 0) {
            ninerr_set_err(ninerr_new("sprintf failed.", false));
            return NULL;
        }
    }



    json_t *response = NULL;
    int http_status = make_authserver_request(&response, "authenticate", &payload[0]);
    if(http_status < 0)
    {
        if(response != NULL) { json_decref(response); }
        return NULL;
    }

    struct mapi_auth_response *res = json_to_auth_response(response);
    json_decref(response);
    if(res == NULL)
    {
        json_decref(response);
        ninerr_set_err(NULL);
        return NULL;
    }
    free(payload);
    return res;
}


void mapi_auth_response_destroy(struct mapi_auth_response *response)
{
    free(response->access_token);
    free(response->client_token);

    for(unsigned int i = 0; i < response->available_profiles_amount; i++)
    {
        free(response->available_profiles + i);
    }

    free(response->selected_profile.name);

    if(response->user_present)
    {
        free(response->user.id);
        free(response->user.preferred_language);
        free(response->user.twitch_access_token);
    }

    free(response);
}

void mapi_refresh_response_destroy(struct mapi_refresh_response *response)
{
    free(response->access_token);
    free(response->client_token);
    free(response->selected_profile.name);

    if(response->user_present)
    {
        free(response->user.id);
        free(response->user.preferred_language);
        free(response->user.twitch_access_token);
    }

    free(response);
}


bool mapi_generate_client_token(char *restrict buf, size_t token_len)
{
    int i = 0;
    while(token_len > 0)
    {
        char tmp;
        if(secure_random(&tmp, 1) < 0) { ninerr_set_err(ninerr_from_errno()); return false; }

        if(isalnum(tmp))
        {
            buf[i] = tmp;
            i++;
            token_len--;
        }
    }
    buf[i] = '\0';

    return true;
}

bool mapi_username_to_uuid(struct ninuuid *output, const char *restrict player_name)
{
    const char *fmt = "https://api.mojang.com/users/profiles/minecraft/%s";
    char url[strlen(fmt) + strlen(player_name) + 1];
    sprintf(&url[0], fmt, player_name);
    json_t *response;
    int status = mapi_make_api_request(&response, url, MAPI_HTTP_GET, NULL, 0, NULL);
    if(status < 0) { return false; }
    if(status == 204) { ninerr_set_err(ninerr_new("No player found.", false)); return false; } // No player found.

    json_t *compressed_uuid_json = json_object_get(response, "id");
    if(compressed_uuid_json == NULL) { ninerr_set_err(NULL); return false; }
    const char *compressed_uuid = json_string_value(compressed_uuid_json);
    if(compressed_uuid == NULL) { ninerr_set_err(NULL); return false; } // Not a string.
    if(strlen(compressed_uuid) != 32) { ninerr_set_err(NULL); return false; } // UUID is malformed, it is not 32 characters long.

    if(!ninuuid_from_string(output, compressed_uuid)) { ninerr_set_err(NULL); return false; }
    return true;
}

struct mapi_err_authserver_err *mapi_err_authserver_err_new(const char *error, const char *friendly_error_message, const char *cause_message, int http_code)
{
    size_t error_len = strlen(error) + 1;
    size_t friendly_error_message_len = strlen(friendly_error_message) + 1;
    size_t cause_message_len = 0;
    if(cause_message != NULL) cause_message_len = strlen(cause_message) + 1;

    size_t required_size = sizeof(struct mapi_err_authserver_err) + error_len + friendly_error_message_len + cause_message_len;
    char tmpbuf;
    int message_required;
    char *message_fmt;
    if(cause_message != NULL)
    {
        message_fmt = "%s: %s. Cause: %s";
        message_required = snprintf(&tmpbuf, 1, message_fmt, error, friendly_error_message, cause_message);
    }
    else
    {
        message_fmt = "%s: %s.";
        message_required = snprintf(&tmpbuf, 1, message_fmt, error, friendly_error_message);
    }
    if(message_required < 0) return NULL;
    required_size += message_required;


    struct mapi_err_authserver_err *err = malloc(required_size);
    if(err == NULL) { return NULL; }
    void *strings = err + sizeof(struct mapi_err_authserver_err);
    char *error_buf = strings;
    char *friendly_error_message_buf = strings + error_len;
    char *cause_message_buf = friendly_error_message_buf + friendly_error_message_len;
    char *message_buf = cause_message_buf + cause_message_len;
    memcpy(error_buf, error, error_len);
    memcpy(friendly_error_message_buf, friendly_error_message, friendly_error_message_len);
    if(cause_message_len > 0) memcpy(cause_message_buf, cause_message, cause_message_len);
    if(cause_message != NULL)
    {
        int result = sprintf(message_buf, message_fmt, error, friendly_error_message, cause_message);
        if(result < 0) { free(err); return NULL; }
    }
    else
    {
        int result = sprintf(message_buf, message_fmt, error, friendly_error_message, cause_message);
        if(result < 0) { free(err); return NULL; }
    }
    err->super.free = mapi_err_authserver_err_free;
    err->super.cause = NULL;
    err->super.type = "mapi_err_authserver_err";
    err->super.child = err;
    err->super.message = message_buf;
    err->http_code = http_code;
    return err;
}

struct mapi_minecraft_has_joined_response *mapi_minecraft_has_joined(const char *username, const char *server_id_hash, const char *ip)
{
    DEBUG_PRINT("in mapi_minecraft_has_joined(username = %s, server_id_hash = %s, ip = %s)", username, server_id_hash, ip);

    //const char *fmt = "https://sessionserver.mojang.com/session/minecraft/hasJoined?username=%s&serverId=%s&ip=%s";
    const char *fmt = "https://sessionserver.mojang.com/session/minecraft/hasJoined?username=%s&serverId=%s";
    // CURL *curl = curl_easy_init();
    // if(curl == NULL) { ninerr_set_err(ninerr_new("Could not initialize CURL object.")); return NULL; }


    // TODO: escape username
    //char *escaped_username = curl_easy_escape(username, );
    // curl_easy_cleanup(curl);

    char url[strlen(server_id_hash) + 84 + strlen(username) + strlen(ip) + 1];
    sprintf(url, fmt, username, server_id_hash, ip);
    json_t *response;
    int status = mapi_make_api_request(&response, url, MAPI_HTTP_GET, NULL, 0, NULL);
    if(status < 0) { return false; }

    return mapi_minecraft_has_joined_response_from_json(response);
}
// Big whitespace to divide actual functions from private helper functions.


























/**
 * mapi_errno error codes: (outdated, kept here for documentation purposes for a little bit )
 *
 * 0: Unknown error occurred, this could be anything, including but not limited to the errors listed below.
 * 1: Remote host could not be resolved.
 * 2: Could not connect to remote host.
 * 3: Out of memory, errno is not guaranteed to be set.
 * 4: Malloc failure, errno will be set.
 * 5: Request timeout.
 * 6: Could not send data over network.
 * 7: Error receiving data from the authentication server.
 * 8: Authentication server returned an error.
 */
static int make_authserver_request(json_t **response, const char *endpoint, const char *payload)
{
    char *url_prefix = "https://authserver.mojang.com/";
    char url[strlen(url_prefix) + strlen(endpoint) + 1];
    strcpy(url, url_prefix);
    strcat(url, endpoint);

    struct mapi_curl_buffer curl_buf;
    curl_buf.content = malloc(1);
    if(curl_buf.content == NULL)
    {
        ninerr_set_err(ninerr_from_errno());
        return -1;
    }
    curl_buf.size = 0;


    CURL *curl = curl_easy_init();
    CURLcode res;
    if(curl == NULL)
    {
        free(curl_buf.content);
        ninerr_set_err(ninerr_new("Failed to initialize CURL.", false));
        return -1;
    }

    struct curl_slist *chunk = NULL;
    chunk = curl_slist_append(chunk, "Content-Type: application/json");
    chunk = curl_slist_append(chunk, "Accept: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "MAPI/1.0");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, mapi_curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curl_buf);

    res = curl_easy_perform(curl);
    if(res != CURLE_OK)
    {
        free(curl_buf.content);
        switch(res)
        {
            case CURLE_COULDNT_RESOLVE_HOST:    ninerr_set_err(ninerr_new("Curl error: Could not resolve host.", false));   break;
            case CURLE_COULDNT_CONNECT:         ninerr_set_err(ninerr_new("Curl error: Could not connect.", false));        break;
            case CURLE_OUT_OF_MEMORY:           ninerr_set_err(&ninerr_out_of_memory_struct);                               break;
            case CURLE_OPERATION_TIMEDOUT:      ninerr_set_err(ninerr_new("Curl error: Operation timed out.", false));      break;
            case CURLE_SEND_ERROR:              ninerr_set_err(ninerr_new("Curl error: Error sending.", false));            break;
            case CURLE_RECV_ERROR:              ninerr_set_err(ninerr_new("Curl error: Error receiving.", false));          break;

            default: ninerr_set_err(NULL); break;
        }

        curl_easy_cleanup(curl);
        return -1;
    }

    long http_code = 0;
    curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code != 200 && http_code != 204) // Authentication server returned an error.
    {
        json_error_t json_error;
        json_t *error_response = json_loadb(curl_buf.content, curl_buf.size, 0, &json_error);
        if(error_response == NULL)
        {
            free(curl_buf.content);
            ninerr_set_err(NULL);
            curl_easy_cleanup(curl);
            return -1;
        }

        struct mapi_err_authserver_err *err = mapi_err_authserver_err_from_json(error_response, http_code);
        ninerr_set_err(&(err->super));
        json_decref(error_response);
        free(curl_buf.content);
        curl_easy_cleanup(curl);
        return -1;
    }

    json_error_t json_error;
    *response = json_loadb(curl_buf.content, curl_buf.size, 0, &json_error);
    if(response == NULL)
    {
        free(curl_buf.content);
        ninerr_set_err(NULL);
        curl_easy_cleanup(curl);
        return -1;
    }

    curl_easy_cleanup(curl);
    free(curl_buf.content);
    return 0;
}

static int mapi_make_api_request(json_t **output, const char *url, enum mapi_http_method http_method, char **headers, size_t header_count, const char *payload) {
    DEBUG_PRINT("in mapi_make_api_request(), arguments: url: %s, http_method: %s, header_count: %zu, payload: %s", url, mapi_http_method_to_string(http_method), header_count, payload);

    CURL *curl = curl_easy_init();
    if(curl == NULL) { ninerr_set_err(ninerr_new("Could not initialize CURL.")); return -1; }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "MAPI/1.0");

    struct curl_slist *chunk = NULL;
    if(headers != NULL) {
        for(unsigned int i = 0; i < header_count; i++) {
            chunk = curl_slist_append(chunk, headers[i]);
        }
    }

    if(payload != NULL) {
        chunk = curl_slist_append(chunk, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
    }

    //chunk = curl_slist_append(chunk, "Accept: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);


    struct mapi_curl_buffer curl_buf;
    curl_buf.content = malloc(1);
    if(curl_buf.content == NULL) {
        ninerr_set_err(ninerr_from_errno());
        return -1;
    }
    curl_buf.size = 0;
    if(http_method != MAPI_HTTP_HEAD) {
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, mapi_curl_write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curl_buf);
    }

    IGNORE("-Wswitch")
    switch(http_method) {
        case MAPI_HTTP_HEAD:
            curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
            break;

        case MAPI_HTTP_PUT:
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
            break;

        case MAPI_HTTP_DELETE:
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "delete");
            break;
    }
    END_IGNORE()

    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK)
    {
        ninerr_set_err(ninerr_new("Could not curl_easy_perform (%s)", curl_easy_strerror(res)));
        free(curl_buf.content);
        return -1;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    DEBUG_PRINT("Returned HTTP code is %ld", http_code);


    json_error_t json_error;

    #ifdef DEBUG
        char tmp[curl_buf.size + 1];
        memcpy(tmp, curl_buf.content, curl_buf.size);
        tmp[curl_buf.size] = '\0';
        DEBUG_PRINT("Content of curl buf(len = %zu): %s\n", curl_buf.size, tmp);
    #endif
    *output = json_loadb(curl_buf.content, curl_buf.size, 0, &json_error);
    if(*output == NULL)
    {
        free(curl_buf.content);
        ninerr_set_err(ninerr_new("Error loading JSON. (text: %s, source: %s, line: %d, column: %d, position: %d)", json_error.text, json_error.source, json_error.line, json_error.column, json_error.position));
        return -1;
    }

    curl_easy_cleanup(curl);
    free(curl_buf.content);
    if(http_code > INT_MAX) { ninerr_set_err(ninerr_arithmetic_new()); return -1; }
    return (int) http_code;
}

static size_t mapi_curl_write_callback(void *contents, size_t size, size_t nmemb, void *arg)
{
    struct mapi_curl_buffer *buf = arg;
    size_t real_size = size * nmemb;
    buf->content = realloc(buf->content, buf->size + real_size);
    if(buf->content == NULL) return 0;

    memcpy(&(buf->content[buf->size]), contents, real_size);
    buf->size += real_size;

    return real_size;
}

static struct mapi_authserver_error *json_to_authserver_error(json_t *json)
{
    json_t *error_json = json_object_get(json, "error");
    if(error_json == NULL) { return NULL; }

    json_t *error_message_json = json_object_get(json, "errorMessage");
    if(error_message_json == NULL) { return NULL; }

    json_t *cause_json = json_object_get(json, "cause");
    bool cause_available = cause_json != NULL;

    const char *error = json_string_value(error_json);
    const char *error_message = json_string_value(error_message_json);

    const char *cause;
    if(cause_available)
    {
        cause = json_string_value(cause_json);
    }
    else
    {
        cause = NULL;
    }

    struct mapi_authserver_error *error_struct = malloc(sizeof(struct mapi_authserver_error));
    if(error_struct == NULL) { ninerr_set_err(ninerr_from_errno()); return NULL; }

    error_struct->error = malloc(strlen(error) + 1);
    if(error_struct->error == NULL)
    {
        free(error_struct);
        return NULL;
    }

    error_struct->error_message = malloc(strlen(error_message) + 1);
    if(error_struct->error_message == NULL)
    {
        free(error_struct->error);
        free(error_struct);
        return NULL;
    }

    if(cause != NULL)
    {
        error_struct->cause = malloc(strlen(cause) + 1);
        if(error_struct->cause == NULL)
        {
            free(error_struct->error);
            free(error_struct->error_message);
            free(error_struct);
            return NULL;
        }
    }


    strcpy(error_struct->error, error);
    strcpy(error_struct->error_message, error_message);
    if(cause != NULL)
    {
        strcpy(error_struct->cause, cause);
    }
    else
    {
        error_struct->cause = NULL;
    }

    return error_struct;
}

static void mapi_authserver_error_destroy(struct mapi_authserver_error *error)
{
    free(error->error);
    free(error->error_message);
    free(error->cause);
    free(error);
}

struct mapi_auth_response *json_to_auth_response(json_t *json)
{
    struct mapi_auth_response *res = malloc(sizeof(struct mapi_auth_response));
    if(res == NULL) { ninerr_set_err(ninerr_from_errno()); return NULL; }


    json_t *access_token_json = json_object_get(json, "accessToken");
    if(access_token_json == NULL) { free(res); ninerr_set_err(NULL); return NULL; }
    const char *access_token = json_string_value(access_token_json);
    if(access_token == NULL) { free(res); return NULL; }

    json_t *client_token_json = json_object_get(json, "clientToken");
    if(client_token_json == NULL) { free(res); ninerr_set_err(NULL); return NULL; }
    const char *client_token = json_string_value(client_token_json);
    if(client_token == NULL) { free(res); return NULL; }

    size_t access_token_len = strlen(access_token);
    res->access_token = malloc(access_token_len + 1);
    if(res->access_token == NULL) { ninerr_set_err(ninerr_from_errno()); free(res); return NULL; }
    memcpy(res->access_token, access_token, access_token_len + 1); // faster than strcpy in this case

    size_t client_token_len = strlen(client_token);
    res->client_token = malloc(client_token_len + 1);
    if(res->client_token == NULL) { ninerr_set_err(ninerr_from_errno()); free(res->access_token); free(res); return NULL; }
    memcpy(res->client_token, client_token, client_token_len + 1); // faster than strcpy in this case

    json_t *available_profiles_json = json_object_get(json, "availableProfiles");
    if(available_profiles_json == NULL)
    {
        ninerr_set_err(ninerr_new("JSON error: availableProfiles is not present or not an array.", false));
        free(res->client_token);
        free(res->access_token);
        free(res);
        return NULL;
    }
    res->available_profiles_amount = json_array_size(available_profiles_json);
    if(res->available_profiles_amount == 0)
    {
        ninerr_set_err(ninerr_new("JSON error: availableProfiles is not present or not an array.", false));
        free(res->client_token);
        free(res->access_token);
        free(res);
        return NULL;
    }

    res->available_profiles = malloc(res->available_profiles_amount * sizeof(struct mapi_profile));
    if(res->available_profiles == NULL)
    {
        ninerr_set_err(ninerr_from_errno());
        free(res->client_token);
        free(res->access_token);
        free(res);
        return NULL;
    }

    for(unsigned int i = 0; i < res->available_profiles_amount; i++)
    {
        json_t *available_profile_json = json_array_get(available_profiles_json, i);
        if(available_profile_json == NULL)
        {
            ninerr_set_err(ninerr_from_errno());
            free(res->client_token);
            free(res->access_token);
            for(int i2 = i-1; i2 >= 0; i2--)
            {
                free(res->available_profiles[i2].name);
            }
            free(res->available_profiles);
            free(res);
            return NULL;
        }

        json_t *profile_id_json = json_object_get(available_profile_json, "id");
        if(profile_id_json == NULL)
        {
            ninerr_set_err(NULL);
            free(res->client_token);
            free(res->access_token);
            for(int i2 = i-1; i2 >= 0; i2--)
            {
                free(res->available_profiles[i2].name);
            }
            free(res->available_profiles);
            free(res);
            return NULL;
        }
        const char *profile_id = json_string_value(profile_id_json);
        if(profile_id == NULL)
        {
            ninerr_set_err(NULL);
            free(res->client_token);
            free(res->access_token);
            for(int i2 = i-1; i2 >= 0; i2--)
            {
                free(res->available_profiles[i2].name);
            }
            free(res->available_profiles);
            free(res);
            return NULL;
        }
        size_t profile_id_len = strlen(profile_id);
        if(!ninuuid_from_string(&(res->available_profiles[i].uuid), profile_id))
        {
            ninerr_set_err(NULL);
            free(res->client_token);
            free(res->access_token);
            for(int i2 = i-1; i2 >= 0; i2--)
            {
                free(res->available_profiles[i2].name);
            }
            free(res->available_profiles);
            free(res);
            return NULL;
        }


        json_t *profile_player_name_json = json_object_get(available_profile_json, "name");
        if(profile_player_name_json == NULL)
        {
            ninerr_set_err(NULL);
            free(res->client_token);
            free(res->access_token);
            for(int i2 = i-1; i2 >= 0; i2--)
            {
                free(res->available_profiles[i2].name);
            }
            free(res->available_profiles);
            free(res);
            return NULL;
        }
        const char *profile_player_name = json_string_value(profile_player_name_json);
        if(profile_player_name == NULL)
        {
            ninerr_set_err(NULL);
            free(res->client_token);
            free(res->access_token);
            for(int i2 = i; i2 >= 0; i2--)
            {
                free(res->available_profiles[i2].name);
            }
            free(res->available_profiles);
            free(res);
            return NULL;
        }
        size_t profile_player_name_len = strlen(profile_player_name);
        res->available_profiles[i].name = malloc(profile_player_name_len + 1);
        if(res->available_profiles[i].name == NULL)
        {
            ninerr_set_err(NULL);
            free(res->client_token);
            free(res->access_token);
            for(int i2 = i; i2 >= 0; i2--)
            {
                free(res->available_profiles[i2].name);
            }
            free(res->available_profiles);
            free(res);
            return NULL;
        }
        memcpy(res->available_profiles[i].name, profile_player_name, profile_player_name_len + 1); // faster than strcpy in this case.

        json_t *profile_legacy_json = json_object_get(available_profile_json, "legacy");
        if(profile_legacy_json == NULL)
        {
            ninerr_set_err(NULL);
            free(res->client_token);
            free(res->access_token);
            free(res->available_profiles[i].name);
            for(int i2 = i; i2 >= 0; i2--)
            {
                free(res->available_profiles[i2].name);
            }

            free(res->available_profiles);
            free(res);
            return NULL;
        }
        res->available_profiles[i].legacy = json_is_true(profile_legacy_json);
    }


    json_t *selected_profile_json = json_object_get(json, "selectedProfile");
    if(selected_profile_json == NULL)
    {
        ninerr_set_err(NULL);
        free(res->client_token);
        free(res->access_token);
        for(unsigned int i = 0; i < res->available_profiles_amount; i++)
        {
            free(res->available_profiles[i].name);
        }
        free(res->available_profiles);
        free(res);
        return NULL;
    }

    json_t *selected_profile_id_json = json_object_get(selected_profile_json, "id");
    if(selected_profile_id_json == NULL)
    {
        ninerr_set_err(NULL);
        free(res->client_token);
        free(res->access_token);
        for(unsigned int i = 0; i < res->available_profiles_amount; i++)
        {
            free(res->available_profiles[i].name);
        }
        free(res->available_profiles);
        free(res);
        return NULL;
    }
    const char *selected_profile_id = json_string_value(selected_profile_id_json);
    if(selected_profile_id == NULL)
    {
        ninerr_set_err(NULL);
        free(res->client_token);
        free(res->access_token);
        for(unsigned int i = 0; i < res->available_profiles_amount; i++)
        {
            free(res->available_profiles[i].name);
        }
        free(res->available_profiles);
        free(res);
        return NULL;
    }
    if(!ninuuid_from_string(&(res->selected_profile.uuid), selected_profile_id))
    {
        ninerr_set_err(NULL);
        free(res->client_token);
        free(res->access_token);
        for(unsigned int i = 0; i < res->available_profiles_amount; i++)
        {
            free(res->available_profiles[i].name);
        }
        free(res->available_profiles);
        free(res);
        return NULL;
    }

    json_t *selected_profile_name_json = json_object_get(selected_profile_json, "name");
    if(selected_profile_name_json == NULL)
    {
        ninerr_set_err(NULL);
        free(res->client_token);
        free(res->access_token);
        for(unsigned int i = 0; i < res->available_profiles_amount; i++)
        {
            free(res->available_profiles[i].name);
        }
        free(res->available_profiles);
        free(res);
        return NULL;
    }
    const char *selected_profile_name = json_string_value(selected_profile_name_json);
    if(selected_profile_name == NULL)
    {
        ninerr_set_err(NULL);
        free(res->client_token);
        free(res->access_token);
        for(unsigned int i = 0; i < res->available_profiles_amount; i++)
        {
            free(res->available_profiles[i].name);
        }
        free(res->available_profiles);
        free(res);
        return NULL;
    }
    size_t selected_profile_name_len = strlen(selected_profile_name);
    res->selected_profile.name = malloc(selected_profile_name_len + 1);
    if(res->selected_profile.name == NULL)
    {
        ninerr_set_err(NULL);
        free(res->client_token);
        free(res->access_token);
        for(unsigned int i = 0; i < res->available_profiles_amount; i++)
        {
            free(res->available_profiles[i].name);
        }
        free(res->available_profiles);
        free(res);
        return NULL;
    }
    memcpy(res->selected_profile.name, selected_profile_name, selected_profile_name_len + 1); // faster than strcpy in this case


    json_t *user_json = json_object_get(json, "user");
    if(user_json == NULL) { res->user_present = false; return res; }

    json_t *user_id_json = json_object_get(user_json, "id");
    if(user_id_json == NULL)
    {
        ninerr_set_err(NULL);
        free(res->client_token);
        free(res->access_token);
        for(unsigned int i = 0; i < res->available_profiles_amount; i++)
        {
            free(res->available_profiles[i].name);
        }
        free(res->available_profiles);
        free(res->selected_profile.name);
        free(res);
        return NULL;
    }
    const char *user_id = json_string_value(user_id_json);
    if(user_id == NULL)
    {
        ninerr_set_err(NULL);
        free(res->client_token);
        free(res->access_token);
        for(unsigned int i = 0; i < res->available_profiles_amount; i++)
        {
            free(res->available_profiles[i].name);
        }
        free(res->available_profiles);
        free(res->selected_profile.name);
        free(res);
        return NULL;
    }
    size_t user_id_len = strlen(user_id);
    res->user.id = malloc(user_id_len + 1);
    if(res->user.id == NULL)
    {
        ninerr_set_err(NULL);
        free(res->client_token);
        free(res->access_token);
        for(unsigned int i = 0; i < res->available_profiles_amount; i++)
        {
            free(res->available_profiles[i].name);
        }
        free(res->available_profiles);
        free(res->selected_profile.name);
        free(res);
        return NULL;
    }
    memcpy(res->user.id, user_id, user_id_len); // faster than strcpy

    json_t *properties_json = json_object_get(json, "properties");
    if(properties_json == NULL)
    {
        ninerr_set_err(NULL);
        free(res->client_token);
        free(res->access_token);
        for(unsigned int i = 0; i < res->available_profiles_amount; i++)
        {
            free(res->available_profiles[i].name);
        }
        free(res->available_profiles);
        free(res->selected_profile.name);
        free(res);
        return NULL;
    }
    size_t properties_json_size = json_array_size(properties_json);
    if(properties_json_size == 0)
    {
        ninerr_set_err(NULL);
        free(res->client_token);
        free(res->access_token);
        for(unsigned int i = 0; i < res->available_profiles_amount; i++)
        {
            free(res->available_profiles[i].name);
        }
        free(res->available_profiles);
        free(res->selected_profile.name);
        free(res);
        return NULL;
    }

    for(size_t i = 0; i < properties_json_size; i++)
    {
        json_t *entry = json_array_get(properties_json, i);
        if(entry == NULL)
        {
            ninerr_set_err(NULL);
            free(res->client_token);
            free(res->access_token);
            for(unsigned int i = 0; i < res->available_profiles_amount; i++)
            {
                free(res->available_profiles[i].name);
            }
            free(res->available_profiles);
            free(res->selected_profile.name);
            free(res);
            return NULL;
        }

        json_t *name_json = json_object_get(entry, "name");
        if(name_json == NULL)
        {
            ninerr_set_err(NULL);
            free(res->client_token);
            free(res->access_token);
            for(unsigned int i = 0; i < res->available_profiles_amount; i++)
            {
                free(res->available_profiles[i].name);
            }
            free(res->available_profiles);
            free(res->selected_profile.name);
            free(res);
            return NULL;
        }
        const char *name = json_string_value(name_json);
        if(name == NULL)
        {
            if(name_json == NULL)
            {
                ninerr_set_err(NULL);
                free(res->client_token);
                free(res->access_token);
                for(unsigned int i = 0; i < res->available_profiles_amount; i++)
                {
                    free(res->available_profiles[i].name);
                }
                free(res->available_profiles);
                free(res->selected_profile.name);
                free(res);
                return NULL;
            }
        }
        bool preferred_language_found = false;
        bool twitch_access_token_found = false;

        json_t *value = json_object_get(entry, "value");
        if(value == NULL)
        {
            ninerr_set_err(NULL);
            free(res->client_token);
            free(res->access_token);
            for(unsigned int i = 0; i < res->available_profiles_amount; i++)
            {
                free(res->available_profiles[i].name);
            }
            free(res->available_profiles);
            free(res->selected_profile.name);
            free(res);
            return NULL;
        }
        const char *string_value = json_string_value(value);
        if(string_value == NULL)
        {
            ninerr_set_err(NULL);
            free(res->client_token);
            free(res->access_token);
            for(unsigned int i = 0; i < res->available_profiles_amount; i++)
            {
                free(res->available_profiles[i].name);
            }
            free(res->available_profiles);
            free(res->selected_profile.name);
            free(res);
            return NULL;
        }
        size_t string_value_len = strlen(string_value);

        if(strcmp(name, "preferredLanguage") == 0)
        {
            if(preferred_language_found) continue;
            preferred_language_found = true;

            res->user.preferred_language = malloc(string_value_len + 1);
            if(res->user.preferred_language == NULL)
            {
                ninerr_set_err(NULL);
                free(res->client_token);
                free(res->access_token);
                for(unsigned int i = 0; i < res->available_profiles_amount; i++)
                {
                    free(res->available_profiles[i].name);
                }
                free(res->available_profiles);
                free(res->selected_profile.name);
                if(twitch_access_token_found) free(res->user.twitch_access_token);
                free(res);
                return NULL;
            }
            memcpy(res->user.preferred_language, string_value, string_value_len + 1);
        }
        else if(strcmp(name, "twitch_access_token") == 0)
        {
            if(twitch_access_token_found) continue;
            twitch_access_token_found = true;

            res->user.twitch_access_token = malloc(string_value_len + 1);
            if(res->user.twitch_access_token == NULL)
            {
                ninerr_set_err(NULL);
                free(res->client_token);
                free(res->access_token);
                for(size_t i = 0; i < res->available_profiles_amount; i++)
                {
                    free(res->available_profiles[i].name);
                }
                free(res->available_profiles);
                free(res->selected_profile.name);
                if(preferred_language_found) free(res->user.preferred_language);
                free(res);
                return NULL;
            }
            memcpy(res->user.twitch_access_token, string_value, string_value_len + 1);
        }
        else
        {
            fprintf(stdout, "Warning: unknown entry (%s) found in properties array. In mapi.c:%i", name, __LINE__);
        }
    }
    return res;
}

struct mapi_refresh_response *json_to_refresh_response(json_t *json)
{
    fprintf(stdout, "json_to_refresh_response is not implemented yet!");
    abort();
    return NULL;
}

static void mapi_err_authserver_err_free(struct ninerr *err)
{
    free(err);
}


static struct mapi_err_authserver_err *mapi_err_authserver_err_from_json(json_t *json, int http_code)
{
    json_t *error_json = json_object_get(json, "error");
    if(error_json == NULL) return NULL;
    const char *error = json_string_value(error_json);
    if(error == NULL) return NULL;

    json_t *error_message_json = json_object_get(json, "errorMessage");
    if(error_message_json == NULL) return NULL;
    const char *error_message = json_string_value(error_message_json);
    if(error_message == NULL) return NULL;

    const char *cause;
    json_t *cause_json = json_object_get(json, "errorMessage");
    if(cause_json != NULL)
    {
        cause = json_string_value(cause_json);
    }
    else
    {
        cause = NULL;
    }
    return mapi_err_authserver_err_new(error, error_message, cause, http_code);
}

static struct mapi_minecraft_has_joined_response *mapi_minecraft_has_joined_response_from_json(json_t *json)
{
    json_t *id_json = json_object_get(json, "id");
    if(id_json == NULL) { ninerr_set_err(NULL); return NULL; }

    const char *id = json_string_value(id_json);
    if(id == NULL) { ninerr_set_err(NULL); return NULL; }
    if(strlen(id) < NINUUID_STRING_SIZE_COMPRESSED) { ninerr_set_err(NULL); return NULL; }

    json_t *name_json = json_object_get(json, "name");
    if(name_json == NULL) { ninerr_set_err(NULL); return NULL; }

    const char *name = json_string_value(name_json);
    if(name == NULL) { ninerr_set_err(NULL); return NULL; }

    json_t *properties_json = json_object_get(json, "properties");
    if(properties_json == NULL) { ninerr_set_err(NULL); return NULL; }

    size_t property_entries = json_array_size(properties_json);
    if(property_entries == 0) { ninerr_set_err(NULL); return NULL; }

    bool textures_found = false;

    const char *value;
    const char *signature;
    for(size_t i = 0; i < property_entries; i++)
    {
        json_t *entry = json_array_get(properties_json, i);
        if(entry == NULL) { fprintf(stdout, "Aborting at %s:%i", __FILENAME__, __LINE__); abort(); } // Shouldn't happen.

        json_t *entry_name_json = json_object_get(entry, "name");
        if(entry_name_json == NULL) { ninerr_set_err(NULL); return NULL; }

        const char *entry_name = json_string_value(entry_name_json);
        if(entry_name == NULL) { ninerr_set_err(NULL); return NULL; }

        if(strcmp(entry_name, "textures") == 0)
        {
            json_t *value_json = json_object_get(entry, "value");
            if(value_json == NULL) { ninerr_set_err(NULL); return NULL; }

            value = json_string_value(value_json);
            if(value == NULL) { ninerr_set_err(NULL); return NULL; }

            json_t *signature_json = json_object_get(entry, "signature");
            if(signature_json == NULL) { ninerr_set_err(NULL); return NULL; }

            signature = json_string_value(signature_json);
            if(signature == NULL) { ninerr_set_err(NULL); return NULL; }

            textures_found = true;
            break;
        }
    }
    if(!textures_found) { ninerr_set_err(NULL); return NULL; }

    size_t value_len = strlen(value);
    size_t signature_len = strlen(signature);
    size_t name_len = strlen(name);
    struct mapi_minecraft_has_joined_response *resp = malloc(sizeof(struct mapi_minecraft_has_joined_response) + value_len+1 + signature_len+1 + name_len+1);
    if(resp == NULL) { ninerr_set_err(ninerr_from_errno()); return NULL; }

    char *value_buf = (char *) resp + sizeof(struct mapi_minecraft_has_joined_response);
    char *signature_buf = (char *) value_buf + value_len;
    char *name_buf = (char *) signature_buf + signature_len;

    memcpy(value_buf, value, value_len + 1);
    memcpy(signature_buf, signature, signature_len + 1);
    memcpy(name_buf, name, name_len + 1);

    resp->player_name = name_buf;
    resp->properties.signature = signature_buf;
    resp->properties.skin_blob_base64 = value_buf;

    DEBUG_PRINT("id: %s", id);
    if(!ninuuid_from_string(&(resp->id), id)) { ninerr_set_err(ninerr_new("ninuuid_from_string() failed")); return NULL; }

    return resp;
}

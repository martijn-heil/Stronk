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

#include <curl/curl.h>
#include <jansson/jansson.h>
#include <c11threads.h>

#include "mapi.h"



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

static size_t mapi_curl_write_callback(void *contents, size_t size, size_t nmemb, void *arg);

// Returns NULL on error or a malloc'ed mcpr_auth_response, which should be free'd
struct mapi_auth_response *json_to_auth_response(json_t *json);
struct mapi_refresh_response *json_to_refresh_response(json_t *json);
static int make_authserver_request(json_t **response, const char *endpoint, const char *payload);
static int mapi_make_api_request(json_t **output, const char *url, enum mapi_http_method http_method, char **headers, size_t header_count, const char *payload);

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


static thread_local unsigned int mapi_errno_value = 0;

unsigned int *mapi_get_errno() {
    return &mapi_errno_value;
}




int mapi_auth_validate(const char *restrict access_token, const char *restrict client_token) {
    char *fmt;
    int required;
    if(client_token != NULL) {
        fmt = "{\"accessToken\":\"%s\",\"clientToken\":\"%s\"}";
        char tmpbuf;
        required = snprintf(&tmpbuf, 1, fmt, access_token, client_token);
        if(required < 0) { return -1; }
    } else {
        fmt = "{\"accessToken\":\"%s\"}";
        char tmpbuf;
        required = snprintf(&tmpbuf, 1, fmt, access_token);
        if(required < 0) { return -1; }
    }
    char payload[required + 1];
    if(client_token != NULL) {
        int result = sprintf(&payload[0], fmt, access_token, client_token);
        if(result < 0) { return -1; }
    } else {
        int result = sprintf(&payload[0], fmt, access_token);
        if(result < 0) { return -1; }
    }

    json_t *json_response = NULL;
    int request_result = make_authserver_request(&json_response, "validate", payload);
    if(request_result < 0) {
        if(mapi_errno == 8) {
            return 0;
        } else {
            return -1;
        }
    }
    json_decref(json_response);
    return 1; // Valid! :D

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
        if(required < 0) { mapi_errno = 0; return NULL; }
    } else {
        char tmpbuf;
        fmt = "{\"agent\":{\"name\":\"%s\",\"version\":%d},\"username\":\"%s\",\"password\":\"%s\",\"clientToken\":\"%s\",\"requestUser\":%s}";
        required = snprintf(&tmpbuf, 1, fmt, agent_name, 1, account_name, password, client_token, request_user?"true":"false");
        if(required < 0) { mapi_errno = 0; return NULL; }
    }

    char payload[required + 1];
    if(client_token != NULL) {
        int result = sprintf(&payload[0], fmt, agent_name, 1, account_name, password, client_token, request_user?"true":"false");
        if(result < 0) {
            mapi_errno = 0;
            return NULL;
        }
    } else {
        int result = sprintf(&payload[0], fmt, agent_name, 1, account_name, password, request_user?"true":"false");
        if(result < 0) {
            mapi_errno = 0;
            return NULL;
        }
    }



    json_t *response = NULL;
    int http_status = make_authserver_request(&response, "authenticate", &payload[0]);
    if(http_status < 0) {
        switch(mapi_errno) {
            case 0: mapi_errno = 0; break;
            case 1: mapi_errno = 3; break;
            case 2: mapi_errno = 4; break;
            case 3: mapi_errno = 5; break;
            case 4: mapi_errno = 6; break;
            case 5: mapi_errno = 7; break;
            case 6: mapi_errno = 8; break;
            case 7: mapi_errno = 9; break;

            default: mapi_errno = 0; break;
        }
        if(response != NULL) { json_decref(response); }
        return NULL;
    }

    struct mapi_auth_response *res = json_to_auth_response(response);
    json_decref(response);
    if(unlikely(res == NULL)) {
        json_decref(response);
        mapi_errno = 0;
        return NULL;
    }
    free(payload);
    return res;
}


void mapi_auth_response_destroy(struct mapi_auth_response *response) {
    free(response->access_token);
    free(response->client_token);

    for(int i = 0; i < response->available_profiles_amount; i++) {
        free(response->available_profiles + i);
    }

    free(response->selected_profile.id);
    free(response->selected_profile.name);

    if(response->user_present) {
        free(response->user.id);
        free(response->user.preferred_language);
        free(response->user.twitch_access_token);
    }

    free(response);
}

void mapi_refresh_response_destroy(struct mapi_refresh_response *response) {
    free(response->access_token);
    free(response->client_token);
    free(response->selected_profile.id);
    free(response->selected_profile.name);

    if(response->user_present) {
        free(response->user.id);
        free(response->user.preferred_language);
        free(response->user.twitch_access_token);
    }

    free(response);
}


int mapi_generate_client_token(char *restrict buf, size_t token_len) {
    int i = 0;
    while(token_len > 0) {
        char tmp;
        if(secure_random(&tmp, 1) < 0) { return -1; }

        if(isalnum(tmp)) {
            buf[i] = tmp;
            i++;
            token_len--;
        }
    }
    buf[i] = '\0';

    return 0;
}

int mapi_username_to_uuid(struct ninuuid *output, const char *restrict player_name) {
    const char *fmt = "https://api.mojang.com/users/profiles/minecraft/%s";
    char url[strlen(fmt) + strlen(player_name) + 1];
    sprintf(&url[0], fmt, player_name);
    json_t *response;
    int status = mapi_make_api_request(&response, url, MAPI_HTTP_GET, NULL, 0, NULL);
    if(status < 0) { mapi_errno = 0; return -1; }
    if(status == 204) { mapi_errno = 1; return -1; } // No player found.

    json_t *compressed_uuid_json = json_object_get(response, "id");
    if(compressed_uuid_json == NULL) { mapi_errno = 0; return -1; }
    const char *compressed_uuid = json_string_value(compressed_uuid_json);
    if(compressed_uuid == NULL) { mapi_errno = 0; return -1; } // Not a string.
    if(strlen(compressed_uuid) != 32) { mapi_errno = 0; return -1; } // UUID is malformed, it is not 32 characters long.

    ninuuid_from_compressed_string(output, compressed_uuid);
    return 0;
}
// Big whitespace to divide actual functions from private helper functions.


























/**
 * mapi_errno error codes:
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
static int make_authserver_request(json_t **response, const char *endpoint, const char *payload) {
    char *url_prefix = "https://authserver.mojang.com/";
    char url[strlen(url_prefix) + strlen(endpoint) + 1];
    strcpy(url, url_prefix);
    strcat(url, endpoint);

    struct mapi_curl_buffer curl_buf;
    curl_buf.content = malloc(1);
    if(curl_buf.content == NULL) {
        mapi_errno = 4;
        return -1;
    }


    curl_global_init(CURL_GLOBAL_ALL);
    CURL *curl = curl_easy_init();
    CURLcode res;
    if(unlikely(curl == NULL)) {
        free(curl_buf.content);
        mapi_errno = 0;
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
    if(res != CURLE_OK) {
        free(curl_buf.content);
        switch(res) {
            case CURLE_COULDNT_RESOLVE_HOST:    mapi_errno = 1; break;
            case CURLE_COULDNT_CONNECT:         mapi_errno = 2; break;
            case CURLE_OUT_OF_MEMORY:           mapi_errno = 3; break;
            case CURLE_OPERATION_TIMEDOUT:      mapi_errno = 5; break;
            case CURLE_SEND_ERROR:              mapi_errno = 6; break;
            case CURLE_RECV_ERROR:              mapi_errno = 7; break;

            default: mapi_errno = 0; break;
        }

        return -1;
    }

    long http_code = 0;
    curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code != 200 && http_code != 204) // Authentication server returned an error.
    {
        // TODO read error type from response

        mapi_errno = 8;
        free(curl_buf.content);
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return -1;
    }

    json_error_t json_error;
    *response = json_loads(curl_buf.content, 0, &json_error);
    if(response == NULL) {
        free(curl_buf.content);
        mapi_errno = 0;
        return -1;
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();
    free(curl_buf.content);
    return 0;
}

static int mapi_make_api_request(json_t **output, const char *url, enum mapi_http_method http_method, char **headers, size_t header_count, const char *payload) {
    curl_global_init(CURL_GLOBAL_ALL);
    CURL *curl = curl_easy_init();
    if(curl == NULL) { return -1; }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "MAPI/1.0");

    struct curl_slist *chunk = NULL;
    if(headers != NULL) {
        for(int i = 0; i < header_count; i++) {
            chunk = curl_slist_append(chunk, headers[i]);
        }
    }

    if(payload != NULL) {
        chunk = curl_slist_append(chunk, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
    }

    chunk = curl_slist_append(chunk, "Accept: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);


    struct mapi_curl_buffer curl_buf;
    curl_buf.content = malloc(1);
    if(curl_buf.content == NULL) {
        mapi_errno = 4;
        return -1;
    }
    if(http_method != MAPI_HTTP_HEAD) {
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, mapi_curl_write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curl_buf);
    }


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

    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        free(curl_buf.content);
        return -1;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);


    json_error_t json_error;
    *output = json_loads(curl_buf.content, 0, &json_error);
    if(*output == NULL) {
        free(curl_buf.content);
        return -1;
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();
    free(curl_buf.content);
    if(http_code > INT_MAX) { return -1; }
    return (int) http_code;
}

static size_t mapi_curl_write_callback(void *contents, size_t size, size_t nmemb, void *arg) {
    struct mapi_curl_buffer *buf = arg;
    size_t real_size = size * nmemb;
    buf->content = realloc(buf->content, buf->size + real_size + 1);
    if(buf->content == NULL) {
        return 0;
    }

    memcpy(&(buf->content[buf->size]), contents, real_size);
    buf->size += real_size;
    buf->content[buf->size] = '\0';

    return real_size;
}

static struct mapi_authserver_error *json_to_authserver_error(json_t *json) {
    json_t *error_json = json_object_get(json, "error");
    if(error_json == NULL) { return NULL; }

    json_t *error_message_json = json_object_get(json, "errorMessage");
    if(error_message_json == NULL) { return NULL; }

    json_t *cause_json = json_object_get(json, "cause");
    bool cause_available = cause_json != NULL;

    const char *error = json_string_value(error_json);
    const char *error_message = json_string_value(error_message_json);

    const char *cause;
    if(cause_available) {
        cause = json_string_value(cause_json);
    } else {
        cause = NULL;
    }

    struct mapi_authserver_error *error_struct = malloc(sizeof(struct mapi_authserver_error));
    if(error_struct == NULL) { return NULL; }

    error_struct->error = malloc(strlen(error) + 1);
    if(error_struct->error == NULL) {
        free(error_struct);
        return NULL;
    }

    error_struct->error_message = malloc(strlen(error_message) + 1);
    if(error_struct->error_message == NULL) {
        free(error_struct->error);
        free(error_struct);
    }

    if(cause != NULL) {
        error_struct->cause = malloc(strlen(cause) + 1);
        if(error_struct->cause == NULL) {
            free(error_struct->error);
            free(error_struct->error_message);
            free(error_struct);
        }
    }


    strcpy(error_struct->error, error);
    strcpy(error_struct->error_message, error_message);
    if(cause != NULL) {
        strcpy(error_struct->cause, cause);
    } else {
        error_struct->cause = NULL;
    }

    return error_struct;
}

static void mapi_authserver_error_destroy(struct mapi_authserver_error *error) {
    free(error->error);
    free(error->error_message);
    free(error->cause);
    free(error);
}

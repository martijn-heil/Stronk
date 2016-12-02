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

#include <unistd.h>

#include <curl/curl.h>
#include <jansson/jansson.h>

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
static int secure_random(void *buf, size_t len) {
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

static size_t mapi_curl_write_callback(void *contents, size_t size, size_t nmemb, void *arg);

// Returns NULL on error or a malloc'ed mcpr_auth_response, which should be free'd
struct mapi_auth_response *json_to_auth_response(json_t *json);
struct mapi_refresh_response *json_to_refresh_response(json_t *json);
struct mcpr_curl_buffer;
static int make_authserver_request(json_t **response, const char *endpoint, const char *payload);


extern thread_local unsigned int mapi_errno = 0;


struct mapi_refresh_response *mapi_auth_refresh(const char *access_token, const char *client_token, bool request_user) {

    // This is inefficient, but the safest way to determine how much space we need.
    // Safety above all right? :P
    char *fmt = "{\"accessToken\":\"%s\",\"clientToken\":\"%s\",\"requestUser\":%s}";
    char tmpbuf;
    int required = snprintf(&tmpbuf, 1, fmt, access_token, client_token, request_user?"true":"false");
    if(required < 0) { return NULL; }
    char payload[required + 1];
    int result = sprintf(&payload, fmt, access_token, client_token, request_user?"true":"false");
    if(result < 0) { return NULL; }

    json_t *json_response;
    int request_result = make_authserver_request(&json_response, "refresh", payload);
    if(request_result < 0) { return NULL; }

    struct mapi_refresh_response *res = json_to_refresh_response(json_response);
    if(res == NULL) { return NULL; }
    return res;
}

struct mapi_auth_response *mapi_auth_authenticate(enum mapi_agent agent, int version, const char *account_name, const char *password, const char *client_token, bool request_user) {
    char *agent_name;
    switch(agent) {
        case MAPI_AGENT_MINECRAFT:
            agent_name = "Minecraft";
        break;

        case MAPI_AGENT_SCROLLS:
            agent_name = "Scrolls";
        break;
    }

    // TODO client_token may be NULL.
    // This is inefficient, but the safest way to determine how much space we need.
    // Safety above all right? :P
    char *fmt = "{\"agent\":{\"name\":\"%s\",\"version\":%d},\"username\":\"%s\",\"password\":\"%s\",\"clientToken\":\"%s\",\"requestUser\":%s}";
    char tmpbuf;
    int required = snprintf(&tmpbuf, 1, fmt, agent_name, 1 account_name, password, client_token, request_user?"true":"false");
    if(required < 0) { mapi_errno = 0; return NULL; }


    char post_data[required + 1];
    int result = sprintf(&post_data, fmt, agent_name, 1, account_name, password, client_token, request_user?"true":"false");
    if(result < 0) {
        mapi_errno = 0;
        return NULL;
    }

    json_t *response;
    int http_status = make_authserver_request(&response, "authenticate", &post_data);
    if(unlikely(http_status < 0) {
        return NULL;
    }

    struct *mcpr_auth_response res = json_to_auth_response(response);
    if(unlikely(res == NULL)) {
        return NULL;
    }
    // TODO destroy json_t response object.
    free(post_data);
    return res;
}


void mapi_auth_response_destroy(struct mcpr_auth_response *response) {
    free(response->access_token);
    free(response->client_token);

    for(int i = 0; i < response->available_profiles_amount; i++) {
        free(response->available_profiles + i);
    }

    free(response->selected_profile.id);
    free(response->selected_profile.name);

    if(response->user_present) {
        free(response->user->id);
        free(response->user->preferred_language);
        free(response->twitch_access_token);
    }

    free(response);
}

void mapi_refresh_response_destroy(struct mapi_refresh_response *response) {
    free(response->access_token);
    free(response->client_token);
    free(resonse->selected_profile.id);
    free(response->selected_profile.name);

    if(response->user_present) {
        free(response->user.id);
        free(response->user.preferred_language);
        free(response->user.twitch_access_token);
    }

    free(response);
}

/*
 * buf should be at least the size of token_len
 * Returns <0 upon error.
 */
int mapi_generate_client_token(char *buf, size_t token_len) {
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
}
// Big whitespace to divide actual functions from private helper functions.


























/*
 * mapi_errno error codes:
 *
 * 0: Unknown error occured, this could be anything, including but not limited to the errors listed below.
 * 1: Remote host could not be resolved.
 * 2: Could not connect to remote host.
 * 3: Out of memory, errno is not guaranteed to be set.
 * 4: Malloc failure, errno will be set.
 * 5: Request timeout.
 * 6: Could not send data over network.
 * 7: Error receiving data from the authentication server.
 */
static int make_authserver_request(json_t **response, const char *endpoint, const char *payload) {
    char *url_prefix = "https://authserver.mojang.com/"
    char url[strlen(url_prefix) + strlen(endpoint) + 1];
    strcpy(url, url_prefix);
    strcat(url, endpoint);

    struct mcpr_curl_buffer curl_buf;
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
    res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "MCPR/1.0");
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
    if (http_code != 200) // Authentication server returned an error.
    {
        free(curl_buf.content);
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return -1;
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();

    json_error_t json_error;
    *response = json_loads(curl_buf.content, 0, &json_error);
    if(response == NULL) {
        free(curl_buf.content);
        mapi_errno = 0;
        return -1;
    }
    free(curl_buf.content);

    return http_code;
}

static size_t mapi_curl_write_callback(void *contents, size_t size, size_t nmemb, void *arg) {
    struct mcpr_curl_buffer *buf = arg;
    size_t realsize = size * nmemb;
    buf->content = realloc(buf->content, buf->size + realsize + 1);
    if(buf->content == NULL) {
        return 0;
    }

    memcpy(&(buf->content[buf->size]), contents, realsize);
    buf->size += realsize;
    buf->content[buf->size] = '\0';

    return realsize;
}

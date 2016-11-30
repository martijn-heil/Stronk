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
struct mcpr_curl_buffer;
static int make_authserver_request(json_t **response, const char *endpoint, const char *payload);




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

    // This is inefficient, but the safest way to determine how much space we need.
    // Safety above all right? :P
    char *fmt = "{\"agent\":{\"name\":\"%s\",\"version\":%d},\"username\":\"%s\",\"password\":\"%s\",\"clientToken\":\"%s\",\"requestUser\":%s}";
    char tmpbuf;
    int required = snprintf(&tmpbuf, 1, fmt, agent_name, account_name, password, client_token, request_user?"true":"false");
    if(required < 0) { return NULL; }


    char post_data[required + 1];
    int result = sprintf(post_data, fmt, agent_name, account_name, password, client_token, request_user?"true":"false");
    if(result < 0) {
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

    free(response)
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
int mapi_generate_client_token(void *buf, size_t token_len) {
    return (secure_random(buf, token_len) < 0) ? -1 : 0;
}
// Big whitespace to divide actual functions vs private helper functions.






































static int make_authserver_request(json_t **response, const char *endpoint, const char *payload) {
    char *url_prefix = "https://authserver.mojang.com/"
    char *url = malloc(strlen(url_prefix) + strlen(endpoint) + 1);
    if(unlikely(url == NULL)) {
        return -1;
    }
    strcpy(url, url_prefix);
    strcat(url, endpoint);


    CURL *curl = curl_easy_init();
    if(unlikely(curl == NULL)) {
        #ifdef MCPR_DO_LOGGING
            nlog_error("Could not initialize CURL.");
        #endif
        free(url);
        return -1;
    }

    struct mcpr_curl_buffer curl_buf;
    curl_buf.content = malloc(1);
    if(curl_buf.content == NULL) {
        free(url);
        return -1;
    }

    curl_global_init(CURL_GLOBAL_ALL);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "MCPR/1.0");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, mapi_curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curl_buf);
    curl_easy_cleanup(curl);
    curl_global_cleanup();


    // TODO error checking & read response in curl_buf

    json_error_t json_error;
    *response = json_loads(curl_buf.content, 0, &json_error);
    if(response == NULL) {
        free(url);
        free(curl_buf.content);
        return -1;
    }
    free(curl_buf.content);

    // TODO return HTTP status code
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

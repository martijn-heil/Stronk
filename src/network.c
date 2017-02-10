#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include <thpool.h>

#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

#include <algo/hash-table.h>
#include <algo/hash-string.h>
#include <algo/compare-string.h>

#include <mcpr/mcpr.h>
#include <mcpr/abstract_packet.h>
#include <mcpr/fdstreams.h>
#include <mcpr/codec.h>

#include "network.h"
#include "stronk.h"
#include "util.h"

static int server_socket;
static size_t client_count = 0;
static pthread_mutex_t *clients_delete_lock;
static SListEntry *clients = NULL; // List of clients.


static void accept_incoming_connections(void);
static int make_server_socket (uint16_t port);
static void serve_client_batch(void *arg);
static void serve_clients(void);
static struct player *conn_to_player(struct connection *conn); // TODO implement

static HashTable *tmp_encryption_states = NULL;
static struct tmp_encryption_state
{
    int32_t verify_token_length;
    void *verify_token;
    RSA *rsa;
};

static HashTable *tmp_username_states = NULL;

static HashTable *players = NULL;
static HashTable *conn_to_player = NULL;


int net_init(void) {
    int port = 25565; // TODO configuration of port.

    nlog_info("Creating server socket on port %i..", port);
    server_socket = make_socket(port);
    if(server_socket < 0)
    {
        nlog_fatal("Could not create server socket. (%s ?)", strerror(errno));
        return -1;
    }


    if(fcntl(server_socket, F_SETFL, O_NONBLOCK) == -1)
    {
        nlog_fatal("Could not set O_NONBLOCK flag for server socket. (%s)", strerror(errno));
        return -1;
    }

    // Ignore broken pipe signals, has to do with sockets.
    nlog_info("Making sure that broken pipe (SIGPIPE) signals are ignored..");
    struct sigaction new_actn, old_actn;
    new_actn.sa_handler = SIG_IGN;
    sigemptyset (&new_actn.sa_mask);
    new_actn.sa_flags = 0;
    sigaction (SIGPIPE, &new_actn, &old_actn);


    // Set up temporary state storage.
    nlog_info("Initializing temporary state storage.");
    tmp_encryption_states = hash_table_new(pointer_hash, pointer_equal);
    if(tmp_encryption_states == NULL)
    {
        nlog_fatal("Could not create hash table.");
        return -1;
    }

    tmp_username_states = hash_table_new(pointer_hash, pointer_equal);
    if(tmp_username_states == NULL)
    {
        nlog_fatal("Could not create hash table.");
        return -1;
    }

    if(pthread_mutex_init(clients_delete_lock, NULL) != 0)
    {
        nlog_fatal("Could not initialize mutex.");
        hash_table_free(players);
        return -1;
    }
}

void net_cleanup(void) {
    // TODO
}

void net_tick(void) {
    accept_incoming_connections();
    serve_clients();
}




static int make_server_socket (uint16_t port) {
    struct sockaddr_in name;

    // Create the socket.
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        nlog_error("Could not create socket. (%s)", strerror(errno));
        return -1;
    }

    // Give the socket a name.
    name.sin_family = AF_INET;
    name.sin_port = hton16(port);
    name.sin_addr.s_addr = hton32(INADDR_ANY);
    if (bind(sockfd, (struct sockaddr *) &name, sizeof (name)) < 0)
    {
        nlog_error("Could not bind socket to address. (%s)", strerror(errno));
        return -1;
    }

    return sockfd;
}

static void accept_incoming_connections(void)
{
    while(true)
    {
        int newfd = accept(server_socket, NULL, NULL)
        if(newfd == -1)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK) // There are no incoming connections in the queue.
            {
                break;
            }
            else if(errno == ECONNABORTED)
            {
                nlog_debug("An incoming connection was aborted.");
            }
            else
            {
                nlog_error("Could not accept incoming connection. (%s)", strerror(errno));
                continue;
            }
        }

        if(fcntl(newfd, F_SETFL, O_NONBLOCK) == -1)
        {
            nlog_fatal("Could not set O_NONBLOCK flag for incoming connection. (%s)", strerror(errno));
            if(close(newfd) == -1) nlog_error("Could not clean up socket after memory allocation failure. (%s)", strerror(errno));
            continue;
        }

        struct connection *conn = malloc(sizeof(struct connection));
        if(conn == NULL)
        {
            nlog_error("Could not allocate memory for connection. (%s)", strerror(errno));
            if(close(newfd) == -1) nlog_error("Could not clean up socket after memory allocation failure. (%s)", strerror(errno));
            continue;
        }

        conn->fd = newfd;
        conn->state = MCPR_STATE_HANDSHAKE;
        conn->use_compression = false;
        conn->use_encryption = false;
        conn->ctx_encrypt = NULL;
        conn->ctx_decrypt = NULL;

        if(slist_append(clients, conn) == NULL)
        {
            nlog_error("Could not add incoming connection to connection storage.");
            if(close(newfd) == -1) nlog_error("Could not clean up socket after error. (%s)", strerror(errno));
            free(conn);
            continue;
        }
        client_count++;
    }
}

static void serve_clients(void)
{
    unsigned int conns_per_thread = client_count / main_threadpool_threadcount;
    unsigned int rest = client_count % main_threadpool_threadcount;

    int index = 1;

    for(unsigned int i = 0; i < main_threadpool_threadcount, i++)
    {
        unsigned int amount = conns_per_thread;
        if(i == (main_threadpool_threadcount - 1)) amount += rest;

        SListEntry *arg1 = slist_nth_entry(clients, index);
        unsigned int arg2 = amount;
        void *args = malloc(sizeof(SListEntry *) + sizeof(unsigned int));
        memcpy(args, &arg1, sizeof(arg1));
        memcpy(args + sizeof(arg1), &arg2, sizeof(arg2));
        thpool_add_work(main_threadpool, serve_client_batch, args);
        index += amount;
    }

    thpool_wait(main_threadpool);
}

static void serve_client_batch(void *arg)
{
    SListEntry *first = *(SListEntry **) arg;
    unsigned int amount = *((unsigned int *) (arg + sizeof(SListEntry *)));


    for(int i = 0; i < amount; i++)
    {
        SListEntry *current_entry = slist_nth_entry(first, i);
        struct connection *conn = slist_data(current_entry);
        if(conn == NULL)
        {
            nlog_error("Bad slist index.");
            break;
        }

        struct player *player = conn_to_player(conn); // Will be NULL if there is no player associated with this connection.



        // Should only be called once.
        // Should not be used like a normal function call.
        #define CLOSE_CURRENT_CONN() \
            if(close(conn->fd) == -1) nlog_error("Error whilst closing socket. (%s)", mcpr_strerror(errno)); \
            pthread_mutex_lock(clients_delete_lock); \
            slist_remove_entry(&first, current_entry); \
            pthread_mutex_unlock(clients_delete_lock); \
            if(conn->ctx_encrypt != NULL) EVP_CIPHER_CTX_cleanup(conn->ctx_encrypt); free(conn->ctx_encrypt); \
            if(conn->ctx_decrypt != NULL) EVP_CIPHER_CTX_cleanup(conn->ctx_decrypt); free(conn->ctx_decrypt); \
            if(player != NULL) \
            { \
                free(player->locale);
                free(player->client_brand);
                free(player);
            } \
            else \
            { \
                free(conn); \
            }

        // Can be called multiple times in the scope.
        // Returns ssize_t less than 0 upon error.
        // Sets mcpr_errno upon error.
        // May be used like a function.
        #define WRITE_PACKET(pkt)
            connection_write_abstract_packet(conn, pkt)


        // Should only be called once.
        // Should not be used like a normal function call.
        #define DISCONNECT_CURRENT_CONN(_reason) \
            if(conn->state == MCPR_STATE_LOGIN || conn->state == MCPR_STATE_PLAY) \
            { \
                struct mcpr_abstract_packet _response; \
                _response.id = ((conn->state == MCPR_STATE_LOGIN) ? MCPR_PKT_LG_CB_DISCONNECT : MCPR_PKT_PL_CB_DISCONNECT); \
                _response.data.login.clientbound.disconnect.reason = _reason; \
                if(WRITE_PACKET(&_response) < 0) \
                { \
                    nlog_error("Could not write packet to connection (%s ?), " \
                                "unable to properly disconnect client.", mcpr_strerror(mcpr_errno)); \
                } \
            } \
            CLOSE_CURRENT_CONN();



        struct mcpr_abstract_packet *pkt = connection_read_abstract_packet(conn); // TODO read multiple packets in a single tick.

        if(pkt == NULL)
        {
            if(mcpr_errno == MCPR_ECOMPRESSTHRESHOLD)
            {
                char *reason = mcpr_as_chat("Compression treshold was violated.");
                DISCONNECT_CURRENT_CONN(reason);
                free(reason);
            }

            nlog_error("Could not read packet from client. (%s ?)", strerror(errno));
        }



        switch(conn->state)
        {
            case MCPR_STATE_HANDSHAKE:
            {
                switch(pkt->id)
                {
                    case MCPR_PKT_HS_SB_HANDSHAKE:
                    {
                        int32_t protocol_version = pkt->data.handshake.serverbound.handshake.protocol_version;
                        conn->state = pkt->data.handshake.serverbound.handshake.next_state;

                        if(protocol_version != MCPR_PROTOCOL_VERSION)
                        {
                            if(conn->state == MCPR_STATE_LOGIN)
                            {
                                char *reason = mcpr_as_chat("Protocol version " PRId32 " is not supported! I'm on protocol version %i (MC %s)", protocol_version, MCPR_PROTOCOL_VERSION, MCPR_MINECRAFT_VERSION);
                                DISCONNECT_CURRENT_CONN(reason);
                                free(reason);
                            }
                        }
                    }
                }
            }

            case MCPR_STATE_STATUS:
            {
                switch(pkt->id)
                {
                    case MCPR_PK_ST_SB_REQUEST:
                    {
                        struct mcpr_abstract_packet response;
                        response.id = MCPR_PKT_ST_SB_RESPONSE;
                        response.data.status.clientbound.response.version_name = MCPR_MINECRAFT_VERSION;
                        response.data.status.clientbound.response.protocol_version = MCPR_PROTOCOL_VERSION;
                        response.data.status.clientbound.response.max_players = max_players;
                        response.data.status.clientbound.response.online_players_size = hash_table_num_entries(players);
                        response.data.status.clientbound.response.description = motd;
                        response.data.status.clientbound.response.online_players = NULL;
                        response.data.status.clientbound.response.favicon = NULL;


                        if(WRITE_PACKET(&response) < 0)
                        {
                            if(mcpr_errno == ECONNRESET)
                            {
                                CLOSE_CURRENT_CONN();
                                break;
                            }
                            else
                            {
                                nlog_error("Could not write packet to connection (%s ?)", mcpr_strerror(errno));
                                break;
                            }
                        }
                    }

                    case MCPR_PKT_ST_SB_PING:
                    {
                        struct mcpr_abstract_packet response;
                        response.id = MCPR_PKT_ST_CB_PONG;
                        response.data.status.clientbound.pong.payload = pkt.data.status.serverbound.ping.payload;

                        if(WRITE_PACKET(&response) < 0)
                        {
                            if(mcpr_errno == ECONNRESET)
                            {
                                CLOSE_CURRENT_CONN();
                                break;
                            }
                            else
                            {
                                nlog_error("Could not write packet to connection (%s ?)", mcpr_strerror(errno));
                                break;
                            }
                        }
                    }
                }
            }

            case MCPR_STATE_LOGIN:
            {
                switch(pkt->id)
                {
                    case MCPR_PKT_LG_SB_LOGIN_START:
                    {
                        RSA *rsa = RSA_generate_key(1024, 3, 0, 0); // TODO this is deprecated in OpenSSL 1.0.2? But the alternative is not there in 1.0.1

                        unsigned char *buf = NULL; // TODO should this be freed afterwards?
                        int buflen = i2d_RSA_PUBKEY(rsa, &buf);
                        if(buflen < 0)
                        {
                            // Error occured.
                            nlog_error("Ermg ze openssl errorz!!"); // TODO proper error handling.
                            break;
                        }
                        if(buflen > INT32_MAX || buflen < INT32_MIX) // TODO is INT32_LEAST a thing?
                        {
                            nlog_error("Integer overflow.");
                            RSA_free(rsa);
                            break;
                        }

                        int32_t verify_token_length = 16;
                        uint8_t *verify_token = malloc(verify_token_length * sizeof(uint8_t)); // 128 bit verify token.
                        if(verify_token == NULL)
                        {
                            nlog_error("Could not allocate memory. (%s)", strerror(errno));
                            RSA_free(rsa);
                            break;
                        }

                        if(secure_random(verify_token, verify_token_length) < 0)
                        {
                            nlog_error("Could not get random data.");
                            RSA_free(rsa);
                            break;
                        }


                        struct mcpr_abstract_packet response;
                        response.id = MCPR_PKT_LG_CB_ENCRYPTION_REQUEST;
                        response.data.login.clientbound.encryption_request.server_id = ""; // Yes that's supposed to be an empty string.
                        response.data.login.clientbound.encryption_request.public_key_length = (int32_t) buflen;
                        response.data.login.clientbound.encryption_request.public_key = buf;
                        response.data.login.clientbound.encryption_request.verify_token_length = verify_token_length;
                        response.data.login.clientbound.encryption_request.verify_token = verify_token;


                        if(WRITE_PACKET(&response) < 0)
                        {
                            if(mcpr_errno == ECONNRESET)
                            {
                                CLOSE_CURRENT_CONN();
                                RSA_free(rsa);
                                break;
                            }
                            else
                            {
                                nlog_error("Could not write packet to connection (%s ?)", mcpr_strerror(errno));
                                RSA_free(rsa);
                                break;
                            }
                        }

                        struct tmp_encryption_state *tmp_state = malloc(sizeof(struct tmp_encryption_state))
                        if(tmp_state == NULL)
                        {
                            nlog_error("Could not allocate memory (%s)", strerror(errno));
                            RSA_free(rsa);
                            break;
                        }

                        tmp_state->rsa = rsa;
                        tmp_state->verify_token_length = verify_token_length;
                        tmp_state->verify_token = verify_token;


                        if(hash_table_insert(tmp_encryption_states, conn, tmp_state) == 0)
                        {
                            nlog_error("Could not insert entry into hash table. (%s ?)", strerror(errno));
                            RSA_free(rsa);
                            break;
                        }
                    }

                    case MCPR_PKT_LG_SB_ENCRYPTION_RESPONSE:
                    {
                        bool player_inserted = false;
                        bool player_inserted2 = false;
                        struct tmp_encryption_state *tmp_state = NULL;
                        void *decrypted_shared_secret = NULL;
                        char *username = NULL;

                        username = hash_table_lookup(tmp_username_states, conn);
                        if(username == HASH_TABLE_NULL)
                        {
                            nlog_error("Could not find entry in hash table.");
                            char *reason = mcpr_as_chat("An error occurred whilst setting up encryption.");
                            DISCONNECT_CURRENT_CONN(reason);
                            free(reason);
                        }

                        tmp_state = hash_table_lookup(tmp_encryption_states, conn);
                        if(tmp_state == HASH_TABLE_NULL) // Shouldn't happen.
                        {
                            nlog_error("Could not find entry in hash table.");
                            char *reason = mcpr_as_chat("An error occurred whilst setting up encryption.");
                            DISCONNECT_CURRENT_CONN(reason);
                            hash_table_remove(tmp_username_states, conn);
                            free(reason);
                            free(username);
                            break;
                        }
                        RSA *rsa = tmp_state->rsa;

                        int32_t shared_secret_length = pkt->data.login.serverbound.encryption_response.shared_secret_length;
                        void *shared_secret = pkt->data.login.serverbound.encryption_response.shared_secret;

                        if(shared_secret_length > INT_MAX)
                        {
                            nlog_error("Integer overflow.");
                            goto err;
                        }

                        decrypted_shared_secret = malloc(RSA_size(rsa));
                        if(decrypted_shared_secret == NULL)
                        {
                            nlog_error("Could not allocate memory. (%s)", strerror(errno));
                            goto err;
                        }

                        int size = RSA_private_decrypt((int) shared_secret_length, (unsigned char *) shared_secret, (unsigned char *) decrypted_shared_secret, rsa, RSA_PKCS1_PADDING);
                        if(size < 0)
                        {
                            nlog_error("Could not decrypt shared secret."); // TODO proper error handling.
                            goto err;
                        }

                        conn->ctx_encrypt = malloc(sizeof(conn->ctx_encrypt));
                        if(conn->ctx_encrypt == NULL)
                        {
                            nlog_error("Could not allocate memory. (%s)", strerror(errno));
                            goto err;
                        }
                        EVP_CIPHER_CTX_init(conn->ctx_encrypt);
                        if(EVP_EncryptInit_ex(conn->ctx_encrypt, EVP_aes_128_cfb8(),
                         NULL, (unsigned char *) decrypted_shared_secret, (unsigned char *) decrypted_shared_secret) == 0) // TODO Should those 2 pointers be seperate buffers of shared secret?
                         {
                            nlog_error("Error upon EVP_EncryptInit_ex()."); // TODO proper error handling.
                            goto err;
                        }

                        conn->ctx_decrypt = malloc(sizeof(conn->ctx_decrypt));
                        if(conn->ctx_decrypt == NULL)
                        {
                            nlog_error("Could not allocate memory. (%s)", strerror(errno));
                            goto err;
                        }
                        EVP_CIPHER_CTX_init(conn->ctx_decrypt);
                        if(EVP_DecryptInit_ex(conn->ctx_decrypt), EVP_aes_128_cfb8(),
                         NULL, (unsigned char *) decrypted_shared_secret, (unsigned char *) decrypted_shared_secret) == 0) // TODO Should those 2 pointers be seperate buffers of shared secret?
                         {
                            nlog_error("Error upon EVP_DecryptInit_ex()."); // TODO proper error handling.
                            goto err;
                        }

                        unsigned char *encoded_public_key = NULL; // TODO should this be freed afterwards?
                        int encoded_public_key_len = i2d_RSA_PUBKEY(rsa, &encoded_public_key);
                        if(encoded_public_key_len < 0)
                        {
                            // Error occured.
                            nlog_error("Ermg ze openssl errorz!!"); // TODO proper error handling.
                            break;
                        }

                        uint8_t server_id_hash[SHA_DIGEST_LENGTH];

                        SHA_CTX sha_ctx;
                        if(unlikely(SHA1_Init(&sha_ctx) == 0))
                        {
                            nlog_error("Could not initialize SHA-1 hash.");
                            goto err;
                        }
                        if(SHA1_Update(&sha_ctx, decrypted_shared_secret, shared_secret_len) == 0)
                        {
                            nlog_error("Could not update SHA-1 hash.");
                            uint8_t ignored_tmpbuf[SHA_DIGEST_LENGTH];
                            SHA1_Final(ignored_tmpbuf, &sha_ctx); // clean up
                            goto err;
                        }
                        if(SHA1_Update(&sha_ctx, encoded_public_key, encoded_public_key_len) == 0)
                        {
                            nlog_error("Could not update SHA-1 hash.");
                            uint8_t ignored_tmpbuf[SHA_DIGEST_LENGTH];
                            SHA1_Final(ignored_tmpbuf, &sha_ctx); // clean up
                            goto err;
                        }
                        if(SHA1_Final(server_id_hash, &sha_ctx) == 0)
                        {
                            nlog_error("Could not finalize SHA-1 hash.");
                            goto err;
                        }

                        char stringified_server_id_hash[SHA_DIGEST_LENGTH * 2 + 2];
                        mcpr_crypto_stringify_sha1(stringified_server_id_hash, server_id_hash);

                        struct mapi_minecraft_has_joined_response *mapi_result = mapi_minecraft_has_joined(username, stringified_server_id_hash);
                        if(mapi_result == NULL)
                        {
                            // TODO handle legit non error case where a failure happened.
                            goto err;
                        }


                        struct mcpr_abstract_packet response;
                        response.id = MCPR_PKT_LG_CB_LOGIN_SUCCESS;
                        response.data.login.clientbound.login_success.uuid = mapi_result->id;
                        response.data.login.clientbound.login_success.username = username;

                        if(WRITE_PACKET(&response) < 0)
                        {
                            if(mcpr_errno == ECONNRESET)
                            {
                                CLOSE_CURRENT_CONN();
                                break;
                            }
                            else
                            {
                                nlog_error("Could not send login success packet to connection. (%s ?)", mcpr_strerror(mcpr_errno));
                                char *reason = mcpr_as_chat("An error occurred.");
                                DISCONNECT_CURRENT_CONN(reason);
                                free(reason);
                            }
                            goto err;
                        }

                        conn->state = MCPR_STATE_PLAY;

                        hash_table_remove(tmp_encryption_states, conn);
                        hash_table_remove(tmp_username_states, conn);
                        RSA_free(rsa);
                        free(tmp_state->verify_token);
                        free(tmp_state);
                        free(decrypted_shared_secret);
                        free(username);

                        player = malloc(sizeof(player));
                        if(player == NULL)
                        {
                            nlog_error("Could not allocate memory. (%s)", strerror(errno));
                            goto err;
                        }
                        player->uuid = mapi_result->id;
                        player->conn = *conn;
                        player->brand = NULL;
                        player->invulnerable = false;
                        player->is_flying = false;
                        player->allow_flying = false;
                        player->gamemode = MCPR_GAMEMODE_SURVIVAL;
                        player->client_settings_known = false;

                        player->compass_target.x = 0;
                        player->compass_target.y = 0;
                        player->compass_target.z = 0;


                        if(pthread_mutex_lock(clients_delete_lock) != 0)
                        {
                            nlog_error("Could not lock client deletion lock. (%s)", strerror(errno));
                            goto err;
                        }
                        slist_remove_entry(&first, current_entry);
                        pthread_mutex_unlock(clients_delete_lock);
                        free(conn);

                        if(pthread_mutex_lock(entity_id_counter_lock) != 0)
                        {
                            nlog_error("Could not lock global entity ID counter lock. (%s)", strerror(errno));
                            goto err;
                        }
                        player->entity_id = entity_id_counter;
                        entity_id_counter++;
                        pthread_mutex_unlock(entity_id_counter_lock);

                        if(hash_table_insert(players, &(player->uuid), player) == 0)
                        {
                            nlog_error("Could not add player to players hash table. (%s ?)", strerror(errno));
                            goto err;
                        }
                        player_inserted = true;

                        if(hash_table_insert(conn_to_player, conn, player) == 0)
                        {
                            nlog_error("Could not add player to hash table. (%s ?)", strerror(errno));
                            goto err;
                        }
                        player_inserted2 = true;

                        struct mcpr_abstract_packet join_game_pkt;
                        join_game_pkt.id = MCPR_PKT_PL_CB_JOIN_GAME;
                        join_game_pkt.data.play.clientbound.join_game.entity_id = player->entity_id;
                        join_game_pkt.data.play.clientbound.join_game.gamemode = player->gamemode;
                        join_game_pkt.data.play.clientbound.join_game.hardcore = false;
                        join_game_pkt.data.play.clientbound.join_game.dimension = MCPR_DIMENSION_OVERWORLD;
                        join_game_pkt.data.play.clientbound.join_game.difficulty = MCPR_DIFFICULTY_PEACEFUL;
                        join_game_pkt.data.play.clientbound.join_game.max_players = 999;
                        join_game_pkt.data.play.clientbound.join_game.level_type = MCPR_LEVEL_DEFAULT;
                        join_game_pkt.data.play.clientbound.join_game.reduced_debug_info = false;

                        if(WRITE_PACKET(&join_game_pkt) < 0)
                        {
                            nlog_error("Could not send join game packet. (%s)", mcpr_strerror(mcpr_errno));
                            goto err;
                        }


                        uint8_t server_brand_buf[MCPR_VARINT_SIZE_MAX + 6];
                        ssize_t encode_str_result = mcpr_encode_string(server_brand_buf, "Stronk");
                        if(encode_str_result < 0)
                        {
                            nlog_error("Could not encode brand string.");
                            goto err;
                        }


                        struct mcpr_abstract_packet pm_brand;
                        pm_brand.id = MCPR_PKT_PL_CB_PLUGIN_MESSAGE;
                        pm_brand.data.play.clientbound.plugin_message.channel = "MC|BRAND";
                        pm_brand.data.play.clientbound.plugin_message.data_length = (size_t) encode_str_result;
                        pm_brand.data.play.clientbound.plugin_message.data = server_brand_buf;

                        if(WRITE_PACKET(&pm_brand) < 0)
                        {
                            if(mcpr_errno == ECONNRESET)
                            {
                                CLOSE_CURRENT_CONN();
                                goto cleanup_only;
                            }
                            else
                            {
                                nlog_error("Could not write MC|BRAND plugin message. (%s)", mcpr_strerror(mcpr_errno));
                                goto err;
                            }
                        }

                        struct mcpr_abstract_packet spawn_position_pkt;
                        spawn_position_pkt.id = MCPR_PKT_PL_CB_SPAWN_POSITION;
                        spawn_position_pkt.location = player->compass_target;

                        if(WRITE_PACKET(&spawn_position_pkt) < 0)
                        {
                            if(mcpr_errno == ECONNRESET)
                            {
                                CLOSE_CURRENT_CONN();
                                goto cleanup_only;
                            }
                            else
                            {
                                nlog_error("Could not write spawn position packet. (%s)", mcpr_strerror(mcpr_errno));
                                goto err;
                            }
                        }

                        struct mcpr_abstract_packet player_abilities_pkt;
                        player_abilities_pkt.id = MCPR_PKT_PL_CB_PLAYER_ABILITIES;
                        player_abilities_pkt.data.play.clientbound.player_abilities.invulnerable = player->invulnerable;
                        player_abilities_pkt.data.play.clientbound.player_abilities.can_fly = player->allow_flying;
                        player_abilities_pkt.data.play.clientbound.player_abilities.is_flying = player->is_flying;
                        player_abilities_pkt.data.play.clientbound.player_abilities.flying_speed = player->flying_speed;
                        player_abilities_pkt.data.play.clientbound.player_abilities.field_of_view_modifier = 1f;

                        if(WRITE_PACKET(&player_abilities_pkt) < 0)
                        {
                            nlog_error("Could not write player abilities packet. (%s)", mcpr_strerror(mcpr_errno));
                            goto err;
                        }

                        break;
                        err:
                            char *reason = mcpr_as_chat("An error occurred whilst logging in.");
                            DISCONNECT_CURRENT_CONN(reason);
                            free(reason);
                        cleanup_only:
                            if(player_inserted) hash_table_remove(players, &(player->uuid));
                            if(player_inserted2) hash_table_remove(conn_to_player, &(player->uuid))
                            hash_table_remove(tmp_encryption_states, conn);
                            hash_table_remove(tmp_username_states, conn);
                            RSA_free(rsa);
                            free(tmp_state->verify_token);
                            free(tmp_state);
                            free(decrypted_shared_secret);
                            free(username);
                            free(player);
                            break;
                    }
                }
            }

            case MCPR_STATE_PLAY:
            {
                switch(pkt->id)
                {
                    case MCPR_PKT_PL_SB_PLUGIN_MESSAGE:
                    {
                        if(strcmp(pkt->data.play.serverbound.plugin_message.channel, "MC|BRAND") == 0)
                        {
                            int32_t data_length = pkt->data.play.serverbound.plugin_message.data_length;
                            if(data_length < 0) break;
                            if(data_length > SIZE_MAX) break;
                            void *data = pkt->data.play.serverbound.plugin_message.data;

                            char *client_brand;
                            ssize_t result = mcpr_decode_string(&client_brand, data, (size_t) data_length);
                            if(result < 0) break;

                            player->client_brand = client_brand;
                        }
                    }

                    case MCPR_PKT_PL_SB_CLIENT_SETTINGS:
                    {
                        // We don't know how the locale in the packet is allocated, so we need to copy the string to ensure
                        // that it is allocated via malloc()
                        char *locale = pkt->data.play.serverbound.client_settings.locale;
                        player->client_settings->locale = malloc(strlen(locale) + 1);
                        if(player->client_settings->locale == NULL)
                        {
                            nlog_error("Could not allocate memory. (%s)", strerror(errno));
                            break;
                        }
                        strcpy(player->client_settings->locale, locale, strlen(locale));

                        player->client_settings->view_distance = pkt->data.play.serverbound.client_settings.view_distance;
                        player->client_settings->chat_mode = pkt->data.play.serverbound.client_settings.chat_mode;
                        player->client_settings->chat_colors = pkt->data.play.serverbound.client_settings.chat_colors;
                        player->client_settings->chat_mode = pkt->data.play.serverbound.client_settings.chat_mode;

                        player->client_settings->displayed_skin_parts->cape_enabled = pkt->data.play.serverbound.client_settings.displayed_skin_parts.cape_enabled;
                        player->client_settings->displayed_skin_parts->jacket_enabled = pkt->data.play.serverbound.client_settings.displayed_skin_parts.jacket_enabled;
                        player->client_settings->displayed_skin_parts->left_sleeve_enabled = pkt->data.play.serverbound.client_settings.displayed_skin_parts.left_sleeve_enabled;
                        player->client_settings->displayed_skin_parts->right_sleeve_enabled = pkt->data.play.serverbound.client_settings.displayed_skin_parts.right_sleeve_enabled;
                        player->client_settings->displayed_skin_parts->left_pants_enabled = pkt->data.play.serverbound.client_settings.displayed_skin_parts.left_pants_enabled;
                        player->client_settings->displayed_skin_parts->right_pants_enabled = pkt->data.play.serverbound.client_settings.displayed_skin_parts.right_pants_enabled;
                        player->client_settings->displayed_skin_parts->hat_enabled = pkt->data.play.serverbound.client_settings.displayed_skin_parts.hat_enabled;

                        player->client_settings_known = true;

                        struct mcpr_abstract_packet response;
                        response.id = MCPR_PKT_PL_CB_PLAYER_POSITION_AND_LOOK;
                        response.data.play.clientbound.player_position_and_look.x = player->pos.x;
                        response.data.play.clientbound.player_position_and_look.feet_y = player->pos.y;
                        response.data.play.clientbound.player_position_and_look.feet_z = player->pos.z;
                        response.data.play.clientbound.player_position_and_look.pitch = player->pos.pitch;
                        response.data.play.clientbound.player_position_and_look.yaw = player->pos.yaw;

                        response.data.play.clientbound.player_position_and_look.x_is_relative = false;
                        response.data.play.clientbound.player_position_and_look.y_is_relative = false;
                        response.data.play.clientbound.player_position_and_look.z_is_relative = false;
                        response.data.play.clientbound.player_position_and_look.pitch_is_relative = false;
                        response.data.play.clientbound.player_position_and_look.yaw_is_relative = false;

                        response.data.play.clientbound.player_position_and_look.teleport_id = 0; // TODO

                        if(WRITE_PACKET(&response) < 0)
                        {
                            if(mcpr_errno == ECONNRESET)
                            {
                                break;
                            }
                            else
                            {
                                nlog_error("Could not send player position and look packet for login sequence. (%s)", mcpr_strerror(mcpr_errno));

                                char *reason = mcpr_as_chat("An error occurred whilst logging in.");
                                DISCONNECT_CURRENT_CONN(reason);
                                free(reason);
                            }
                        }
                    }
                }
            }
        }

        mcpr_free_abstract_packet(pkt);
    }

    free(arg);

    #undef CLOSE_CURRENT_CONN
    #undef DISCONNECT_CURRENT_CONN
    #undef WRITE_PACKET
}

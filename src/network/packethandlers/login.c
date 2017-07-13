/*
    MIT License

    Copyright (c) 2017 Martijn Heil

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
*/

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <pthread.h>

#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

#include <algo/hash-table.h>
#include <algo/hash-pointer.h>
#include <algo/compare-pointer.h>

#include <ninerr/ninerr.h>

#include <mcpr/mcpr.h>
#include <mcpr/packet.h>
#include <mcpr/crypto.h>
#include <mcpr/codec.h>

#include <mapi/mapi.h>

#include <logging/logging.h>
#include <network/packethandlers/packethandlers.h>
#include <world/entity.h>
#include <util.h>

static HashTable *tmp_username_states = NULL;
static HashTable *tmp_encryption_states = NULL;
bool tmp_encryption_states_init = false;
bool tmp_username_states_init = false;
struct tmp_encryption_state
{
    int32_t verify_token_length;
    void *verify_token;
    RSA *rsa;
};

static bool ensure_init(void)
{
    // Set up temporary state storage.
    if(!tmp_encryption_states_init)
    {
        nlog_info("Initializing temporary encryption state storage.");
        tmp_encryption_states = hash_table_new(pointer_hash, pointer_equal);
        if(tmp_encryption_states == NULL)
        {
            nlog_error("Could not create hash table.");
            return false;
        }
    }

    if(!tmp_username_states_init)
    {
        tmp_username_states = hash_table_new(pointer_hash, pointer_equal);
        if(tmp_username_states == NULL)
        {
            nlog_error("Could not create hash table.");
            return false;
        }
    }

    return true;
}

struct hp_result handle_lg_login_start(const struct mcpr_packet *pkt, struct connection *conn)
{
        if(!ensure_init())
        {
            nlog_error("Could not handle login start packet, initialization failed.");
            char *reason = mcpr_as_chat("A fatal error occurred whilst logging in.");
            struct hp_result result;
            result.result = HP_RESULT_FATAL;
            result.disconnect_message = reason;
            result.free_disconnect_message = true;
            return result;
        }

        RSA *rsa = RSA_generate_key(1024, 3, 0, 0); // TODO this is deprecated in OpenSSL 1.0.2? But the alternative is not there in 1.0.1

        unsigned char *buf = NULL; // TODO should this be freed afterwards?
        int buflen = i2d_RSA_PUBKEY(rsa, &buf);
        if(buflen < 0)
        {
            // Error occured.
            nlog_error("Ermg ze openssl errorz!!"); // TODO proper error handling.

            char *reason = mcpr_as_chat("A fatal error occurred whilst logging in.");
            struct hp_result result;
            result.result = HP_RESULT_FATAL;
            result.disconnect_message = reason;
            result.free_disconnect_message = true;
            return result;
        }
        if(buflen > INT32_MAX || buflen < INT32_MIN)
        {
            nlog_error("Integer overflow.");
            RSA_free(rsa);

            char *reason = mcpr_as_chat("A fatal error occurred whilst logging in.");
            struct hp_result result;
            result.result = HP_RESULT_FATAL;
            result.disconnect_message = reason;
            result.free_disconnect_message = true;
            return result;
        }

        int32_t verify_token_length = 16;
        uint8_t *verify_token = malloc(verify_token_length * sizeof(uint8_t)); // 128 bit verify token.
        if(verify_token == NULL)
        {
            nlog_error("Could not allocate memory. (%s)", strerror(errno));
            RSA_free(rsa);

            char *reason = mcpr_as_chat("A fatal error occurred whilst logging in.");
            struct hp_result result;
            result.result = HP_RESULT_FATAL;
            result.disconnect_message = reason;
            result.free_disconnect_message = true;
            return result;
        }

        if(secure_random(verify_token, verify_token_length) < 0)
        {
            nlog_error("Could not get random data.");
            RSA_free(rsa);

            char *reason = mcpr_as_chat("A fatal error occurred whilst logging in.");
            struct hp_result result;
            result.result = HP_RESULT_FATAL;
            result.disconnect_message = reason;
            result.free_disconnect_message = true;
            return result;
        }


        struct mcpr_packet response;
        response.id = MCPR_PKT_LG_CB_ENCRYPTION_REQUEST;
        response.data.login.clientbound.encryption_request.server_id = ""; // Yes, that's supposed to be an empty string.
        response.data.login.clientbound.encryption_request.public_key_length = (int32_t) buflen;
        response.data.login.clientbound.encryption_request.public_key = buf;
        response.data.login.clientbound.encryption_request.verify_token_length = verify_token_length;
        response.data.login.clientbound.encryption_request.verify_token = verify_token;


        if(mcpr_connection_write_packet(conn->conn, &response) < 0)
        {
            if(ninerr != NULL && ninerr->message != NULL && strcmp(ninerr->message, "ninerr_closed") == 0)
            {
                RSA_free(rsa);

                struct hp_result result;
                result.result = HP_RESULT_CLOSED;
                result.disconnect_message = NULL;
                result.free_disconnect_message = false;
                return result;
            }
            else
            {
                if(ninerr != NULL && ninerr->message != NULL)
                {
                    nlog_error("Could not write packet to connection. (%s ?)", ninerr->message);
                }
                else
                {
                    nlog_error("Could not write packet to connection");
                }

                RSA_free(rsa);

                char *reason = mcpr_as_chat("A fatal error occurred whilst logging in.");
                struct hp_result result;
                result.result = HP_RESULT_FATAL;
                result.disconnect_message = reason;
                result.free_disconnect_message = true;
                return result;
            }
        }

        struct tmp_encryption_state *tmp_state = malloc(sizeof(struct tmp_encryption_state));
        if(tmp_state == NULL)
        {
            nlog_error("Could not allocate memory. (%s)", strerror(errno));
            RSA_free(rsa);

            char *reason = mcpr_as_chat("A fatal error occurred whilst logging in.");
            struct hp_result result;
            result.result = HP_RESULT_FATAL;
            result.disconnect_message = reason;
            result.free_disconnect_message = true;
            return result;
        }

        tmp_state->rsa = rsa;
        tmp_state->verify_token_length = verify_token_length;
        tmp_state->verify_token = verify_token;


        if(hash_table_insert(tmp_encryption_states, conn, tmp_state) == 0)
        {
            nlog_error("Could not insert entry into hash table. (%s ?)", strerror(errno));
            RSA_free(rsa);

            char *reason = mcpr_as_chat("A fatal error occurred whilst logging in.");
            struct hp_result result;
            result.result = HP_RESULT_FATAL;
            result.disconnect_message = reason;
            result.free_disconnect_message = true;
            return result;
        }

        struct hp_result result;
        result.result = HP_RESULT_OK;
        result.disconnect_message = NULL;
        result.free_disconnect_message = false;
        return result;
    }

    struct hp_result handle_lg_encryption_response(const struct mcpr_packet *pkt, struct connection *conn)
    {
        if(!ensure_init())
        {
            nlog_error("Could not handle login start packet, initialization failed.");
            char *reason = mcpr_as_chat("A fatal error occurred whilst logging in.");
            struct hp_result result;
            result.result = HP_RESULT_FATAL;
            result.disconnect_message = reason;
            result.free_disconnect_message = true;
            return result;
        }

        struct tmp_encryption_state *tmp_state = NULL;
        void *decrypted_shared_secret = NULL;
        char *username = NULL;
        uint8_t server_id_hash[SHA_DIGEST_LENGTH];
        char stringified_server_id_hash[SHA_DIGEST_LENGTH * 2 + 2];
        uint8_t server_brand_buf[MCPR_VARINT_SIZE_MAX + 6];

        username = hash_table_lookup(tmp_username_states, conn);
        if(username == HASH_TABLE_NULL)
        {
            nlog_error("Could not find entry in hash table.");
            goto err;
        }

        tmp_state = hash_table_lookup(tmp_encryption_states, conn);
        if(tmp_state == HASH_TABLE_NULL) // Shouldn't happen.
        {
            nlog_error("Could not find entry in hash table.");
            hash_table_remove(tmp_username_states, conn);
            free(username);
            goto err;
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

        EVP_CIPHER_CTX *ctx_encrypt = EVP_CIPHER_CTX_new();
        if(ctx_encrypt == NULL)
        {
            nlog_error("Could not create ctx_encrypt.");
            goto err;
        }
        EVP_CIPHER_CTX_init(ctx_encrypt);
        if(EVP_EncryptInit_ex(ctx_encrypt, EVP_aes_128_cfb8(), NULL, (unsigned char *) decrypted_shared_secret, (unsigned char *) decrypted_shared_secret) == 0) // TODO Should those 2 pointers be seperate buffers of shared secret?
         {
            nlog_error("Error upon EVP_EncryptInit_ex()."); // TODO proper error handling.
            goto err;
        }

        EVP_CIPHER_CTX *ctx_decrypt = EVP_CIPHER_CTX_new();
        if(ctx_decrypt == NULL)
        {
            nlog_error("Could not create ctx_decrypt.");
            goto err;
        }
        EVP_CIPHER_CTX_init(ctx_decrypt);
        if(EVP_DecryptInit_ex(ctx_decrypt, EVP_aes_128_cfb8(), NULL, (unsigned char *) decrypted_shared_secret, (unsigned char *) decrypted_shared_secret) == 0) // TODO Should those 2 pointers be seperate buffers of shared secret?
         {
            nlog_error("Error upon EVP_DecryptInit_ex()."); // TODO proper error handling.
            goto err;
        }
        mcpr_connection_set_crypto(conn->conn, ctx_encrypt, ctx_decrypt);

        unsigned char *encoded_public_key = NULL; // TODO should this be freed afterwards?
        int encoded_public_key_len = i2d_RSA_PUBKEY(rsa, &encoded_public_key);
        if(encoded_public_key_len < 0)
        {
            // Error occured.
            nlog_error("Ermg ze openssl errorz!!"); // TODO proper error handling.
            goto err;
        }



        SHA_CTX sha_ctx;
        if(unlikely(SHA1_Init(&sha_ctx) == 0))
        {
            nlog_error("Could not initialize SHA-1 hash.");
            goto err;
        }
        if(SHA1_Update(&sha_ctx, decrypted_shared_secret, shared_secret_length) == 0)
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


        mcpr_crypto_stringify_sha1(stringified_server_id_hash, server_id_hash);

        struct mapi_minecraft_has_joined_response *mapi_result = mapi_minecraft_has_joined(username, stringified_server_id_hash, conn->server_address_used);
        if(mapi_result == NULL)
        {
            // TODO handle legit non error case where a failure happened.
            goto err;
        }


        struct mcpr_packet response;
        response.id = MCPR_PKT_LG_CB_LOGIN_SUCCESS;
        response.data.login.clientbound.login_success.uuid = mapi_result->id;
        response.data.login.clientbound.login_success.username = username;

        if(mcpr_connection_write_packet(conn->conn, &response) < 0)
        {
            if(strcmp(ninerr->type, "ninerr_closed") == 0)
            {
                goto closed;
            }
            else
            {
                if(ninerr != NULL && ninerr->message != NULL)
                {
                    nlog_error("Could not send login success packet to connection. (%s ?)", ninerr->message);
                }
                else
                {
                    nlog_error("Could not send login success packet to connection.");
                }
                goto err;
            }
        }

        mcpr_connection_set_state(conn->conn, MCPR_STATE_PLAY);

        hash_table_remove(tmp_encryption_states, conn);
        hash_table_remove(tmp_username_states, conn);
        RSA_free(rsa);
        free(tmp_state->verify_token);
        free(tmp_state);
        free(decrypted_shared_secret);
        free(username);

        struct player *player = malloc(sizeof(struct player));
        if(player == NULL)
        {
            nlog_error("Could not allocate memory. (%s)", strerror(errno));
            goto err;
        }
        player->uuid = mapi_result->id;
        player->conn = conn->conn;
        player->client_brand = NULL;
        player->invulnerable = false;
        player->is_flying = false;
        player->allow_flying = false;
        player->gamemode = MCPR_GAMEMODE_SURVIVAL;
        player->client_settings_known = false;

        player->compass_target.x = 0;
        player->compass_target.y = 0;
        player->compass_target.z = 0;

        player->entity_id = generate_new_entity_id();

        struct mcpr_packet join_game_pkt;
        join_game_pkt.id = MCPR_PKT_PL_CB_JOIN_GAME;
        join_game_pkt.state = MCPR_STATE_PLAY;
        join_game_pkt.data.play.clientbound.join_game.entity_id = player->entity_id;
        join_game_pkt.data.play.clientbound.join_game.gamemode = player->gamemode;
        join_game_pkt.data.play.clientbound.join_game.hardcore = false;
        join_game_pkt.data.play.clientbound.join_game.dimension = MCPR_DIMENSION_OVERWORLD;
        join_game_pkt.data.play.clientbound.join_game.difficulty = MCPR_DIFFICULTY_PEACEFUL;
        join_game_pkt.data.play.clientbound.join_game.max_players = 255;
        join_game_pkt.data.play.clientbound.join_game.level_type = MCPR_LEVEL_DEFAULT;
        join_game_pkt.data.play.clientbound.join_game.reduced_debug_info = false;

        if(mcpr_connection_write_packet(conn->conn, &join_game_pkt) < 0)
        {
            if(ninerr != NULL && strcmp(ninerr->type, "ninerr_closed") == 0)
            {
                goto closed;
            }
            else
            {
                if(ninerr != NULL && ninerr->message != NULL)
                {
                    nlog_error("Could not send join game packet. (%s)", ninerr->message);
                }
                else
                {
                    nlog_error("Could not send join game packet.");
                }
                goto err;
            }
        }



        ssize_t encode_str_result = mcpr_encode_string(server_brand_buf, "Stronk");
        if(encode_str_result < 0)
        {
            nlog_error("Could not encode brand string.");
            goto err;
        }


        struct mcpr_packet pm_brand;
        pm_brand.id = MCPR_PKT_PL_CB_PLUGIN_MESSAGE;
        pm_brand.data.play.clientbound.plugin_message.channel = "MC|BRAND";
        pm_brand.data.play.clientbound.plugin_message.data_length = (size_t) encode_str_result;
        pm_brand.data.play.clientbound.plugin_message.data = server_brand_buf;

        if(mcpr_connection_write_packet(conn->conn, &pm_brand) < 0)
        {
            if(ninerr != NULL && strcmp(ninerr->type, "ninerr_closed") == 0)
            {
                goto closed;
            }
            else
            {
                if(ninerr != NULL && ninerr->message != NULL)
                {
                    nlog_error("Could not write MC|BRAND plugin message. (%s)", ninerr->message);
                }
                else
                {
                    nlog_error("Could not write MC|BRAND plugin message.");
                }
                goto err;
            }
        }

        struct mcpr_packet spawn_position_pkt;
        spawn_position_pkt.id = MCPR_PKT_PL_CB_SPAWN_POSITION;
        spawn_position_pkt.state = MCPR_STATE_PLAY;
        spawn_position_pkt.data.play.clientbound.spawn_position.location = player->compass_target;

        if(mcpr_connection_write_packet(conn->conn, &spawn_position_pkt) < 0)
        {
            if(ninerr != NULL && strcmp(ninerr->type, "ninerr_closed") == 0)
            {
                goto closed;
            }
            else
            {
                if(ninerr != NULL && ninerr->message != NULL)
                {
                    nlog_error("Could not write spawn position packet. (%s)", ninerr->message);
                }
                else
                {
                    nlog_error("Could not write spawn position packet.");
                }
                goto err;
            }
        }

        struct mcpr_packet player_abilities_pkt;
        player_abilities_pkt.id = MCPR_PKT_PL_CB_PLAYER_ABILITIES;
        player_abilities_pkt.state = MCPR_STATE_PLAY;
        player_abilities_pkt.data.play.clientbound.player_abilities.invulnerable = player->invulnerable;
        player_abilities_pkt.data.play.clientbound.player_abilities.allow_flying = player->allow_flying;
        player_abilities_pkt.data.play.clientbound.player_abilities.is_flying = player->is_flying;
        player_abilities_pkt.data.play.clientbound.player_abilities.flying_speed = player->flying_speed;
        player_abilities_pkt.data.play.clientbound.player_abilities.field_of_view_modifier = 1.0;

        if(mcpr_connection_write_packet(conn->conn, &player_abilities_pkt) < 0)
        {
            if(ninerr != NULL && strcmp(ninerr->type, "ninerr_closed") == 0)
            {
                goto closed;
            }
            else
            {
                if(ninerr != NULL && ninerr->message != NULL)
                {
                    nlog_error("Could not write player abilities packet. (%s)", ninerr->message);
                }
                else
                {
                    nlog_error("Could not write player abilities packet.");
                }

                goto err;
            }
        }

    goto cleanup_only;
    err:
        hash_table_remove(tmp_encryption_states, conn);
        hash_table_remove(tmp_username_states, conn);
        RSA_free(rsa);
        free(tmp_state->verify_token);
        free(tmp_state);
        free(decrypted_shared_secret);
        free(username);
        free(player);
        char *reason = mcpr_as_chat("A fatal error occurred whilst logging in.");
        struct hp_result result1;
        result1.result = HP_RESULT_FATAL;
        result1.disconnect_message = reason;
        result1.free_disconnect_message = true;
        return result1;

    cleanup_only:
        hash_table_remove(tmp_encryption_states, conn);
        hash_table_remove(tmp_username_states, conn);
        RSA_free(rsa);
        free(tmp_state->verify_token);
        free(tmp_state);
        free(decrypted_shared_secret);
        free(username);
        free(player);
        struct hp_result result2;
        result2.result = HP_RESULT_OK;
        result2.disconnect_message = NULL;
        result2.free_disconnect_message = false;
        return result2;

    closed:
        hash_table_remove(tmp_encryption_states, conn);
        hash_table_remove(tmp_username_states, conn);
        RSA_free(rsa);
        free(tmp_state->verify_token);
        free(tmp_state);
        free(decrypted_shared_secret);
        free(username);
        free(player);
        struct hp_result result3;
        result3.result = HP_RESULT_CLOSED;
        result3.disconnect_message = NULL;
        result3.free_disconnect_message = false;
        return result3;
}

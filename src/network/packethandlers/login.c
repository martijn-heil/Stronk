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
#include <inttypes.h>

#include <pthread.h>

#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/err.h>

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
#include "../../util.h"
#include "../../server.h"

static bool is_auth_required(const char *username)
{
    return false;
}

static struct player *create_player(struct connection *conn, struct ninuuid uuid)
{
    struct player *player = malloc(sizeof(struct player));
    if(player == NULL)
    {
        nlog_error("Could not allocate memory. (%s)", strerror(errno));
        return NULL;
    }
    player->uuid = uuid;
    player->username = conn->tmp.username;
    player->conn = conn;
    player->client_brand = NULL;
    player->invulnerable = true;
    player->is_flying = true;
    player->allow_flying = true;
    player->flying_speed = 1.0;
    player->gamemode = MCPR_GAMEMODE_SURVIVAL;
    player->client_settings_known = false;
    player->compass_target.x = 0;
    player->compass_target.y = 70;
    player->compass_target.z = 0;
    player->pos.x = 0;
    player->pos.y = 128;
    player->pos.z = 0;
    player->pos.yaw = 0;
    player->pos.pitch = 0;

    player->entity_id = generate_new_entity_id();
    server_get_internal_clock_time(&(player->last_keepalive_sent));
    server_get_internal_clock_time(&(player->last_keepalive_received));
    conn->player = player;
    return player;
}


static struct hp_result send_post_login_sequence(struct connection *conn)
{
    struct player *player = conn->player;

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
            struct hp_result result;
            result.result = HP_RESULT_CLOSED;
            result.disconnect_message = NULL;
            result.free_disconnect_message = true;
            return result;
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
            struct hp_result result;
            result.result = HP_RESULT_FATAL;
            result.disconnect_message = NULL;
            result.free_disconnect_message = true;
            return result;
        }
    }


    uint8_t server_brand_buf[MCPR_VARINT_SIZE_MAX + 6];
    ssize_t encode_str_result = mcpr_encode_string(server_brand_buf, "Stronk");
    if(encode_str_result < 0)
    {
        nlog_error("Could not encode brand string.");
        struct hp_result result;
        result.result = HP_RESULT_FATAL;
        result.disconnect_message = NULL;
        result.free_disconnect_message = true;
        return result;
    }


    struct mcpr_packet pm_brand;
    pm_brand.id = MCPR_PKT_PL_CB_PLUGIN_MESSAGE;
    pm_brand.state = MCPR_STATE_PLAY;
    pm_brand.data.play.clientbound.plugin_message.channel = "MC|BRAND";
    pm_brand.data.play.clientbound.plugin_message.data_length = (size_t) encode_str_result;
    pm_brand.data.play.clientbound.plugin_message.data = server_brand_buf;

    if(mcpr_connection_write_packet(conn->conn, &pm_brand) < 0)
    {
        if(ninerr != NULL && strcmp(ninerr->type, "ninerr_closed") == 0)
        {
            struct hp_result result;
            result.result = HP_RESULT_CLOSED;
            result.disconnect_message = NULL;
            result.free_disconnect_message = true;
            return result;
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
            struct hp_result result;
            result.result = HP_RESULT_FATAL;
            result.disconnect_message = NULL;
            result.free_disconnect_message = true;
            return result;
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
            struct hp_result result;
            result.result = HP_RESULT_CLOSED;
            result.disconnect_message = NULL;
            result.free_disconnect_message = true;
            return result;
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
            struct hp_result result;
            result.result = HP_RESULT_FATAL;
            result.disconnect_message = NULL;
            result.free_disconnect_message = true;
            return result;
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
    player_abilities_pkt.data.play.clientbound.player_abilities.creative_mode = false;

    if(mcpr_connection_write_packet(conn->conn, &player_abilities_pkt) < 0)
    {
        if(ninerr != NULL && strcmp(ninerr->type, "ninerr_closed") == 0)
        {
            struct hp_result result;
            result.result = HP_RESULT_CLOSED;
            result.disconnect_message = NULL;
            result.free_disconnect_message = true;
            return result;
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

            struct hp_result result;
            result.result = HP_RESULT_FATAL;
            result.disconnect_message = NULL;
            result.free_disconnect_message = true;
            return result;
        }
    }

    struct hp_result result;
    result.result = HP_RESULT_OK;
    result.disconnect_message = NULL;
    result.free_disconnect_message = true;
    return result;
}


struct hp_result handle_lg_login_start(const struct mcpr_packet *pkt, struct connection *conn)
{
    nlog_debug("in handle_lg_login_start");
    conn->tmp.username = pkt->data.login.serverbound.login_start.name;

    if(is_auth_required(pkt->data.login.serverbound.login_start.name))
    {
        nlog_info("Connection at %p is required to do authentication.", (void *) conn);
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
        response.state = MCPR_STATE_LOGIN;
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

        size_t username_len = strlen(pkt->data.login.serverbound.login_start.name);
        char *username = malloc(username_len + 1);
        if(username == NULL)
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
        memcpy(username, pkt->data.login.serverbound.login_start.name, username_len + 1);

        conn->tmp_present = true;
        conn->tmp.rsa = rsa;
        conn->tmp.verify_token_length = verify_token_length;
        conn->tmp.verify_token = verify_token;
        conn->tmp.username = username;

        struct hp_result result;
        result.result = HP_RESULT_OK;
        result.disconnect_message = NULL;
        result.free_disconnect_message = false;
        return result;
    }
    else
    {
        nlog_info("Connection at %p is not required to do authentication.", (void *) conn);
        struct ninuuid uuid;
        if(!ninuuid_generate(&uuid, 4)) // TODO should not actually be version 4 but version 3
        {
            nlog_error("Could not generate new UUID for player.");
            struct hp_result result;
            result.result = HP_RESULT_FATAL;
            result.disconnect_message = NULL;
            result.free_disconnect_message = false;
            return result;
        }

        struct mcpr_packet response;
        response.id = MCPR_PKT_LG_CB_LOGIN_SUCCESS;
        response.state = MCPR_STATE_LOGIN;
        response.data.login.clientbound.login_success.uuid = uuid;
        response.data.login.clientbound.login_success.username = conn->tmp.username;

        if(mcpr_connection_write_packet(conn->conn, &response) < 0)
        {
            if(strcmp(ninerr->type, "ninerr_closed") == 0)
            {
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
                    nlog_error("Could not send login success packet to connection. (%s ?)", ninerr->message);
                }
                else
                {
                    nlog_error("Could not send login success packet to connection.");
                }
                struct hp_result result;
                result.result = HP_RESULT_FATAL;
                result.disconnect_message = NULL;
                result.free_disconnect_message = false;
                return result;
            }
        }
        // 42
        // uint8_t buf[] = {
        //     /* packet len: */                    0x2D,
        //     /* packet id: (1 byte) */            0x02,
        //     /* uuid: (36 bytes plus 1 byte prefix length) */      0x24,        0x35, 0x30, 0x64, 0x38, 0x66, 0x63, 0x66, 0x30, 0x2d, 0x31, 0x36, 0x36, 0x65, 0x2d, 0x34, 0x61, 0x62, 0x33, 0x2d, 0x39, 0x31, 0x37, 0x36, 0x2d, 0x63, 0x34, 0x31, 0x66, 0x62, 0x35, 0x37, 0x35, 0x30, 0x37, 0x31, 0x61,
        //     /* username: (6 bytes plus 1 byte prefix length) */             0x06, 0x4e, 0x69, 0x6e, 0x6a, 0x6f, 0x68
        // };
        // mcpr_connection_write(conn->conn, buf, sizeof(buf));

        mcpr_connection_set_state(conn->conn, MCPR_STATE_PLAY);

        struct player *player = create_player(conn, uuid);
        if(player == NULL)
        {
            struct hp_result result;
            result.result = HP_RESULT_FATAL;
            result.disconnect_message = NULL;
            result.free_disconnect_message = false;
            return result;
        }

        nlog_debug("State for connection at %p (username: %s) switched to PLAY", conn, player->username);

        struct hp_result result = send_post_login_sequence(conn);
        if(result.result != HP_RESULT_OK) free(player);
        return result;
    }
}











    struct hp_result handle_lg_encryption_response(const struct mcpr_packet *pkt, struct connection *conn)
    {
        nlog_debug("in handle_lg_encryption_response(pkt = %p, conn = %p)", (void *) pkt, (void *) conn);
        if(!conn->tmp_present)
        {
            nlog_error("Could not handle login start packet, tmp_present was false.");
            char *reason = mcpr_as_chat("A fatal error occurred whilst logging in.");
            struct hp_result result;
            result.result = HP_RESULT_FATAL;
            result.disconnect_message = reason;
            result.free_disconnect_message = true;
            return result;
        }

        void *decrypted_shared_secret = NULL;
        uint8_t server_id_hash[SHA_DIGEST_LENGTH];
        char stringified_server_id_hash[SHA_DIGEST_LENGTH * 2 + 2];
        struct player *player;
        bool player_init = false;

        int32_t shared_secret_length = pkt->data.login.serverbound.encryption_response.shared_secret_length;
        void *shared_secret = pkt->data.login.serverbound.encryption_response.shared_secret;

        if(shared_secret_length > INT_MAX || shared_secret_length < INT_MIN)
        {
            nlog_error("Integer overflow.");
            goto err;
        }

        decrypted_shared_secret = malloc(RSA_size(conn->tmp.rsa));
        if(decrypted_shared_secret == NULL)
        {
            nlog_error("Could not allocate memory. (%s)", strerror(errno));
            goto err;
        }

        nlog_debug("Attempting to decrypt shared secret of (encrypted) length %lld", (long long) shared_secret_length);
        nlog_debug("RSA_size(rsa) is %d", RSA_size(conn->tmp.rsa));
        if(shared_secret_length > RSA_size(conn->tmp.rsa))
        {
            nlog_error("Shared secret length is greater than RSA_size(rsa)");
            goto err;
        }
        int size = RSA_private_decrypt((int) shared_secret_length, (unsigned char *) shared_secret, (unsigned char *) decrypted_shared_secret, conn->tmp.rsa, RSA_NO_PADDING);
        if(size < 0)
        {
            nlog_error("Could not decrypt shared secret.");
            ERR_print_errors_fp(fp_error);
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
            nlog_error("Error upon EVP_EncryptInit_ex().");
            ERR_print_errors_fp(fp_error);
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
            nlog_error("Error upon EVP_DecryptInit_ex().");
            ERR_print_errors_fp(fp_error);
            goto err;
        }
        mcpr_connection_set_crypto(conn->conn, ctx_encrypt, ctx_decrypt);

        unsigned char *encoded_public_key = NULL; // TODO should this be freed afterwards?
        int encoded_public_key_len = i2d_RSA_PUBKEY(conn->tmp.rsa, &encoded_public_key);
        if(encoded_public_key_len < 0)
        {
            // Error occured.
            nlog_error("Ermg ze openssl errorz!!");
            ERR_print_errors_fp(fp_error);
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
        nlog_debug("Value before mcpr_crypto_stringify_sha1: %p", (void *) conn);
        mcpr_crypto_stringify_sha1(stringified_server_id_hash, server_id_hash);
        nlog_debug("Value after mcpr_crypto_stringify_sha1: %p", (void *) conn);
        struct mapi_minecraft_has_joined_response *mapi_result = mapi_minecraft_has_joined(conn->tmp.username, stringified_server_id_hash, conn->server_address_used);
        nlog_debug("Value after mapi_minecraft_has_joined: %p", (void *) conn);
        if(mapi_result == NULL)
        {
            // TODO handle legit non error case where a failure happened.
            ninerr_print(ninerr);
            goto err;
        }


        struct mcpr_packet response;
        response.id = MCPR_PKT_LG_CB_LOGIN_SUCCESS;
        response.state = MCPR_STATE_LOGIN;
        response.data.login.clientbound.login_success.uuid = mapi_result->id;
        response.data.login.clientbound.login_success.username = conn->tmp.username; // eh i think we should get the username from another source?

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


        player = create_player(conn, mapi_result->id);
        if(player == NULL) goto err;
        player_init = true;

        struct hp_result send_post_login_sequence_result = send_post_login_sequence(conn);
        if(send_post_login_sequence_result.result == HP_RESULT_FATAL) goto err;
        else if(send_post_login_sequence_result.result == HP_RESULT_CLOSED) goto closed;
        else goto cleanup_only;

    err:
        nlog_debug("Last address of conn: %p", (void *) conn);
        RSA_free(conn->tmp.rsa);
        free(conn->tmp.verify_token);
        free(conn->tmp.username);
        conn->tmp_present = false;
        free(decrypted_shared_secret);
        if(player_init) free(player);
        char *reason = mcpr_as_chat("A fatal error occurred whilst logging in.");
        struct hp_result result1;
        result1.result = HP_RESULT_FATAL;
        result1.disconnect_message = reason;
        result1.free_disconnect_message = true;
        return result1;

    cleanup_only:
        RSA_free(conn->tmp.rsa);
        free(conn->tmp.verify_token);
        conn->tmp_present = false;
        free(decrypted_shared_secret);
        struct hp_result result2;
        result2.result = HP_RESULT_OK;
        result2.disconnect_message = NULL;
        result2.free_disconnect_message = false;
        return result2;

    closed:
        RSA_free(conn->tmp.rsa);
        free(conn->tmp.verify_token);
        free(conn->tmp.username);
        conn->tmp_present = false;
        free(decrypted_shared_secret);
        if(player_init) free(player);
        struct hp_result result3;
        result3.result = HP_RESULT_CLOSED;
        result3.disconnect_message = NULL;
        result3.free_disconnect_message = false;
        return result3;
}

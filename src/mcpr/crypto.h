#ifndef MCPR_CRYPTO_H
#define MCPR_CRYPTO_H

#include <stdbool.h>

#include <sys/types.h>
#include <ninstd/types.h>

#include <openssl/evp.h>
#include <openssl/rsa.h>

#include <ninio/bstream.h>


/**
 * Encrypt data for use in the Minecraft protocol.
 *
 * @param [out] out Output buffer, should be at least the size of (len + mcpr_client->encryption_block_size -1), may not be NULL.
 * @param [in] data Raw binary data to encrypt, should be at least the size of len.
 * @param [in] ctx_encrypt ctx_encrypt from the mcpr_client struct.
 *
 * @returns The amount of bytes written to out, or a negative integer upon error.
 */
isize mcpr_crypto_encrypt(void *restrict out, const void *restrict in, EVP_CIPHER_CTX *ctx_encrypt, usize len);

/**
 * Decrypt data for use in the Minecraft protocol.
 *
 * @param [out] out Output buffer, should be at least size of (len + encryption_block_size), may not be NULL.
 * @param [in] data Input buffer, should be at least the size of len, may not be NULL.
 *
 * @returns The amount of bytes written to out, or a negative integer upon error.
 */
isize mcpr_crypto_decrypt(void *restrict out, const void *restrict in, EVP_CIPHER_CTX *ctx_decrypt, usize len);

/**
 * @param [out] out Output buffer, should be at least size of SHA_DIGEST_LENGTH
 */
bool mcpr_crypto_auth_hash(void *out, const char *server_id, const void *shared_secret, usize shared_secret_len, const RSA *server_public_key);

/*
 * @param [out] out Output buffer, should be the size of SHA_DIGEST_LENGTH * 2 + 2 where SHA_DIGEST_LENGTH is 20
 */
bool mcpr_crypto_auth_digest(char *out, const char *server_id, const void *shared_secret, usize shared_secret_len, const RSA *server_public_key);

/*
 * @param [out] out Output buffer, should be the size of SHA_DIGEST_LENGTH * 2 + 2 where SHA_DIGEST_LENGTH is 20
 */
void mcpr_crypto_stringify_sha1(char *out, const void *hash);

#endif

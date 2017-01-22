#ifndef MCPR_CRYPTO_H
#define MCPR_CRYPTO_H


/**
 * Encrypt data for use in the Minecraft protocol.
 *
 * @param [out] out Output buffer, should be at least the size of (len + mcpr_client->encryption_block_size -1), may not be NULL.
 * @param [in] data Raw binary data to encrypt, should be at least the size of len.
 * @param [in] ctx_encrypt ctx_encrypt from the mcpr_client struct.
 *
 * @returns The amount of bytes written to out, or a negative integer upon error.
 */
ssize_t mcpr_crypto_encrypt(void restrict* out, const void restrict* in, EVP_CIPHER_CTX ctx_encrypt, size_t len);

/**
 * Decrypt data for use in the Minecraft protocol.
 *
 * @param [out] out Output buffer, should be at least size of (len + encryption_block_size), may not be NULL.
 * @param [in] data Input buffer, should be at least the size of len, may not be NULL.
 *
 * @returns The amount of bytes written to out, or a negative integer upon error.
 */
ssize_t mcpr_crypto_decrypt(void restrict* out, const void restrict* in, EVP_CIPHER_CTX ctx_decrypt, size_t len);

// These functions may change internal state.
// Synchronizing should be done by the user, based on the file pointer.
// mcpr_fflush() should be done to flush potentional buffers after both reading and writing.
ssize_t mcpr_crypto_fread(FILE *in, void *out, size_t len, bool use_encryption, EVP_CIPHER_CTX *ctx_decrypt);
ssize_t mcpr_crypto_fwrite(FILE *out, void *in, size_t len, bool use_encryption, EVP_CIPHER_CTX *ctx_encrypt);
ssize_t mcpr_crypto_fflush(FILE *f);


ssize_t mcpr_crypto_generate_server_public_key(void *out, size_t pubkey_len);
ssize_t mcpr_crypto_generate_auth_hash(void *out, char *server_id, void *shared_secret, size_t shared_secret_len, void *server_pubkey, size_t server_pubkey_len);
ssize_t mcpr_crypto_stringify_sha1(char *out, const void *hash);

#endif

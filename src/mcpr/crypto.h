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
*/

#ifndef MCPR_CRYPTO_H
#define MCPR_CRYPTO_H

#include <stdbool.h>

#include <sys/types.h>

#include <openssl/evp.h>

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
ssize_t mcpr_crypto_encrypt(void *restrict out, const void *restrict in, EVP_CIPHER_CTX *ctx_encrypt, size_t len);

/**
 * Decrypt data for use in the Minecraft protocol.
 *
 * @param [out] out Output buffer, should be at least size of (len + encryption_block_size), may not be NULL.
 * @param [in] data Input buffer, should be at least the size of len, may not be NULL.
 *
 * @returns The amount of bytes written to out, or a negative integer upon error.
 */
ssize_t mcpr_crypto_decrypt(void *restrict out, const void *restrict in, EVP_CIPHER_CTX *ctx_decrypt, size_t len);


ssize_t mcpr_crypto_generate_auth_hash(void *out, char *server_id, void *shared_secret, size_t shared_secret_len, void *server_pubkey, size_t server_pubkey_len);

// Out should be the size of SHA_DIGEST_LENGTH * 2 + 2 where SHA_DIGEST_LENGTH is 20
void mcpr_crypto_stringify_sha1(char *out, const void *hash);

#endif

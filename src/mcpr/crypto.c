#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/sha.h>

#include <ninerr/ninerr.h>

#include "mcpr/crypto.h"
#include "mcpr/mcpr.h"

#include "internal.h"



ssize_t mcpr_crypto_encrypt(void *out, const void *data, EVP_CIPHER_CTX *ctx_encrypt, size_t len)
{
    if(len > INT_MAX) { ninerr_set_err(ninerr_arithmetic_new()); return -1; }

    int writtenlen;
    if(EVP_EncryptUpdate(ctx_encrypt, (unsigned char *) out, &writtenlen, (const unsigned char *) data, (int) len) == 0)
    {
        ninerr_set_err(ninerr_new("EVP_EncryptUpdate failed.", false));
        return -1;
    }
    return writtenlen;
}

ssize_t mcpr_crypto_decrypt(void *out, const void *data, EVP_CIPHER_CTX *ctx_decrypt, size_t len)
{
    if(len > INT_MAX) { ninerr_set_err(ninerr_arithmetic_new()); return -1; }

    int writtenlen;
    if(EVP_DecryptUpdate(ctx_decrypt, (unsigned char *) out, &writtenlen, (unsigned char *) data, (int) len) == 0)
    {
        ninerr_set_err(ninerr_new("EVP_DecryptUpdate failed.", false));
        return -1;
    }
    return writtenlen;
}

bool mcpr_crypto_auth_hash(void *out, const char *server_id, const void *shared_secret, size_t shared_secret_len, const RSA *public_key)
{
    u8 *public_key_bytes = NULL;
    isize public_key_bytes_len = i2d_RSA_PUBKEY(public_key, &public_key_bytes);
    if(public_key_bytes_len < 0)
    {
        ninerr_set_err(ninerr_new("i2d_RSA_PUBKEY() failed.", false));
        return false;
    }

    SHA_CTX sha_ctx;
    if(SHA1_Init(&sha_ctx) == 0)
    {
        ninerr_set_err(ninerr_new("SHA1_Init() failed.", false));
        OPENSSL_free(public_key_bytes);
        return false;
    }
    if(SHA1_Update(&sha_ctx, shared_secret, shared_secret_len) == 0)
    {
        uint8_t ignored_tmpbuf[SHA_DIGEST_LENGTH];
        SHA1_Final(ignored_tmpbuf, &sha_ctx); // clean up
        ninerr_set_err(ninerr_new("SHA1_Update() failed.", false));
        OPENSSL_free(public_key_bytes);
        return false;
    }
    if(SHA1_Update(&sha_ctx, public_key_bytes, public_key_bytes_len) == 0)
    {
        uint8_t ignored_tmpbuf[SHA_DIGEST_LENGTH];
        SHA1_Final(ignored_tmpbuf, &sha_ctx); // clean up
        ninerr_set_err(ninerr_new("SHA1_Update() failed.", false));
        OPENSSL_free(public_key_bytes);
        return false;
    }
    if(SHA1_Final(out, &sha_ctx) == 0)
    {
        ninerr_set_err(ninerr_new("SHA1_Final() failed.", false));
        OPENSSL_free(public_key_bytes);
        return false;
    }

    OPENSSL_free(public_key_bytes);
    return true;
}

void mcpr_crypto_stringify_sha1(char *out, const void *hash)
{
    DEBUG_PRINT("in mcpr_crypto_stringify_sha1(out = %p, hash = %p)", (void *) out, (void *) hash);
    const u8 *bytes = (const u8 *) hash;
    bool is_negative = bytes[0] & 0x80;
    if(is_negative) *out = '-';

    char *outp = (is_negative) ? out + 1 : out;

    const u8 *final_bytes;
    if(is_negative)
    {
        u8 tmp[20];
        bool carry = true;
        u8 new_byte;
        u8 value;
        for(int_fast8_t i = 19; i >= 0; i--)
        {
            new_byte = ~(bytes[i]) & 0xFF;
            if(carry)
            {
                carry = new_byte == 0xFF;
                tmp[i] = new_byte + 1;
            }
            else
            {
                tmp[i] = new_byte;
            }
        }
        final_bytes = tmp;
    }
    else
    {
        final_bytes = bytes;
    }

    // Write it as a hex string.
    for(usize i = 0; i < 20; i++)
    {
        sprintf(outp, "%02x", final_bytes[i]);
        outp += 2;
    }
    *outp = '\0';

    // Trim leading zeros
    outp = (is_negative) ? out + 1 : out;
    usize zero_count = 0;
    for(usize i = 0; i < 40; i++)
    {
        if(outp[i] != '0') break;
        zero_count++;
    }
    if (zero_count > 0) memmove(outp, outp+zero_count, 42 - zero_count);
}


bool mcpr_crypto_auth_digest(char *out, const char *server_id, const void *shared_secret, usize shared_secret_len, const RSA *server_public_key)
{
    u8 hash[SHA_DIGEST_LENGTH];
    if(!mcpr_crypto_auth_hash(hash, server_id, shared_secret, shared_secret_len, server_public_key)) return false;
    mcpr_crypto_stringify_sha1(out, hash);
    return true;
}

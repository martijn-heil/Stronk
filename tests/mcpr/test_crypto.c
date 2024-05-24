#include <string.h>

#include <openssl/sha.h>
#include <ninstd/types.h>
#include <mcpr/crypto.h>
#include "../testing.h"
#include "test_crypto.h"

static void sha1_digest(char *output, const char *input)
{
    uint8_t buf[SHA_DIGEST_LENGTH];
    SHA_CTX sha_ctx;
    assert_true(SHA1_Init(&sha_ctx));
    assert_true(SHA1_Update(&sha_ctx, input, strlen(input)));
    assert_true(SHA1_Final(buf, &sha_ctx));

    mcpr_crypto_stringify_sha1(output, buf);
}

void test_mcpr_crypto_stringify_sha1(void **state)
{
    char output[SHA_DIGEST_LENGTH * 2 + 2];

    sha1_digest(output, "Notch");
    assert_string_equal(output, "4ed1f46bbe04bc756bcb17c0c7ce3e4632f06a48");

    sha1_digest(output, "jeb_");
    assert_string_equal(output, "-7c9d5b0044c130109a5d7b5fb5c317c02b4e28c1");

    sha1_digest(output, "simon");
    assert_string_equal(output, "88e16a1019277b15d58faf0541e11910eb756f6");
}

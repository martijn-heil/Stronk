#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <openssl/ssl.h>
#include <openssl/opensslv.h>

#include "mcpr/test_codec.h"
#include "mcpr/test_crypto.h"

/* A test case that does nothing and succeeds. */
static void null_test_success(void **state) {
    (void) state; /* unused */
}

int main(void) {
    OpenSSL_add_all_algorithms();
	SSL_load_error_strings();
	SSL_library_init();

    const struct CMUnitTest tests[] = {
        cmocka_unit_test(null_test_success),
        cmocka_unit_test(test_mcpr_encode_varint),
        cmocka_unit_test(test_mcpr_crypto_stringify_sha1),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}


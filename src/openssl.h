#pragma once
#include <openssl/crypto.h>
#include <openssl/rsa.h>
#include "utils.h"

static inline void _defer_openssl_free(void *p) { OPENSSL_free(*((void**) p)); }
static inline void _defer_rsa_free(RSA **p) { RSA_free(*p); }
#define defer_openssl_free defer(_defer_openssl_free)
#define defer_rsa_free defer(_defer_rsa_free)

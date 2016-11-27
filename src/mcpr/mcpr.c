#include <stdlib.h>
#include <stdint.h>

#ifdef MCPR_DO_LOGGING
    #include "../stronk.h"
#endif

#include "codec.h"
#include "mcpr.h"
#include "../util.h"
#include "../stack.h"



int mcpr_encrypt(void *out, const void *data, EVP_CIPHER_CTX ctx_encrypt, size_t len) {
    int writtenlen;
    if(unlikely(EVP_EncryptUpdate(&ctx_encrypt, (unsigned char *) out, &writtenlen, (unsigned char *) data, len) == 0)) {
        return -1;
    }
    return writtenlen;
}

int mcpr_decrypt(void *out, const void *data, EVP_CIPHER_CTX ctx_decrypt, size_t len) {
    int writtenlen;
    if(unlikely(EVP_DecryptUpdate(&ctx_decrypt, (unsigned char *) out, &writtenlen, (unsigned char *) data, len) == 0)) {
        return -1;
    }
    return writtenlen;
}


int mcpr_write_packet(struct mcpr_connection *conn, struct mcpr_packet *pkt) {
    uint8_t *buf = malloc(pkt->data_len + 10 + conn->encryption_block_size - 1);
    if(buf == NULL) {
        return -1;
    }

    // TODO insert header

    if(conn->use_encryption) {
        // TODO
    }

    if(conn->use_compression) {
        // TODO
    }

    return 0;
}

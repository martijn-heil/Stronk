#ifndef MCPR_ERROR_H
#define MCPR_ERROR_H

#include <stdint.h>
#include <ninerr/ninerr.h>


void mcpr_free_err_unsupported_protocol_version(struct ninerr *tmp1);
struct mcpr_err_unsupported_protocol_version *mcpr_err_unsupported_protocol_version(int32_t protocol_version);
struct mcpr_err_unsupported_protocol_version
{
    struct ninerr *super;
    int32_t protocol_version;
};

#endif

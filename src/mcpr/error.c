#include <stdlib.h>
#include <stdio.h>

#include <mcpr/error.h>

void mcpr_free_err_unsupported_protocol_version(struct ninerr *tmp1)
{
    struct mcpr_err_unsupported_protocol_version *tmp2 = (struct mcpr_err_unsupported_protocol_version *) tmp1->child;
    free(tmp2->super);
    free(tmp2->super->message);
    free(tmp2);
}

struct mcpr_err_unsupported_protocol_version *mcpr_err_unsupported_protocol_version(int32_t protocol_version)
{
    struct mcpr_err_unsupported_protocol_version *tmp = malloc(sizeof(struct mcpr_err_unsupported_protocol_version));
    if(tmp == NULL) return NULL;
    tmp->super = malloc(sizeof(tmp->super));
    if(tmp->super == NULL) { free(tmp); return NULL; }

    char *fmt = "Protocol version %i is not supported.";
    char tmpbuf;
    ssize_t required = snprintf(&tmpbuf, 1, fmt, protocol_version);
    if(required < 0) { return NULL; }
    char *msg = malloc(required);
    if(msg == NULL) return NULL;
    tmp->super->message = msg;
    tmp->super->child = tmp;
    tmp->super->type = "mcpr_err_unsupported_protocol_version";
    tmp->super->free = mcpr_free_err_unsupported_protocol_version;
    tmp->super->cause = NULL;
    return tmp;
}

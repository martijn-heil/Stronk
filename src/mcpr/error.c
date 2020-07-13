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

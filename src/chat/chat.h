/*
  MIT License

  Copyright (c) 2020 Martijn Heil

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

#include <stdbool.h>

#include "../network/connection.h"

enum chat_type
{
  CHAT_TYPE_CHAT,
  CHAT_TYPE_SYSTEM,
  CHAT_TYPE_GAME_INFO
};

struct chat_entry
{
  const char *msg;
  enum mcpr_chat_position position;
};

bool chat_send              (struct connection *conn, struct chat_entry entry);
int chat_send_if_appropiate (struct player *p, struct chat_entry entry, enum chat_type t);
bool chat_broadcast         (struct chat_entry entry, enum chat_type t);
bool chat_broadcast_forced  (struct chat_entry entry);

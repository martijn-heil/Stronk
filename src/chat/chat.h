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

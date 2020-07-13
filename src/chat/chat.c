#include <stdbool.h>
#include <stddef.h>

#include <pthread.h>

#include <server.h>
#include <mcpr/packet.h>

#include <network/player.h>
#include <network/connection.h>

#include <logging/logging.h>

#include "chat.h"


// [chat_mode][chat_type]
static bool should_send_table[3][3] =
{
  {true, true, true},
  {false, true, true},
  {false, false, true}
};
static size_t chat_mode_to_index(enum mcpr_chat_mode mode)
{
  switch(mode)
  {
    case MCPR_CHAT_MODE_ENABLED: return 0;
    case MCPR_CHAT_MODE_COMMANDS_ONLY: return 1;
    case MCPR_CHAT_MODE_HIDDEN: return 2;
  }
  abort();
}
static size_t chat_type_to_index(enum chat_type t)
{
  switch(t)
  {
    case CHAT_TYPE_CHAT: return 0;
    case CHAT_TYPE_SYSTEM: return 1;
    case CHAT_TYPE_GAME_INFO: return 2;
  }
  abort();
}
static bool should_send(enum mcpr_chat_mode mode, enum chat_type t)
{
  return should_send_table[chat_mode_to_index(mode)][chat_type_to_index(t)];
}

bool chat_send(struct connection *conn, struct chat_entry entry)
{
  struct mcpr_packet pkt;
  pkt.data.play.clientbound.chat_message.json_data = entry.msg;
  pkt.data.play.clientbound.chat_message.position = entry.position;

  if(fwrite(&pkt, sizeof(pkt), 1, conn->pktstream) == 0)
  {
    return false;
  }
  return true;
}

// returns 0 on failure
// returns 1 on success
// returns 2 if the message is not sent because of player chat settings
int chat_send_if_appropiate(struct player *p, struct chat_entry entry, enum chat_type t)
{
  if(p->client_settings_known && !should_send(p->client_settings.chat_mode, t))
  {
    return 2;
  }

  return chat_send(p->conn, entry);
}

bool chat_broadcast(struct chat_entry entry, enum chat_type t)
{
  pthread_rwlock_t *players_lock = server_get_players_lock();
  again: if(pthread_rwlock_rdlock(players_lock) != 0) { nlog_warn("Could not lock server_get_players_lock() lock. Retrying.."); goto again; }

  size_t players_count;
  struct player **players;
  server_get_players(&players, &players_count);
  for (size_t i = 0; i < players_count; i++)
  {
    struct player *p = players[i];
    chat_send_if_appropiate(p, entry, t);
  }
  free(players);
  pthread_rwlock_unlock(players_lock);
  return true;
}

bool chat_broadcast_forced(struct chat_entry entry)
{
  pthread_rwlock_t *players_lock = server_get_players_lock();
  again: if(pthread_rwlock_rdlock(players_lock) != 0) { nlog_warn("Could not lock server_get_players_lock() lock. Retrying.."); goto again; }

  size_t players_count;
  struct player **players;
  server_get_players(&players, &players_count);
  for (size_t i = 0; i < players_count; i++)
  {
    struct player *p = players[i];
    struct connection *conn = p->conn;
    chat_send(conn, entry);
  }
  free(players);
  pthread_rwlock_unlock(players_lock);
  return true;
}

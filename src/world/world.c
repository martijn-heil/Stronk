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
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>

#include <inttypes.h>

#include <sys/types.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <algo/hash-ull.h>
#include <algo/compare-ull.h>
#include <algo/hash-table.h>

#include <ninerr/ninerr.h>

#include <mcpr/mcpr.h>
#include <mcpr/packet.h>
#include <mcpr/connection.h>

#include <logging/logging.h>
#include <network/connection.h>
#include <network/player.h>

#include "world/world.h"
#include "world/block.h"
#include "../util.h"

int32_t entity_id_counter = INT32_MIN;
pthread_mutex_t entity_id_counter_mutex = PTHREAD_MUTEX_INITIALIZER;

//#define block_xz_to_index(x, z) ((z-1) * 16 + x - 1)


/*
    This is a bottom layer of 16x16 blocks of a chunk section, a chunk contains 256 layers of these.
    They are indexed bottom to top. 0 to 15. The layer displayed here is the top one, the 1st.
    (Would be 0 if starting index at 0 instead of 1)

                        North(-z)

      +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
      | 0  | 1  | 2  | 3  | 4  | 5  | 6  | 7  | 8  | 9  | 10 | 11 | 12 | 13 | 14 | 15 |
      +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
      | 16 | 17 | 18 | 19 | 20 | 21 | 22 | 23 | 24 | 25 | 26 | 27 | 28 | 29 | 30 | 31 |
      +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
      | 32 | 33 | 34 | 35 | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | And so on..
      +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
      | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  |
      +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
      | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  |
      +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
      | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  |
      +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
      | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  |
      +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
      | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  |
West(-x)  +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+   East(+x)
      | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  |
      +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
      | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  |
      +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
      | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  |
      +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
      | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  |
      +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
      | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  |
      +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
      | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  |
      +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
      | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  |
      +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
      | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  |
      +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+

                        South (+z)


West to east, north to south block changing is the fastest (as they are indexed in that direction), so from bottom to top, left to right:

 -------------------------------------->
 -------------------------------------->
 -------------------------------------->
 -------------------------------------->
 -------------------------------------->
 -------------------------------------->
 -------------------------------------->
 -------------------------------------->
 -------------------------------------->
 -------------------------------------->


*/

//#define xz_to_index(x, z)

#define BLOCKS_PER_CHUNK_SECTION 4096
#define CHUNK_SECTIONS_PER_CHUNK 16
#define BLOCKS_PER_CHUNK (BLOCKS_PER_CHUNK_SECTION * CHUNK_SECTIONS_PER_CHUNK)

static struct chunk *load_chunk(world *world, long x, long z);
static bool send_chunk_data(const struct player *p, const struct chunk *chunk, long x, long z, bool send_sky_light);

struct chunk_section {
  pthread_rwlock_t lock;

  struct block blocks[4096]; // 16x16x16
};
static void encode_chunk_section_blocks(uint64_t *out, const struct chunk_section *section);

struct chunk
{
  unsigned long long last_update;
  struct chunk_section sections[CHUNK_SECTIONS_PER_CHUNK]; // 16 high indexed bottom to top.
  uint8_t biomes[256];
};

#define get_chunk_key(x, z) ((unsigned long long) (((unsigned long long) x) << 32 | ((unsigned long long) z)))
struct world
{
  HashTable *chunks;
  enum mcpr_dimension dimension;
};

struct world *default_world = NULL;
unsigned int server_view_distance = 15;


int world_manager_init(void)
{
  default_world = malloc(sizeof(struct world));
  if(default_world == NULL) { nlog_fatal("Could not allocate memory. (%s)", strerror(errno)); return -1; }

  default_world->chunks = hash_table_new(ull_hash, ull_equal);
  if(default_world->chunks == NULL) { nlog_fatal("Could not create hash table. (%s ?)", strerror(errno)); free(default_world); return -1; }
  default_world->dimension = MCPR_DIMENSION_OVERWORLD;
  return 1;
}

void world_manager_cleanup(void)
{
  // TODO free memory from world
}

static struct chunk *get_chunk(world *w, long x, long z)
{
  unsigned long long key = get_chunk_key(x, z);
  struct chunk *chunk = hash_table_lookup(default_world->chunks, &key);
  return (chunk != HASH_TABLE_NULL) ? chunk : load_chunk(w, x, z);
}

IGNORE("-Wunused-parameter")
static struct chunk *load_chunk(world *world, long x, long z)
{
  // TODO later we need to read a chunk from disk, but for now we just use dummy chunks filled with stone up to a certain level.
  struct chunk *chunk = malloc(sizeof(struct chunk));
  if(chunk == NULL) return NULL;

  chunk->last_update = 0; // TODO

  // Fill the bottom 8 chunk sections with solid stone.
  for(size_t i = 0; i < 8; i++)
  {
    struct chunk_section *chunk_section = &(chunk->sections[i]);

    for(int i = 0; i < BLOCKS_PER_CHUNK_SECTION; i++)
    {
      chunk_section->blocks[i].type_id = 1;
      chunk_section->blocks[i].data = 0;
    }
  }

  // Fill the top 8 chunk sections with air.
  for(size_t i = 8; i < 16; i++)
  {
    struct chunk_section *chunk_section = &(chunk->sections[i]);

    for(int i = 0; i < BLOCKS_PER_CHUNK_SECTION; i++)
    {
      chunk_section->blocks[i].type_id = 0;
      chunk_section->blocks[i].data = 0;
    }
  }

  // Fill in biome data.
  memset(chunk->biomes, 0, 256);

  unsigned long long key = get_chunk_key(x, z);
  int result = hash_table_insert(((struct world *) world)->chunks, &key, chunk);
  if(result == 0){ nlog_error("Could not put new chunk in chunk map. (%s ?)", strerror(errno)); free(chunk); return NULL; }

  return chunk;
}
END_IGNORE()

bool world_send_chunk_data1(const struct player *p) // TODO
{
  unsigned short view_distance = 10; // TODO proper view distance
  long base_x = p->pos.x / 16;
  long base_z = p->pos.z / 16;
  for(int mod_x = -view_distance; mod_x <= view_distance; mod_x++)
  {
    for(int mod_z = -view_distance; mod_z <= view_distance; mod_z++)
    {
      long x = base_x + mod_x;
      long z = base_z + mod_z;

      send_chunk_data(p, get_chunk(default_world, x, z), x, z, default_world->dimension == MCPR_DIMENSION_OVERWORLD);
    }
  }

  return true;
}

static bool send_chunk_data(const struct player *p, const struct chunk *chunk, long x, long z, bool send_sky_light)
{
  nlog_debug("In send_chunk_data(player = %s, chunkX = %ld, chunkZ = %ld)", p->username, x, z);

  struct mcpr_packet pkt;
  pkt.id = MCPR_PKT_PL_CB_CHUNK_DATA;
  pkt.state = MCPR_STATE_PLAY;
  pkt.data.play.clientbound.chunk_data.chunk_x = x;
  pkt.data.play.clientbound.chunk_data.chunk_z = z;
  pkt.data.play.clientbound.chunk_data.ground_up_continuous = true;
  pkt.data.play.clientbound.chunk_data.primary_bit_mask = 0xFFFF;
  pkt.data.play.clientbound.chunk_data.size = CHUNK_SECTIONS_PER_CHUNK;
  pkt.data.play.clientbound.chunk_data.block_entities = NULL;
  pkt.data.play.clientbound.chunk_data.block_entity_count = 0;
  void *membuf = malloc(CHUNK_SECTIONS_PER_CHUNK * sizeof(struct mcpr_chunk_section));
  if(membuf == NULL)
  {
    nlog_error("Could not allocate memory. (%s)", strerror(errno));
    return false;
  }

  pkt.data.play.clientbound.chunk_data.chunk_sections = membuf;
  pkt.data.play.clientbound.chunk_data.biomes = chunk->biomes;


  for(size_t i = 0; i < CHUNK_SECTIONS_PER_CHUNK; i++)
  {
    const struct chunk_section *section = &(chunk->sections[i]);

    struct mcpr_chunk_section *mcpr_chunk_section = pkt.data.play.clientbound.chunk_data.chunk_sections + i;
    mcpr_chunk_section->bits_per_block = 13;
    mcpr_chunk_section->palette_length = 0;
    mcpr_chunk_section->palette = NULL;

    void *membuf2 = (send_sky_light) ? malloc(BLOCKS_PER_CHUNK_SECTION / 2 * 2 + 832 * sizeof(uint64_t)) : malloc(BLOCKS_PER_CHUNK_SECTION / 2 + 832 * sizeof(uint64_t));
    if(membuf2 == NULL)
    {
      nlog_error("Could not allocate memory. (%s)", strerror(errno));
      return false;
    }
    mcpr_chunk_section->block_light = membuf2;
    memset(mcpr_chunk_section->block_light, 0, BLOCKS_PER_CHUNK_SECTION / 2); // TODO block light.

    if(send_sky_light)
    {
      mcpr_chunk_section->sky_light = membuf2 + BLOCKS_PER_CHUNK_SECTION / 2;
      memset(mcpr_chunk_section->sky_light, ((((uint8_t) 15) << 4) | ((uint8_t) 15)), BLOCKS_PER_CHUNK_SECTION / 2); // TODO sky light.
    }
    else
    {
      mcpr_chunk_section->sky_light = NULL;
    }


    mcpr_chunk_section->blocks = (send_sky_light) ? membuf2 + BLOCKS_PER_CHUNK_SECTION / 2 * 2 : membuf2 + BLOCKS_PER_CHUNK_SECTION / 2;
    mcpr_chunk_section->block_array_length = 832;

    encode_chunk_section_blocks(mcpr_chunk_section->blocks, section);
	  //memset(mcpr_chunk_section->blocks, 0, 832 * sizeof(uint64_t));
  }

  const struct connection *conn = player_get_connection(p);
  if(fwrite(&pkt, sizeof(pkt), 1, conn->pktstream) == 0)
  {
    nlog_error("Could not send chunk data packet.");
    ninerr_print(ninerr);
    return false;
  }

  // Clean up..
  for(int i = 0; i < CHUNK_SECTIONS_PER_CHUNK; i++)
  {
    free(pkt.data.play.clientbound.chunk_data.chunk_sections[i].block_light);
  }
  free(pkt.data.play.clientbound.chunk_data.chunk_sections);
  return true;
}

// section_y from 0 to 15 (inclusive)
static bool send_chunk_section_data(const struct player *p, const struct chunk_section *section, long long chunk_x, long long chunk_z, unsigned char section_y)
{
  struct mcpr_packet pkt;
  pkt.id = MCPR_PKT_PL_CB_CHUNK_DATA;
  pkt.state = MCPR_STATE_PLAY;
  pkt.data.play.clientbound.chunk_data.chunk_x = chunk_x;
  pkt.data.play.clientbound.chunk_data.chunk_z = chunk_z;
  pkt.data.play.clientbound.chunk_data.ground_up_continuous = false;
  pkt.data.play.clientbound.chunk_data.primary_bit_mask = 0xFFFF;
  pkt.data.play.clientbound.chunk_data.block_entity_count = 0;
  pkt.data.play.clientbound.chunk_data.block_entities = NULL;
  pkt.data.play.clientbound.chunk_data.size = 1;


  void *membuf = malloc(sizeof(struct mcpr_chunk_section) + BLOCKS_PER_CHUNK_SECTION / 2 + 832 * sizeof(uint64_t));
  if(membuf == NULL)
  {
    nlog_error("Could not allocate memory. (%s)", strerror(errno));
    return false;
  }

  pkt.data.play.clientbound.chunk_data.chunk_sections = (struct mcpr_chunk_section *) membuf;


  struct mcpr_chunk_section mcpr_chunk_section;
  mcpr_chunk_section.bits_per_block = 13;
  mcpr_chunk_section.palette_length = 0;
  mcpr_chunk_section.palette = NULL;
  mcpr_chunk_section.sky_light = NULL;
  mcpr_chunk_section.block_light = membuf + sizeof(struct mcpr_chunk_section);
  memset(mcpr_chunk_section.block_light, 0, BLOCKS_PER_CHUNK_SECTION / 2); // TODO block light.

  mcpr_chunk_section.blocks = membuf + sizeof(struct mcpr_chunk_section) + BLOCKS_PER_CHUNK_SECTION / 2;
  mcpr_chunk_section.block_array_length = 832;
  encode_chunk_section_blocks(mcpr_chunk_section.blocks, section);

  pkt.data.play.clientbound.chunk_data.chunk_sections[section_y] = mcpr_chunk_section;

  struct connection *conn = p->conn;
  if(fwrite(&pkt, sizeof(pkt), 1, conn->pktstream) == 0)
  {
    nlog_error("Could not send chunk data packet.");
    ninerr_print(ninerr);
    return false;
  }

  // Clean up..
  free(membuf);
  return true;
}


int32_t generate_new_entity_id(void)
{
  pthread_mutex_lock(&entity_id_counter_mutex);
  int32_t tmp = ++entity_id_counter;
  pthread_mutex_unlock(&entity_id_counter_mutex);
  return tmp;
}

static void encode_chunk_section_blocks(uint64_t *out, const struct chunk_section *section)
{
  uint_fast8_t bits_used = 0;
  uint64_t tmp_result = 0;
  size_t block_data_index = 0;
  for(unsigned int i = 0; i < BLOCKS_PER_CHUNK_SECTION; i++)
  {
    const struct block *block = &(section->blocks[i]);
    uint64_t current_block_data = ((((uint64_t) block->type_id) << 4) | ((uint64_t) block->data)); // This line has been tested, it works.
    tmp_result |= (current_block_data << bits_used);
    bits_used += 13;

    if(bits_used >= 64 || i == BLOCKS_PER_CHUNK_SECTION - 1)
    {
      out[block_data_index] = hton64(tmp_result);
      block_data_index++;

      if(bits_used > 64) // It overflowed, we need to continue the last block in the next long.
      {
        uint_fast8_t overflowed_bits = bits_used - 64;
        tmp_result = current_block_data >> (13 - overflowed_bits);
        bits_used = overflowed_bits;
      }
      else
      {
        bits_used = 0;
        tmp_result = 0;
      }
    }
  }
}

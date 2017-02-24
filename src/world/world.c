#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#include <sys/types.h>
#include <pthread.h>

#include <algo/hash-table.h>

#include <mcpr/mcpr.h>
#include <mcpr/abstract_packet.h>

#include <logging/logging.h>
#include <network/connection.h>

#include "world.h"

//#define block_xz_to_index(x, z) ((z-1) * 16 + x - 1)
struct block {
    unsigned int type_id;
    unsigned char damage_value;
    void *extra_data;
};

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
West(-x)    +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+   East(+x)
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

#define xz_to_index(x, z)

#define BLOCKS_PER_CHUNK_SECTION 4096
#define CHUNK_SECTIONS_PER_CHUNK 16
#define BLOCKS_PER_CHUNK (BLOCKS_PER_CHUNK_SECTION * CHUNK_SECTIONS_PER_CHUNK)

struct chunk_section {
    pthread_rwlock_t lock;

    struct block blocks[4096]; // 16x16x16
};

struct chunk
{
    long x;
    long z;
    unsigned long long last_update;
    struct chunk_section sections[16]; // 16 high indexed bottom to top.
};

#define get_chunk_key(x, z) ((unsigned long long) (((unsigned long long) x) << 32 | ((unsigned long long) z)))
struct world
{
    HashTable *chunks;
};

struct world *world;
unsigned int server_view_distance = 15;


int world_manager_init(void)
{
    world = malloc(sizeof(struct world));
    if(world == NULL) { nlog_fatal("Could not allocate memory. (%s)", strerror(errno)); return -1; }

    world->chunks = hash_table_new(ull_hash, ull_equal);
    if(chunks == NULL) { nlog_fatal("Could not create hash table. (%s ?)", strerror(errno)) return -1; }
};

static struct chunk *get_chunk(world *w, long x, long z)
{
    unsigned long long key = get_chunk_key(x, z);
    struct chunk *chunk1 = hash_table_lookup(world->chunks, &key);
    if(chunk1 != HASH_TABLE_NULL)
    {
        return chunk1;
    }
    else
    {
        struct chunk *chunk2 = load_chunk(w, x, z);
        if(chunk2 == NULL) return NULL;
        return chunk2;
    }
}

static struct chunk *load_chunk(world *world, long x, long z)
{
    // TODO later we need to read a chunk from disk, but for now we just use dummy chunks filled with stone up to a certain level.
    struct chunk *chunk = malloc(sizeof(struct chunk));
    if(chunk == NULL) return NULL;

    chunk->sections = malloc(CHUNK_SECTIONS_PER_CHUNK * sizeof(struct chunk_section));
    if(chunk->sections == NULL) return NULL;

    chunk->x = x;
    chunk->z = z;
    chunk->last_update = 0; // TODO

    // Fill the bottom 8 chunk sections with solid stone.
    for(int i = 0; i < 8; i++)
    {
        struct chunk_section *chunk_section = chunk->sections[i];

        for(int i = 0; i < BLOCKS_PER_CHUNK_SECTION; i++)
        {
            chunk_section->blocks[i]->type_id = 1;
            chunk_section->blocks[i]->damage_value = 0;
        }
    }

    return chunk;
}

int world_send_chunk_data(player *p)
{
    struct player_client_settings *settings = player_get_client_settings(p);
    int view_distance = settings->view_distance <= server_view_distance ? settings->view_distance : server_view_distance;


    while(true)
    {
        unsigned long long key = get_chunk_key(x, z);
        struct chunk *chunk = hash_table_lookup(world, &key);

    }
}

static int send_chunk_data(player *p, const struct chunk *chunk)
{
    struct mcpr_abstract_packet pkt;
    pkt.id = MCPR_PKT_PL_CB_CHUNK_DATA;
    pkt.data.play.clientbound.chunk_data.chunk_x = chunk->x;
    pkt.data.play.clientbound.chunk_data.chunk_z = chunk->z;
    pkt.data.play.clientbound.chunk_data.ground_up_continuous = true;
    pkt.data.play.clientbound.chunk_data.primary_bit_mask = 0xFFFF;

    pkt.data.play.clientbound.chunk_data.chunk_sections = malloc(CHUNK_SECTIONS_PER_CHUNK * sizeof(struct mcpr_chunk_section));
    if(pkt.data.play.clientbound.chunk_data.chunk_sections == NULL)
    {
        nlog_error("Could not allocate memory. (%s)", strerror(errno));
        return -1;
    }

    for(int i = 0; i < CHUNK_SECTIONS_PER_CHUNK; i++)
    {
        struct chunk_section *section = chunk->sections[i];

        struct mcpr_chunk_section mcpr_chunk_section;
        mcpr_chunk_section.bits_per_block = 13;
        mcpr_chunk_section.palette_length = 0;
        mcpr_chunk_section.palette = NULL;
        mcpr_chunk_section.sky_light = NULL;
        mcpr_chunk_section.block_light = malloc(BLOCKS_PER_CHUNK_SECTION / 2);
        if(mcpr_chunk_section.block_light == NULL)
        {
            nlog_error("Could not allocate memory. (%s)", strerror(errno));
            return -1;
        }
        memset(mcpr_chunk_section.block_light, 0, BLOCKS_PER_CHUNK_SECTION / 2); // TODO block light.

        // 823 longs, the formula used to get at that number is as follows: BLOCKS_PER_CHUNK_SECTION / (64 / 13)
        // Where BLOCKS_PER_CHUNK_SECTION is obviously 4096 with the current setup.
        // Note that this formula requires floating point.
        mcpr_chunk_section.blocks = malloc(823 * sizeof(uint64_t));
        if(mcpr_chunk_section.blocks == NULL)
        {
            nlog_error("Could not allocate memory. (%s)", strerror(errno));
            free(mcpr_chunk_section.block_light);
            return -1;
        }
        mcpr_chunk_section.block_array_length = 823;

        unsigned char bits_used = 0;
        uint64_t tmp_result = 0;
        size_t block_data_index = 0;
        for(int i2 = 0; i2 < BLOCKS_PER_CHUNK_SECTION; i++)
        {
            struct block *block = section->blocks[i2];

            uint64_t current_block_data = (((uint64_t) block->type_id) << 4 | ((uint64_t) block->damage_value));
            tmp_result = tmp_result | (current_block_data << bits_used);
            bits_used += 13;

            if(bits_used >= 64 || i2 == BLOCKS_PER_CHUNK_SECTION - 1)
            {
                mcpr_chunk_section.blocks[block_data_index] = tmp_result;
                block_data_index++;
                tmp_result = 0;

                if(bits_used > 64) // It overflowed, we need to continue the last block in the next long.
                {
                    unsigned char overflowed_bits = bits_used - 64;
                    tmp_result = block_data >> (13 - overflowed_bits);
                    bits_used = overflowed_bits;
                }
                else
                {
                    bits_used = 0;
                }
            }
        }

        pkt.data.play.clientbound.chunk_data.chunk_sections[i] = mcpr_chunk_section;
    }

    struct connection *conn = player_get_connection(p);
    ssize_t result = connection_write_abstract_packet(conn, &pkt);
    if(result < 0)
    {
        nlog_error("Could not send chunk data packet. (%s)", mcpr_strerror(errno));
        return -1;
    }

    // Clean up..
    for(int i = 0; i < CHUNK_SECTIONS_PER_CHUNK; i++)
    {
        free(pkt.data.play.clientbound.chunk_data.chunk_sections[i].blocks)
        free(pkt.data.play.clientbound.chunk_data.chunk_sections[i].block_light)
    }
    free(pkt.data.play.clientbound.chunk_data.chunk_sections);
}

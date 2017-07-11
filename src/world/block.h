#ifndef STRONK_BLOCK_H
#define STRONK_BLOCK_H

#include <stdint.h>

struct block {
    uint16_t type_id;
    uint8_t data;
    void *extra_data;
};

enum block_type {
    AIR,
    STONE,
    GRANITE,
    DIORITE,
    ANDESITE,
    SOIL, // dirt, coarse dirt, podzol, grass, sand, red sand, mycelium
    COBBLESTONE, // cobblestone, mossy cobblestone
    PLANK,
    SAPLING,
    BEDROCK,
    FLUID, // water, lava (flowing and not flowing)
    GRAVEL,
    ORE, // all ores
    LOG, // all logs
    LEAVES, // all types of leaves
    SPONGE, // sponge, wet sponge
    GLASS, // all types of solid glass, including stained, including panes
    LAPIS_LAZULI,
    DISPENSER, // dispenser, dropper
    SANDSTONE, // chiseled sandstone, smooth sandstone
    NOTE_BLOCK,
    BED,
    RAIL, // all types of rail
    PISTON, // piston
    WOOL,
    FLOWER,
    MUSHROOM,
    GOLD,
    IRON,
    SLAB,
    DOUBLE_SLAB,
    BRICK, // nether too
    TNT,
    BOOKSHELF,
    OBSIDIAN,
    TORCH, // torch, redstone torch
    FIRE,
    MOB_SPAWNER,
    STAIRS, // all types of stairs
    CHEST, // trapped and normal
    REDSTONE_WIRE,
    DIAMOND_BLOCK,
    WORKBENCH,
    WHEAT,
    FURNACE,
    SIGN,
    DOOR, // all types of door
    LADDER,
    LEVER,
    PRESSURE_PLATE,
    REDSTONE_ORE, // redstone ore, glowsting redstone ore.
    BUTTON, // wood button, stone button
    SNOW, // snow block, snow layer
    CLAY,
    SUGAR_CANE,
    JUKEBOX,
    FENCE, // nether too
    PUMPKIN, // lit pumpkin & pumpkin
    NETHERRACK,
    SOUL_SAND,
    GLOWSTONE,
    PORTAL,
    CAKE,
    REDSTONE_REPEATER,
    TRAPDOOR,
    MONSTER_EGG,
    STONE_BRICK,
    MUSHROOM_BLOCK, // brown & red mushroom block
    STEM, // pumpkin stem, melon stem
    VINE,
    FENCE_GATE,
    LILY_PAD,
    NETHER_WART,
    ENCHANTMENT_TABLE,
    // TODO rest

};



#endif

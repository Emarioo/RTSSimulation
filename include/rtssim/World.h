#pragma once

#include "Engone/Util/Array.h"
#include "Engone/Util/BucketArray.h"

// The world has two parts: Tiles and entities
//  The map is divided into chunks and tiles.
//  Entities may exist in between multiple tiles.

// TODO: Use a TileRegistryType instead of TileType.
//  The type should come from a registry which allows a mod
//  to add their own types. 

enum TileType : u16 {
    TILE_EMPTY=0,
    TILE_GRASS=1,
    TILE_STONE,
};
struct Tile {
    TileType tileType;
    i8 height;
};
struct Chunk {
    int x, z;
    static const int TILES_PER_SIDE = 16;
    static const int TILES_PER_CHUNK = TILES_PER_SIDE * TILES_PER_SIDE;
    Tile tiles[TILES_PER_CHUNK];
};

enum EntityType : u16 {
    ENTITY_WORKER = 0,
    ENTITY_SOLDIER,
};
struct Entity{
    enum Flag : u16 {
        HIGHLIGHTED = 0x1,
    };
    EntityType entityType;
    u16 flags;
    float x, z;
    u32 extraData; // index or id into some array or other data structure
    u32 reserved1;
};
struct Registries;
struct World {
    void cleanup();

    static World* CreateTest(Registries* registries);
    static void Destroy(World* world);

    BucketArray<Chunk> chunks{128};
    BucketArray<Entity> entities{512};

    // @return Index of clicked entity. -1 if no entity was hit.
    u32 raycast(glm::vec3 rayPos, glm::vec3 rayDir, float rayDistance);

    Registries* registries = nullptr;
};
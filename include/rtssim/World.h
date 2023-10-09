#pragma once

#include "Engone/Util/Array.h"
#include "Engone/Util/BucketArray.h"
#include "rtssim/Config.h"

// The world has two parts: Tiles and entities
//  The map is divided into chunks and tiles.
//  Entities may exist in between multiple tiles.

// TODO: Use a TileRegistryType instead of TileType.
//  The type should come from a registry which allows a mod
//  to add their own types. 


enum TileType : u16 {
    TILE_EMPTY=0,
    TILE_TERRAIN=1,
    TILE_STONE,
    TILE_WOOD,
    TILE_IRON,
    
    TILE_TYPE_MAX,
};
const char* ToString(TileType tileType);
struct Tile {
    TileType tileType;
    i8 height;
    u8 red,green,blue;
    u32 amount; // wood, stone, iron...
    bool occupied = false; // whether collision is active for this tile
};
struct Chunk {
    int x, z;
    // static const float TILE_SIZE_IN_WORLD = 1.f;
    #define TILE_SIZE_IN_WORLD 1.f
    static const int TILES_PER_SIDE = 16;
    static const int TILES_PER_CHUNK = TILES_PER_SIDE * TILES_PER_SIDE;
    Tile tiles[TILES_PER_CHUNK];
};

enum EntityType : u16 {
    ENTITY_NONE = 0,
    ENTITY_WORKER = 1,
    ENTITY_SOLDIER,
    ENTITY_TRAINING_HALL,
    
    ENTITY_TYPE_MAX,
};
const char* ToString(EntityType entityType);
struct Entity{
    enum Flag : u16 {
        HIGHLIGHTED = 0x1,
    };
    EntityType entityType;
    u16 flags;
    glm::vec3 pos;
    u32 extraData; // index or id into some array or other data structure
    u32 reserved1;
};
struct Registries;

struct World {
    void cleanup();

    static World* CreateTest(Registries* registries);
    static World* CreateFromImage(Registries* registries, const char* world_save_folder);
    static void Destroy(World* world);

    BucketArray<Chunk> chunks{128};
    BucketArray<Entity> entities{512};

    // @return Index of clicked entity. -1 if no entity was hit.
    u32 raycast(glm::vec3 rayPos, glm::vec3 rayDir, float rayDistance);

    // Chunk* recentlyReferencedChunks[4]{nullptr}; // TODO: Optimize
    Tile* tileFromWorldPosition(float world_x, float world_z, int& tile_x, int& tile_z);
    Tile* tileFromGridPosition(int grid_x, int grid_z);

    Registries* registries = nullptr;
};
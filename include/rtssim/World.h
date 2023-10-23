#pragma once

#include "Engone/Util/Array.h"
#include "Engone/Util/BucketArray.h"
#include "rtssim/Config.h"
// #include "rtssim/Registry.h"
#include "rtssim/Pathfinding.h"

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

struct EntityAction {
    enum Type : u32 {
        ACTION_MOVE,
        ACTION_GATHER,
        ACTION_SEARCH_GATHER,
    };
    Type actionType;
    glm::vec3 targetPosition;
    TileType gatherMaterial;
    int grid_x,grid_z;
    bool hasTarget;
};

// per entity instance
struct EntityData {
    // Worker, soldiers
    DynamicArray<EntityAction> actionQueue{};
    bool gathering = false;
    float gatherTime = 0;
    // training hall
    bool busyTraining = false;

    Path* path=nullptr;
    int targetPointInPath = -1;
    float stepAcc = 0.f;
};
// per entity type
struct EntityStats {
    glm::vec3 color;
    glm::vec3 size;
    float moveSpeed;
};

struct Registries {
    EntityStats* getEntityStats(u32 entityType);

    glm::vec3 getTileColor(u32 tileType);

    u32 registerEntityData(EntityData** outEntityData = nullptr);
    void unregisterEntityData(u32 dataId);
    EntityData* getEntityData(u32 dataId);

    BucketArray<EntityData> entityDatas{512};
};

struct World {
    void cleanup();

    static World* CreateTest();
    static World* CreateFromImage(const char* world_save_folder);
    static void Destroy(World* world);

    BucketArray<Chunk> chunks{128};
    BucketArray<Entity> entities{512};

    // @return Index of clicked entity. -1 if no entity was hit.
    u32 raycast(glm::vec3 rayPos, glm::vec3 rayDir, float rayDistance);

    // Chunk* recentlyReferencedChunks[4]{nullptr}; // TODO: Optimize
    Tile* tileFromWorldPosition(const glm::vec3& pos, int& tile_x, int& tile_z);
    Tile* tileFromGridPosition(int grid_x, int grid_z);

    void gridPosFromWorld(const glm::vec3& pos, int& grid_x, int& grid_z, bool rounded = false) {
        if(rounded) {
            grid_x = roundf(pos.x / TILE_SIZE_IN_WORLD);
            grid_z = roundf(pos.z / TILE_SIZE_IN_WORLD);
        } else {
            grid_x = pos.x / TILE_SIZE_IN_WORLD;
            grid_z = pos.z / TILE_SIZE_IN_WORLD;
        }
    }

    // Registries* registries = nullptr;
    Registries registries{};
    Pathfinder pathfinder{};

    bool saveToFile(const char* path);
    bool loadFromFile(const char* path);
};
#pragma once

#include "Engone/Util/Array.h"
#include "rtssim/World.h"

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

private:
    BucketArray<EntityData> entityDatas{512};
};
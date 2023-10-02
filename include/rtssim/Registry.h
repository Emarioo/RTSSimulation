#pragma once

#include "Engone/Util/Array.h"

struct EntityAction {
    enum Type : u32 {
        ACTION_MOVE,
    };
    Type actionType;
    union {
        glm::vec3 movePosition;
    };
};

// per entity instance
struct EntityData {
    DynamicArray<EntityAction> actionQueue{};
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
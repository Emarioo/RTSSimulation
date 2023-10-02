#include "rtssim/Registry.h"

#include "rtssim/World.h"

static EntityStats s_entityStats[]{
    {{ 0.01f, 0.3f, 0.92f }, {0.4f, 0.8f, 0.4f }, 1.7f}, // worker
    {{ 0.1f, 0.1f, 0.7f }, { 0.6f, 0.7f, 0.6f }, 1.5f}, // soldier
};

EntityStats* Registries::getEntityStats(u32 entityType) {
    return s_entityStats + entityType;
}
glm::vec3 Registries::getTileColor(u32 tileType) {
    switch(tileType) {
        case TILE_GRASS: return { 0.01f, 0.9f, 0.07f };
        case TILE_STONE: return { 0.01f, 0.1f, 0.14f };
    }
    return {1.f,1.f,1.f};
}

u32 Registries::registerEntityData(EntityData** outEntityData) {
    EntityData empty{};
    u32 dataId = entityDatas.add(&empty,outEntityData);
    return dataId + 1;
}
void Registries::unregisterEntityData(u32 dataId) {
    entityDatas.get(dataId-1)->~EntityData();
    entityDatas.removeAt(dataId - 1);
}
EntityData* Registries::getEntityData(u32 dataId) {
    auto ptr = entityDatas.get(dataId - 1);
    return ptr;
}
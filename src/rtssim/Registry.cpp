#include "rtssim/Registry.h"

#include "rtssim/World.h"

static EntityStats s_entityStats[]{
    {{ 1,1,1}, {1,1,1 }, 1.0f}, // none
    {{ 0.01f, 0.3f, 0.92f }, {0.4f, 1.2f, 0.4f }, 2.4f}, // worker
    {{ 0.1f, 0.1f, 0.7f }, { 0.6f, 0.7f, 0.6f }, 1.5f}, // soldier
    {{ 0.8f, 0.5f, 0.3f }, { 3.f, 2.f, 3.f }, 1.5f}, // working hall
};

EntityStats* Registries::getEntityStats(u32 entityType) {
    return s_entityStats + entityType;
}
glm::vec3 ColorFromHex(const char* hex) {
    Assert(strlen(hex) == 6);
    #define HEXIFY(chr) ((u8)chr <= '9' ? (u8)chr - '0' : ((u8)chr&~(32)) + 10 - 'A')
    return {
        (HEXIFY(hex[0])*16 + HEXIFY(hex[1])) / 255.f,
        (HEXIFY(hex[2])*16 + HEXIFY(hex[3])) / 255.f,
        (HEXIFY(hex[4])*16 + HEXIFY(hex[5])) / 255.f,
    };
    #undef HEXIFY
}
glm::vec3 Registries::getTileColor(u32 tileType) {
    switch(tileType) {
        case TILE_WOOD: return ColorFromHex("633F30");
        case TILE_STONE: return ColorFromHex("70787A");
        case TILE_IRON: return ColorFromHex("BAA89B");
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
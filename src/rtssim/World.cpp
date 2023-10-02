#include "rtssim/World.h"

#include "Engone/Util/Tracker.h"
#include "Engone/Logger.h"

#include "rtssim/Registry.h"

void World::cleanup() {
    using namespace engone;
    log::out << log::YELLOW << "World cleanup not implemented\n";
}

World* World::CreateTest(Registries* registries) {
    World* world = TRACK_ALLOC(World);
    new(world)World();

    world->registries = registries;

    Chunk* chunk;
    u32 chunkIndex = world->chunks.add(nullptr,&chunk);

    chunk->x = 0;
    chunk->z = 0;
    int prevHeight = 0;
    for(int i=0;i<sizeof(chunk->tiles)/sizeof(*chunk->tiles);i++){
        Tile* tile = chunk->tiles + i;
        tile->tileType = TILE_GRASS;
        tile->height = prevHeight;

        prevHeight += rand()%3-1;
    }

    int workerCount = 4;
    for(int i=0;i<workerCount;i++){
        Entity* entity;
        u32 entityIndex = world->entities.add(nullptr, &entity);

        entity->entityType = ENTITY_WORKER;
        entity->x = i*1.5;
        entity->z = 1.5 * (i%5);
        EntityData* data;
        entity->extraData = world->registries->registerEntityData(&data);
    }

    return world;
}

u32 World::raycast(glm::vec3 rayPos, glm::vec3 rayDir, float maxRayDistance) {
    float shortestDistance = maxRayDistance;
    u32 entityIndex = -1;

    // TODO: Optimize, quad tree of entities? ignore entities you know you won't hit?

    BucketArray<Entity>::Iterator entityIterator{};
    while(entities.iterate(entityIterator)) {
        Entity* entity = entityIterator.ptr;

        glm::vec3 pos = {entity->x, 1, entity->z}; // TODO: Don't assume 1 as Y position!
        EntityStats* stats = registries->getEntityStats(entity->entityType);
        glm::vec3 size = stats->size;

        float radius = 0.5f;

        float distance = 0;
        bool hit = glm::intersectRaySphere(rayPos, glm::normalize(rayDir), pos + glm::vec3(1.f,1.f,1.f)/2.f, radius, distance);
        if(hit) {
            if(shortestDistance > distance) {
                entityIndex = entityIterator.index;
                shortestDistance = distance;
            }
        }
    }
    return entityIndex;
}

void World::Destroy(World* world) {
    world->~World();
    TRACK_FREE(world,World);
}
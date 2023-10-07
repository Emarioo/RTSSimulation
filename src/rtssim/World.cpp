#include "rtssim/World.h"

#include "Engone/Util/Tracker.h"
#include "Engone/Logger.h"

#include "rtssim/Registry.h"

#include "stb/stb_image.h"

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
World* World::CreateFromImage(Registries* registries, const char* path) {
    using namespace engone;
    World* world = TRACK_ALLOC(World);
    new(world)World();

    world->registries = registries;
    
    int img_width, img_height, channels;
    u8* img_data = stbi_load(path, &img_width, &img_height, &channels, 4);
    
    _LOG_WORLD_CREATION(log::out << log::LIME << "Creating world ("<<path<<", "<<img_width<<"x"<<img_height<<")\n")

    // how many chunks do we need?
    int totalTiles = img_width * img_height;
    int xChunks = ceilf((float)img_width / Chunk::TILES_PER_SIDE);
    int yChunks = ceilf((float)img_height / Chunk::TILES_PER_SIDE);
    int totalChunks = ceilf((float)img_width / Chunk::TILES_PER_SIDE) * ceilf((float)img_height / Chunk::TILES_PER_SIDE);
    
    _LOG_WORLD_CREATION(log::out << log::LIME << " Tiles: "<<totalTiles<<", Chunks: "<<totalChunks<<"\n")

    int world_offset_x = 0;
    int world_offset_y = 0;
    
    // int world_offset_x = img_width / 2;
    // int world_offset_y = img_height / 2;

    int prevHeight = 0;
    for(int chunk_y=0;chunk_y<yChunks;chunk_y++) {
        for(int chunk_x=0;chunk_x<xChunks;chunk_x++) {
            Chunk* chunk = nullptr;
            u32 chunkIndex = world->chunks.add(nullptr,&chunk);
            chunk->x = world_offset_x + chunk_x * Chunk::TILES_PER_SIDE;
            chunk->z = world_offset_y + chunk_y * Chunk::TILES_PER_SIDE;
            
            for(int y=0;y<Chunk::TILES_PER_SIDE;y++){
                for(int x=0;x<Chunk::TILES_PER_SIDE;x++){
                    // img_x is more accurate than tile_x
                    int tile_x = chunk_x * Chunk::TILES_PER_SIDE + x;
                    int tile_y = chunk_y * Chunk::TILES_PER_SIDE + y;
                    if(tile_x >= img_width || tile_y >= img_height)
                        continue;
                        
                    struct {
                        u8 b,g,r,a;
                    } rgba;
                    
                    if(channels == 4) {
                        rgba.b =  img_data[(tile_y * img_width + tile_x)*4 + 0];
                        rgba.g =  img_data[(tile_y * img_width + tile_x)*4 + 1];
                        rgba.r =  img_data[(tile_y * img_width + tile_x)*4 + 2];
                        rgba.a =  img_data[(tile_y * img_width + tile_x)*4 + 3];
                    } else {
                        rgba.b =  img_data[(tile_y * img_width + tile_x)*4 + 0];
                        rgba.g =  img_data[(tile_y * img_width + tile_x)*4 + 1];
                        rgba.r =  img_data[(tile_y * img_width + tile_x)*4 + 2];
                        rgba.a = 255;
                    }
                    
                    Tile* tile = chunk->tiles + y * Chunk::TILES_PER_SIDE + x;
                    tile->tileType = TILE_COLORED;
                    tile->green = rgba.g;
                    tile->red = rgba.r;
                    tile->blue = rgba.b;
                    
                    // log::out << " "<<tile_x<<"."<<tile_y<<": "<<rgba.r<<", "<<rgba.g<<" "<<rgba.b << " "<<rgba.a<<"\n";
                    int t = tile->green / 64;
                    tile->height = 255 - t * 64;
                    // tile->height = 255 - tile->green;
                    // tile->height = prevHeight;
                    // prevHeight += rand()%3-1;
                }
            }
        }
    }
    
    stbi_image_free(img_data);

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
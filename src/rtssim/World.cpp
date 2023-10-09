#include "rtssim/World.h"

#include "Engone/Util/Tracker.h"
#include "Engone/Logger.h"

#include "rtssim/Registry.h"

#include "stb/stb_image.h"

static const char* tile_names[TILE_TYPE_MAX] {
    "Empty",
    "Terrain",
    "Stone",
    "Wood",
    "Iron",
};
const char* ToString(TileType tileType) {
    return tile_names[tileType];
}
static const char* entity_names[ENTITY_TYPE_MAX] {
    "None",
    "Worker",
    "Soldier",
    "Training Hall",
};
const char* ToString(EntityType entityType) {
    return entity_names[entityType];
}   

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
        tile->tileType = TILE_TERRAIN;
        tile->green = 160 + rand()%50;
        tile->red = 10;
        tile->blue = 20;
        tile->height = prevHeight;

        prevHeight += rand()%3-1;
    }

    int workerCount = 4;
    for(int i=0;i<workerCount;i++){
        Entity* entity;
        u32 entityIndex = world->entities.add(nullptr, &entity);

        entity->entityType = ENTITY_WORKER;
        entity->pos.x = i*1.5;
        entity->pos.y = 0;
        entity->pos.z = 1.5 * (i%5);
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
    
    char path_heightMap[256];
    char path_colorMap[256];
    snprintf(path_heightMap,sizeof(path_heightMap), "%s-height.png", path);
    snprintf(path_colorMap,sizeof(path_colorMap), "%s-color.png", path);
    
    int img_width, img_height, channels;
    int img2_width, img2_height, channels2;
    u8* color_map = stbi_load(path_colorMap, &img_width, &img_height, &channels, 4);
    u8* height_map = stbi_load(path_heightMap, &img2_width, &img2_height, &channels2, 4);
    Assert(color_map && height_map);
    Assert(img_width == img2_width && img_height == img2_height);
    Assert((channels == 3 || channels == 4) && (channels2 == 3 || channels2 == 4));
    
    // how many chunks do we need?
    int totalTiles = img_width * img_height;
    int xChunks = ceilf((float)img_width / Chunk::TILES_PER_SIDE);
    int yChunks = ceilf((float)img_height / Chunk::TILES_PER_SIDE);
    int totalChunks = ceilf((float)img_width / Chunk::TILES_PER_SIDE) * ceilf((float)img_height / Chunk::TILES_PER_SIDE);
    
    _LOG_WORLD_CREATION(log::out << log::LIME << "Creating world ("<<path<<", "<<img_width<<"x"<<img_height<<")\n")
    _LOG_WORLD_CREATION(log::out << log::LIME << " Tiles: "<<totalTiles<<", Chunks: "<<totalChunks<<"\n")

    int world_offset_x = 0;
    int world_offset_y = 0;
    
    // int world_offset_x = img_width / 2;
    // int world_offset_y = img_height / 2;
    
    Tile* tile = nullptr;
    int tile_x = 0;
    int tile_y = 0;
    auto ResourceCircle = [&](int center_x, int center_y, int radius, float density, TileType tileType, int amount_min, int amount_max) {
        int dist = (center_x-tile_x) * (center_x-tile_x) + (center_y-tile_y) * (center_y-tile_y)/2;
        if(dist < radius*radius) {
            if((float)rand()/RAND_MAX < density) {
                tile->tileType = tileType;
                tile->amount = amount_min + ((float)rand()/RAND_MAX)*(amount_max - amount_min);
            }
        }
    };

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
                    tile_x = chunk_x * Chunk::TILES_PER_SIDE + x;
                    tile_y = chunk_y * Chunk::TILES_PER_SIDE + y;
                    if(tile_x >= img_width || tile_y >= img_height)
                        continue;
                        
                    struct {
                        u8 b,g,r,a;
                    } rgba;
                    
                    if(channels == 4) {
                        rgba.r =  color_map[(tile_y * img_width + tile_x)*4 + 0];
                        rgba.g =  color_map[(tile_y * img_width + tile_x)*4 + 1];
                        rgba.b =  color_map[(tile_y * img_width + tile_x)*4 + 2];
                        rgba.a =  color_map[(tile_y * img_width + tile_x)*4 + 3];
                    } else {
                        rgba.r =  color_map[(tile_y * img_width + tile_x)*4 + 0];
                        rgba.g =  color_map[(tile_y * img_width + tile_x)*4 + 1];
                        rgba.b =  color_map[(tile_y * img_width + tile_x)*4 + 2];
                        rgba.a = 255;
                    }
                    
                    tile = chunk->tiles + y * Chunk::TILES_PER_SIDE + x;
                    tile->tileType = TILE_TERRAIN;
                    
                    ResourceCircle(12,5,4,0.8,TILE_WOOD,4,6);
                    ResourceCircle(5,15,6,0.8,TILE_STONE,3,6);
                    ResourceCircle(20,10,9,0.6,TILE_WOOD,3,6);
                    ResourceCircle(13,30,10,0.8,TILE_STONE,3,6);
                    
                    if(tile->tileType != TILE_TERRAIN)
                        tile->occupied = true;
                    
                    tile->green = rgba.g;
                    tile->red = rgba.r;
                    tile->blue = rgba.b;
                    
                    if(channels2 == 4) {
                        tile->height = height_map[(tile_y * img_width + tile_x)*4 + 2]-128;
                    } else {
                        tile->height = height_map[(tile_y * img_width + tile_x)*4 + 2]-128;
                    }
                    
                    // log::out << " "<<tile_x<<"."<<tile_y<<": "<<rgba.r<<", "<<rgba.g<<" "<<rgba.b << " "<<rgba.a<<"\n";
                    // int t = tile->green / 64;
                    // tile->height = 255 - t * 64;
                    // tile->height = 255 - tile->green;
                    // tile->height = prevHeight;
                    // prevHeight += rand()%3-1;
                }
            }
        }
    }
    
    stbi_image_free(color_map);
    stbi_image_free(height_map);

    int workerCount = 4;
    for(int i=0;i<workerCount;i++){
        Entity* entity;
        u32 entityIndex = world->entities.add(nullptr, &entity);

        entity->entityType = ENTITY_WORKER;
        entity->pos.x = i*1.5;
        entity->pos.y = 0;
        entity->pos.z = 1.5 * (i%5);
        EntityData* data;
        entity->extraData = world->registries->registerEntityData(&data);
    }

    return world;
}

u32 World::raycast(glm::vec3 rayPos, glm::vec3 rayDir, float maxRayDistance) {
    float shortestDistance = 0.0f;
    u32 entityIndex = -1;

    // TODO: Optimize, quad tree of entities? ignore entities you know you won't hit?
    BucketArray<Entity>::Iterator entityIterator{};
    while(entities.iterate(entityIterator)) {
        Entity* entity = entityIterator.ptr;

        glm::vec3 pos = entity->pos;
        EntityStats* stats = registries->getEntityStats(entity->entityType);
        glm::vec3 size = stats->size;

        float radius = glm::length(size/2.f); // TODO: Calculate this once and store it per entity type.

        float distance = 0;
        bool hit = glm::intersectRaySphere(rayPos, glm::normalize(rayDir), pos + glm::vec3(1.f,1.f,1.f)/2.f, radius, distance);
        if(hit) {
            if(shortestDistance > distance || entityIndex == -1) {
                entityIndex = entityIterator.index;
                shortestDistance = distance;
            }
        }
    }
    Assert(entityIndex == -1 || shortestDistance < maxRayDistance); // ignoring max distance for now
    return entityIndex;
}
Tile* World::tileFromWorldPosition(float world_x, float world_z, int& grid_x, int& grid_z){
    // #define grid(x) ((x)<0 ? -ceilf(-(x) / 16) : (x) / 16 )
    // #define pri(x) printf("%f -> %d\n",x,(int)(grid(x)));
    
    grid_x = world_x / TILE_SIZE_IN_WORLD;
    grid_z = world_z / TILE_SIZE_IN_WORLD;
    
    int chunk_x = grid_x < 0 ? -ceilf((-grid_x) / (float)Chunk::TILES_PER_SIDE) : grid_x / (float)Chunk::TILES_PER_SIDE;
    int chunk_z = grid_z < 0 ? -ceilf((-grid_z) / (float)Chunk::TILES_PER_SIDE) : grid_z / (float)Chunk::TILES_PER_SIDE;
    chunk_x *= Chunk::TILES_PER_SIDE;
    chunk_z *= Chunk::TILES_PER_SIDE;
    
    int tile_x = grid_x % Chunk::TILES_PER_SIDE;
    if(tile_x < 0) tile_x += Chunk::TILES_PER_SIDE;
    int tile_z = grid_z % Chunk::TILES_PER_SIDE;
    if(tile_z < 0) tile_z += Chunk::TILES_PER_SIDE;
    
    BucketArray<Chunk>::Iterator iterator{};
    while(chunks.iterate(iterator)){
        Chunk* chunk = iterator.ptr;
        
        if(chunk->x != chunk_x || chunk->z != chunk_z) 
            continue;
            
        Tile* tile = chunk->tiles + tile_z * Chunk::TILES_PER_SIDE + tile_x;
        return tile;
    }
    return nullptr;
}
Tile* World::tileFromGridPosition(int grid_x, int grid_z){
    int chunk_x = grid_x < 0 ? -ceilf((-grid_x) / (float)Chunk::TILES_PER_SIDE) : grid_x / (float)Chunk::TILES_PER_SIDE;
    int chunk_z = grid_z < 0 ? -ceilf((-grid_z) / (float)Chunk::TILES_PER_SIDE) : grid_z / (float)Chunk::TILES_PER_SIDE;
    chunk_x *= Chunk::TILES_PER_SIDE;
    chunk_z *= Chunk::TILES_PER_SIDE;
    
    int tile_x = grid_x % Chunk::TILES_PER_SIDE;
    if(tile_x < 0) tile_x += Chunk::TILES_PER_SIDE;
    int tile_z = grid_z % Chunk::TILES_PER_SIDE;
    if(tile_z < 0) tile_z += Chunk::TILES_PER_SIDE;
    
    BucketArray<Chunk>::Iterator iterator{};
    while(chunks.iterate(iterator)){
        Chunk* chunk = iterator.ptr;
        
        if(chunk->x != chunk_x || chunk->z != chunk_z) 
            continue;
            
        Tile* tile = chunk->tiles + tile_z * Chunk::TILES_PER_SIDE + tile_x;
        return tile;
    }
    return nullptr;
}
        

void World::Destroy(World* world) {
    world->~World();
    TRACK_FREE(world,World);
}
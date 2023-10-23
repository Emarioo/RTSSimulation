#include "rtssim/World.h"

#include "Engone/Util/Tracker.h"
#include "Engone/Logger.h"

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

World* World::CreateTest() {
    World* world = TRACK_ALLOC(World);
    new(world)World();

    // world->registries = registries;

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
        entity->extraData = world->registries.registerEntityData(&data);
    }

    return world;
}
World* World::CreateFromImage(const char* path) {
    using namespace engone;
    World* world = TRACK_ALLOC(World);
    new(world)World();

    // world->registries = registries;
    
    char path_heightMap[256];
    char path_colorMap[256];
    char path_resourceMap[256];
    snprintf(path_heightMap,sizeof(path_heightMap), "%s-height.png", path);
    snprintf(path_colorMap,sizeof(path_colorMap), "%s-color.png", path);
    snprintf(path_resourceMap,sizeof(path_resourceMap), "%s-resource.png", path);
    
    int img_width, img_height, channels;
    int img2_width, img2_height, channels2;
    int img3_width, img3_height, channels3;
    u8* color_map = stbi_load(path_colorMap, &img_width, &img_height, &channels, 4);
    u8* height_map = stbi_load(path_heightMap, &img2_width, &img2_height, &channels2, 4);
    u8* resource_map = stbi_load(path_resourceMap, &img3_width, &img3_height, &channels3, 4);
    Assert(color_map && height_map);
    Assert(img_width == img2_width && img_height == img2_height && img_height == img3_height);
    Assert((channels == 3 || channels == 4) && (channels2 == 3 || channels2 == 4) && (channels3 == 3 || channels3 == 4));
    
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
                        
                    struct RGBA {
                        u8 b,g,r,a;
                    };
                    RGBA rgba;
                    
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
                    
                    RGBA resourceData;
                    if(channels3 == 4) {
                        resourceData.r = resource_map[(tile_y * img_width + tile_x)*4 + 0];
                        resourceData.g = resource_map[(tile_y * img_width + tile_x)*4 + 1];
                        resourceData.b = resource_map[(tile_y * img_width + tile_x)*4 + 2];
                        resourceData.a = resource_map[(tile_y * img_width + tile_x)*4 + 3];
                    } else if(channels3 == 3) {
                        resourceData.r = resource_map[(tile_y * img_width + tile_x)*4 + 0];
                        resourceData.g = resource_map[(tile_y * img_width + tile_x)*4 + 1];
                        resourceData.b = resource_map[(tile_y * img_width + tile_x)*4 + 2];
                        resourceData.a = 255;
                    }
                    // if(resourceData.r == 200 && resourceData.g == 100 && resourceData.b == 20) {
                    if(resourceData.r != 0) {
                        tile->tileType = TILE_WOOD;
                        tile->amount = 4;
                    }

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
        entity->extraData = world->registries.registerEntityData(&data);
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
        EntityStats* stats = registries.getEntityStats(entity->entityType);
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
Tile* World::tileFromWorldPosition(const glm::vec3& pos, int& grid_x, int& grid_z){
    // #define grid(x) ((x)<0 ? -ceilf(-(x) / 16) : (x) / 16 )
    // #define pri(x) printf("%f -> %d\n",x,(int)(grid(x)));
    gridPosFromWorld(pos, grid_x, grid_z);
    
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

struct FileWorldHeader {
    // TODO: magic number, version, checksum, 
    u32 sectionCount;

    // TODO: Divide the save file into sections. One section for entities, one for chunks, and also registries.
    //   The header has a sectionCount field. Each section begins with a type. Then the
};
struct FileWorldSectionHeader {
    enum SectionType : u32 {
        SECTION_ENTITIES=1,
        SECTION_CHUNKS=2,
        SECTION_ENTITY_DATA=3,
        SECTION_PATHFINDING=4,
    };
    SectionType type;
    u32 contentSize; // does not include the size of the section header
};
bool World::saveToFile(const char* path) {
    // TODO: Implement techniques for dealing with file corruption. Checksum, backups, autosave.

    auto file = engone::FileOpen(path, nullptr, engone::FILE_ALWAYS_CREATE);
    if(!file) {
        engone::log::out << engone::log::RED << "Failed saving world '"<<path<<"'\n";
        return false;
    }

    FileWorldHeader header{};
    header.sectionCount = 3; // entities, chunks, entity data
    engone::FileWrite(file, &header, sizeof(FileWorldHeader));

    // ENTITIES
    FileWorldSectionHeader sheader{};
    sheader.type = FileWorldSectionHeader::SECTION_ENTITIES;
    sheader.contentSize = sizeof(Entity) * entities.getCount();
    engone::FileWrite(file, &sheader, sizeof(FileWorldSectionHeader));

    BucketArray<Entity>::Iterator entityIterator{};
    while(entities.iterate(entityIterator)) {
        engone::FileWrite(file, &entityIterator.index, sizeof(int));
        engone::FileWrite(file, entityIterator.ptr, sizeof(Entity));
    }

    // CHUNKS
    sheader.type = FileWorldSectionHeader::SECTION_CHUNKS;
    sheader.contentSize = sizeof(Chunk) * chunks.getCount();
    engone::FileWrite(file, &sheader, sizeof(FileWorldSectionHeader));
    
    BucketArray<Chunk>::Iterator chunkIterator{};
    while(chunks.iterate(chunkIterator)) {
        engone::FileWrite(file, &chunkIterator.index, sizeof(int));
        engone::FileWrite(file, chunkIterator.ptr, sizeof(Chunk));
    }

    // ENTITY DATA
    sheader.type = FileWorldSectionHeader::SECTION_ENTITY_DATA;
    sheader.contentSize = sizeof(EntityData) * registries.entityDatas.getCount();
    engone::FileWrite(file, &sheader, sizeof(FileWorldSectionHeader));

    BucketArray<EntityData>::Iterator entityDataIterator{};
    while(registries.entityDatas.iterate(entityDataIterator)) {
        engone::FileWrite(file, &entityDataIterator.index, sizeof(int));
        Path* tmp = entityDataIterator.ptr->path;
        entityDataIterator.ptr->path = nullptr;
        engone::FileWrite(file, entityDataIterator.ptr, sizeof(Chunk));
        entityDataIterator.ptr->path = tmp;
    }

    //  TODO: Pathfinding
    // sheader.type = FileWorldSectionHeader::SECTION_PATHFINDING;
    // sheader.contentSize = sizeof(EntityData) * registries..getCount();
    // engone::FileWrite(file, &sheader, sizeof(FileWorldSectionHeader));

    // BucketArray<EntityData>::Iterator entityDataIterator{};
    // while(registries.entityDatas.iterate(entityDataIterator)) {
    //     engone::FileWrite(file, &entityDataIterator.index, sizeof(int));
    //     engone::FileWrite(file, entityDataIterator.ptr, sizeof(Chunk));
    // }

    engone::FileClose(file);
    return true;
}
bool World::loadFromFile(const char* path) {
    u64 fileSize;
    auto file = engone::FileOpen(path, &fileSize, engone::FILE_ONLY_READ);
    if(!file) {
        engone::log::out << engone::log::RED << "Failed loading world '"<<path<<"'\n";
        return false;
    }

    // TODO: Check remaining bytes in file before each read, or check for returned value of FileRead.
    //  The file may be corrupted and have less stuff than you expect.
    // TODO: Check for abnormally large values. A size or count of 5831295 makes no sense. The file is
    //  probably corrupt if you encounted it.

    // TODO: MEMORY LEAK! destructor is not called on the elements!
    entities.cleanup();
    chunks.cleanup();
    registries.entityDatas.cleanup(); // especially here

    FileWorldHeader header{};
    engone::FileRead(file, &header, sizeof(FileWorldHeader));

    for(int sectionIndex=0;sectionIndex<header.sectionCount;sectionIndex++) {
        FileWorldSectionHeader sheader{};
        engone::FileRead(file, &sheader, sizeof(FileWorldSectionHeader));

        switch(sheader.type) {
            case FileWorldSectionHeader::SECTION_ENTITIES: {
                u32 entityCount = sheader.contentSize / sizeof(Entity);
                int index;
                for(int i=0;i<entityCount;i++){
                    engone::FileRead(file, &index, sizeof(int));
                    Entity* entity;
                    bool yes = entities.requestSpot(index, nullptr, &entity);
                    Assert(yes);
                    engone::FileRead(file, entity, sizeof(Entity));
                }
                break;
            }
            case FileWorldSectionHeader::SECTION_CHUNKS: {
                u32 chunkCount = sheader.contentSize / sizeof(Chunk);
                int index;
                for(int i=0;i<chunkCount;i++){
                    engone::FileRead(file, &index, sizeof(int));
                    Chunk* chunk;
                    bool yes = chunks.requestSpot(index, nullptr, &chunk);
                    Assert(yes);
                    engone::FileRead(file, chunk, sizeof(Chunk));
                }
                break;
            }
            case FileWorldSectionHeader::SECTION_ENTITY_DATA: {
                u32 dataCount = sheader.contentSize / sizeof(EntityData);
                int index;
                for(int i=0;i<dataCount;i++){
                    engone::FileRead(file, &index, sizeof(int));
                    EntityData* entityData;
                    bool yes = registries.entityDatas.requestSpot(index, nullptr, &entityData);
                    Assert(yes);
                    engone::FileRead(file, entityData, sizeof(EntityData));
                }
                break;
            }
        }
    }

    engone::FileClose(file);
    return true;
}

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
#pragma once

#include "Engone/Util/Array.h"
#include "Engone/Camera.h"
#include "Engone/Rendering/Buffer.h"
#include "Engone/Util/Tracker.h"
#include "Engone/UIModule.h"
#include "Engone/InputModule.h"

#include "rtssim/World.h"
#include "rtssim/Registry.h"
#include "rtssim/Item.h"

#include "rtssim/Util/Perf.h"
#include "rtssim/Util/LinearAllocator.h"
#include "rtssim/Pathfinding.h"

#ifdef USE_HOT_RELOAD
#define GAME_API extern "C" __declspec(dllexport)
#else
#define GAME_API
#endif

struct Cube {
    glm::vec3 pos;
    glm::vec3 size;
};
struct Particle {
    glm::vec3 pos;
    glm::vec3 vel;
    glm::vec3 color;
    float size;
    float lifeTime;
};
struct TeamResources {
    // int wood = 10;
    // int stone = 10;
    // int iron = 10;
    DynamicArray<Item> resources;
    // int teamItems_amount[ITEM_TYPE_MAX];
    
    void addItem(ItemType type, int amount) {
        for(int i=0;i<resources.size();i++){
            if(type == resources[i].type) {
                resources[i].amount += amount;
                return;
            }
        }
        resources.add({type, amount});
        return;
    }
    int getAmount(ItemType type) {
        for(int i=0;i<resources.size();i++){
            if(type == resources[i].type)
                return resources[i].amount;
        }
        return 0;
    }
    void removeItem(ItemType type, int amount) {
        for(int i=0;i<resources.size();i++){
            if(type == resources[i].type) {
                resources[i].amount -= amount;
                if(resources[i].amount < 0)
                    resources[i].amount = 0;
                break;
            }
        }
    }
};
struct GameState;
typedef void (*GameProcedure)(GameState* gameState);
struct GameState {
    static GameState* Create();
    static void Destroy(GameState* gameState);

    // Application stuff
    engone::Thread updateThread{};
    // engone::Thread renderThread{};
    // engone::Mutex 
    
    GameProcedure activeUpdateProc = nullptr;
    GameProcedure activeRenderProc = nullptr;
    GameProcedure inactiveUpdateProc = nullptr;
    GameProcedure inactiveRenderProc = nullptr;
    
    GLFWwindow* window=nullptr;
    float winWidth=0, winHeight=0;
    
    bool locked_fps = true;
    
    bool isRunning = true;

    // int prev_winX = 0, prev_winY = 0, prev_winW = 0, prev_winH = 0;

    engone::InputModule inputModule{};

    // Rendering stuff
    struct CubeInstance {
        float x,y,z,sx,sy,sz,r,g,b;
    };
    static const int CUBE_BATCH_MAX = 10;
    engone::VertexArray cubeVA{};
    engone::VertexBuffer cubeVB{};
    engone::IndexBuffer cubeIB{};

    engone::VertexBuffer cubeInstanceVB{};

    u32 cubeInstanceBatch_max = 0;
    CubeInstance* cubeInstanceBatch = nullptr;
    DynamicArray<Cube> cubes{};
    int cubesToDraw = 0;

    engone::Shader cubeShader{};
    float fov = 90.f, zNear = 0.1f, zFar = 400.f;
    engone::Camera camera{};

    engone::UIModule uiModule{};
    
    // update code SHOULD NOT USE THIS ALLOCATOR!
    LinearAllocator stringAllocator_render{};

    double update_deltaTime = 1.0f/60.0f; // fixed
    double current_frameTime = 1.0f/60.0f;
    
    AvgValue<double> avg_frameTime{30};
    double game_runtime = 0;

    AvgValue<float> avg_updateTime{30};
    
    bool use_area_select = false;
    glm::vec3 area_select_start{};

    // Actual game
    Registries registries{};
    World* world = nullptr;

    float gravity_acc = 7.f;    
    DynamicArray<Particle> particles{}; // TODO: Optimize with a bucket array and a stack array to keep track of empty spots.
    void addParticle(const Particle& particle);

    DynamicArray<u32> selectedEntities{};
    
    EntityType blueprintType = ENTITY_NONE;
    
    TeamResources teamResources{};
    Pathfinder pathfinder{};
    
    static const glm::vec3 MSG_COLOR_RED;
    bool hasSufficientResources(EntityType entityType, bool useResources = false, bool logMissingResources = false);
    bool refundResources(EntityType entityType);
    
    struct Message {
        std::string log{};
        float lifetime;
        glm::vec3 color;
    };
    DynamicArray<Message> messages{};
    void addMessage(const std::string& text, float time, const glm::vec3& color = {1.f,1.f,1.f});
    
    struct EntityCost {
        int amounts[ITEM_TYPE_MAX]{0};
    };
    EntityCost entityCosts[ENTITY_TYPE_MAX]{};
    
    struct TrainingEntry {
        EntityType type;
        glm::vec3 target_position;
        float remaining_time;
        u32 responsible_entityIndex; // id/index of training hall/building
    };
    DynamicArray<TrainingEntry> trainingQueue{};
    
    bool trainUnit(EntityType entityType, const glm::vec3& target);
    
    DynamicArray<u32> training_halls{}; // entity index
};

GAME_API void RenderGame(GameState* gameState);
GAME_API void UpdateGame(GameState* gameState);

void StartGame();
void FirstPersonProc(GameState* gameState);

void RenderWorld(GameState* gameState, World* world);
#include "rtssim/Game.h"

#include "rtssim/CubeShader.glsl"
#include "rtssim/UI/BaseUI.h"

#include "Engone/Utilities/TimeUtility.h"

#define RANDOMF(MIN,MAX) ((float)rand()/RAND_MAX * ((MAX)-(MIN)) + (MIN))

const glm::vec3 GameState::MSG_COLOR_RED = {1.0f,0.5f,0.5f};

// TODO: Move function elsewhere
ItemType TileToItem(TileType type) {
    switch(type) {
        case TILE_WOOD: return ITEM_WOOD;
        case TILE_STONE: return ITEM_STONE;
        case TILE_IRON: return ITEM_IRON;
    }
    return ITEM_NONE;
}

GameState* GameState::Create(){
    GameState* state = TRACK_ALLOC(GameState);
    new(state)GameState();
    state->stringAllocator_render.init(0x10000);
    
    return state;
}
void GameState::Destroy(GameState* state){
    state->~GameState();
    TRACK_FREE(state,GameState);
}
void CreateRayOfCubes(GameState* gameState, glm::vec3 pos, glm::vec3 dir, int cubeCount){
    for(int i=0;i<cubeCount;i++){
        float distance = i * 0.7f + i*i*0.03f;
        glm::vec3 size = {0.1f,0.1f,0.1f};
        gameState->cubes.add({pos- size / 2.f + dir * distance, size});
    }
}
void SetupRendering(GameState* gameState){
    u32 cubeIndices[] {
        0, 2, 4,  2, 6, 4,
        1, 5, 3,  5, 7, 3,

        0, 4, 1,  4, 5, 1,
        2, 3, 6,  3, 7, 6,

        0, 1, 2,  1, 3, 2,
        4, 6, 5,  6, 7, 5,
    };
    float cubeVertices[] {
        0, 0, 0, 1, 1, 1,
        1, 0, 0, 0,-1, 0,
        0, 1, 0, 0, 0,-1,
        1, 1, 0, 1, 0, 0,
        0, 0, 1,-1, 0, 0,
        1, 0, 1, 0, 0, 1,
        0, 1, 1, 0, 1, 0,
        1, 1, 1, 1, 1, 1,
    };
    gameState->cubeIB.setData(sizeof(cubeIndices), cubeIndices);

    gameState->cubeVB.setData(sizeof(cubeVertices), cubeVertices);

    gameState->cubeInstanceBatch_max = GameState::CUBE_BATCH_MAX;
    gameState->cubeInstanceBatch = new GameState::CubeInstance[gameState->cubeInstanceBatch_max];
    gameState->cubeInstanceVB.setData(GameState::CUBE_BATCH_MAX * sizeof(GameState::CubeInstance), nullptr);

    gameState->cubeVA.addAttribute(3); // pos
    gameState->cubeVA.addAttribute(3, &gameState->cubeVB); // normal
    gameState->cubeVA.addAttribute(3, 1); // pos
    gameState->cubeVA.addAttribute(3, 1); // size
    gameState->cubeVA.addAttribute(3, 1, &gameState->cubeInstanceVB); // color

    gameState->cubeShader.init(vs_CubeShaderGLSL);
}
u32 UpdateThread(void*);
u32 RenderThread(void*);
void StartGame() {
    using namespace engone;
    
    GameState* gameState = GameState::Create();
    // gameState->world = World::CreateTest(&gameState->registries);
    gameState->world = World::CreateFromImage(&gameState->registries, "assets/world/base-0");
    gameState->camera.setPosition(2,4,5);
    gameState->camera.setRotation(0.3f,-1.57f,0);
    
    gameState->teamResources.resources.add({ITEM_WOOD, 10});
    gameState->teamResources.resources.add({ITEM_STONE, 10});
    gameState->teamResources.resources.add({ITEM_IRON, 10});
    
    #define COST(E,I,A) gameState->entityCosts[E].amounts[I] = A;
    COST(ENTITY_TRAINING_HALL,ITEM_WOOD,5)
    COST(ENTITY_TRAINING_HALL,ITEM_STONE,3)
    COST(ENTITY_WORKER,ITEM_STONE,1)
    #undef COST
    
    // gameState->updateThread.init(UpdateThread, gameState);
    
    RenderThread(gameState);
    
    // gameState->updateThread.join();
    GameState::Destroy(gameState);
}
u32 UpdateThread(void * arg) {
    using namespace engone;
    GameState* gameState = (GameState*)arg;
    
    auto lastTime = engone::StartMeasure();
    auto gameStartTime = engone::StartMeasure();
    double updateAccumulation = 0;
    double sec_timer = 0;
    double reloadTime = 0;
    while (gameState->isRunning) {
        auto now = engone::StartMeasure();
        double frame_deltaTime = DiffMeasure(now - lastTime);
        lastTime = now;
        
        if(gameState->activeUpdateProc != gameState->inactiveUpdateProc) {
            gameState->activeUpdateProc = gameState->inactiveUpdateProc;
        }
        
        updateAccumulation += frame_deltaTime;
        if(gameState->activeUpdateProc) {
            while(updateAccumulation > gameState->update_deltaTime){
                updateAccumulation -= gameState->update_deltaTime;
                gameState->activeUpdateProc(gameState);
            }
        }
        // gameState->inputModule.resetEvents(true);
        // gameState->inputModule.resetPollChar();
        // gameState->inputModule.m_lastMouseX = gameState->inputModule.m_mouseX;
        // gameState->inputModule.m_lastMouseY = gameState->inputModule.m_mouseY;
        if(updateAccumulation + 0.002 < gameState->update_deltaTime) {
            // TODO: Is there a bug here somewhere?
            auto sleep_start = engone::StartMeasure();
            engone::Sleep(0.001);
            // log::out << "Sleep: " << engone::StopMeasure(sleep_start)*1000<<", dt:" << (gameState->update_deltaTime*1000)<<", acc: "<<(updateAccumulation*1000)<<" \n";
        }
    }
    
    return 0;
}
u32 RenderThread(void* arg) {
    using namespace engone;
    GameState* gameState = (GameState*)arg;
    
    Assert(glfwInit());
 
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
 
    GLFWwindow* window = glfwCreateWindow(800, 600, "RTS Simulation", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return 0;
    }
 
    glfwMakeContextCurrent(window);
    glewInit();

    if(gameState->locked_fps) 
        glfwSwapInterval(1); // limit fps to your monitors frequency?
    else
        glfwSwapInterval(0);
        
    gameState->window = window;

    SetupRendering(gameState);

    // gameState->cubes.add({{0,0,0},{1,1,1}});
    // gameState->cubes.add({{0,-1.5,0},{0.7,0.5,1}});
    // gameState->cubes.add({{0.3,0,0},{0.1,0.1,0.1}});

    #define HOT_RELOAD_ORIGIN "bin/game.dll"
    #define HOT_RELOAD_IN_USE "bin/hotloaded.dll"
    #define HOT_RELOAD_ORIGIN_PDB "bin/game.pdb"
    #define HOT_RELOAD_IN_USE_PDB "bin/hotloaded.pdb"
    #define HOT_RELOAD_TIME 2

    gameState->activeUpdateProc = UpdateGame;
    gameState->activeRenderProc = RenderGame;
    gameState->inactiveUpdateProc = UpdateGame;
    gameState->inactiveRenderProc = RenderGame;

    void* prev_hot_reload_dll = nullptr;
    void* hot_reload_dll = nullptr;
    double last_dll_write = 0;
 
    auto lastTime = engone::StartMeasure();

    // glEnable(GL_BLEND);
    // glEnable(GL_CULL_FACE);
    // glEnable(GL_DEPTH_TEST);
    // glCullFace(GL_FRONT);

    gameState->uiModule.init();
    gameState->inputModule.init(gameState->window);

    auto gameStartTime = engone::StartMeasure();
    double updateAccumulation = 0;
    double sec_timer = 0;
    double reloadTime = 0;
    while (!glfwWindowShouldClose(window)) {    
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        gameState->winWidth = width;
        gameState->winHeight = height;
    
        auto now = engone::StartMeasure();
        double frame_deltaTime = DiffMeasure(now - lastTime);
        lastTime = now;
        gameState->current_frameTime = frame_deltaTime;
        
        gameState->avg_frameTime.addSample(frame_deltaTime);

        gameState->game_runtime = DiffMeasure(now - gameStartTime);

        glViewport(0, 0, width, height);
        glClearColor(0.33,0.2,0.33,1);
        glClear(GL_COLOR_BUFFER_BIT);

        #ifdef USE_HOT_RELOAD
        reloadTime -= frame_deltaTime;
        if(reloadTime <= 0) {
            reloadTime += HOT_RELOAD_TIME;
            // log::out << "Try reload\n";
            double new_dll_write = 0;
            bool yes = engone::FileLastWriteSeconds(HOT_RELOAD_ORIGIN, &new_dll_write);
            if(yes && new_dll_write > last_dll_write) {
                log::out << "Reloaded\n";
                last_dll_write = new_dll_write;
                if(hot_reload_dll) {
                    Assert(!prev_hot_reload_dll);
                    prev_hot_reload_dll = hot_reload_dll;
                    hot_reload_dll = nullptr;
                }
                // TODO: Is 256 enough?
                char dll_path[256]{0};
                char pdb_path[256]{0};
                snprintf(dll_path,sizeof(dll_path),"bin/hotloaded-%d.dll", (int)rand());
                snprintf(pdb_path,sizeof(pdb_path),"bin/hotloaded-%d.pdb", (int)rand());
                engone::FileCopy(HOT_RELOAD_ORIGIN, dll_path);
                engone::FileCopy(HOT_RELOAD_ORIGIN_PDB, pdb_path);
                // engone::FileCopy(HOT_RELOAD_ORIGIN, HOT_RELOAD_IN_USE);
                // engone::FileCopy(HOT_RELOAD_ORIGIN_PDB, HOT_RELOAD_IN_USE_PDB);
                hot_reload_dll = engone::LoadDynamicLibrary(dll_path);
                // hot_reload_dll = engone::LoadDynamicLibrary(HOT_RELOAD_IN_USE);

                gameState->inactiveUpdateProc = (GameProcedure)engone::GetFunctionPointer(hot_reload_dll, "UpdateGame");
                gameState->inactiveRenderProc = (GameProcedure)engone::GetFunctionPointer(hot_reload_dll, "RenderGame");
                gameState->activeRenderProc = gameState->inactiveRenderProc;
            }
        }
        // TODO: Mutex on game proc
        if(gameState->activeUpdateProc == gameState->inactiveUpdateProc && prev_hot_reload_dll) {
            engone::UnloadDynamicLibrary(prev_hot_reload_dll);
            prev_hot_reload_dll = nullptr;
        }
        #endif
        if(gameState->activeUpdateProc != gameState->inactiveUpdateProc) {
            gameState->activeUpdateProc = gameState->inactiveUpdateProc;
        }
        updateAccumulation += frame_deltaTime;
        if(gameState->activeUpdateProc){
            while(updateAccumulation>gameState->update_deltaTime){
                updateAccumulation-=gameState->update_deltaTime;
                gameState->activeUpdateProc(gameState);
            }
        }
        if(gameState->activeRenderProc) {
            gameState->activeRenderProc(gameState);
        }

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        gameState->uiModule.render(gameState->winWidth, gameState->winHeight);

        gameState->inputModule.resetEvents(true);
        gameState->inputModule.resetPollChar();
        gameState->inputModule.m_lastMouseX = gameState->inputModule.m_mouseX;
        gameState->inputModule.m_lastMouseY = gameState->inputModule.m_mouseY;
 
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    gameState->isRunning = false;
 
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}
void CreateParticleExplosion(GameState* gameState, int gridx, int gridy, Tile* tile) {
    glm::vec3 centerPos = {gridx * TILE_SIZE_IN_WORLD, tile->height/16.f, gridy*TILE_SIZE_IN_WORLD };
    for(int i=0;i<256;i++){
        Particle part{};
        part.pos = centerPos + glm::vec3(0.5,0.5,0.5) + glm::vec3(RANDOMF(-0.45,0.45), RANDOMF(-0.45,0.45), RANDOMF(-0.45,0.45));
        part.vel = glm::vec3(RANDOMF(-1.3,1.3), RANDOMF(0.5f,2.f), RANDOMF(-1.3,1.3));
        part.color = gameState->registries.getTileColor(tile->tileType);
        part.color.x += RANDOMF(-0.035,0.035);
        part.color.y += RANDOMF(-0.02,0.02);
        part.color.z += RANDOMF(-0.03,0.03);
        part.size = RANDOMF(0.1,0.2f);
        part.lifeTime = RANDOMF(0.7f,1.0f);
        gameState->addParticle(part);
    }
}
GAME_API void UpdateGame(GameState* gameState) {
    auto update_start = engone::StartMeasure();
    
    // TODO: If a training hall is removed while a unit is being trained, we must give the responsibility of training the unit
    //  to another training hall.
    // TODO: What do we do when there are no training halls. We should probably send a message when we notice
    //  that there are zero training halls if the training queue isn't empty. We don't want to spam messages though.
    
    // First loop will decrease time and potentially add entities and free training halls
    for(int i = 0;i < gameState->trainingQueue.size();i++) {
        GameState::TrainingEntry& entry = gameState->trainingQueue[i];
        if(entry.responsible_entityIndex != -1) {
            entry.remaining_time -= gameState->update_deltaTime;
            if(entry.remaining_time < 0) {
                Entity* entity;
                u32 entityIndex = gameState->world->entities.add(nullptr, &entity);
                entity->entityType = entry.type;
                
                Entity* training_entity = gameState->world->entities.get(entry.responsible_entityIndex);
                EntityData* training_data = gameState->registries.getEntityData(training_entity->extraData);
                training_data->busyTraining = false;
                
                entity->pos = training_entity->pos;
                entity->pos.x = floorf((entity->pos.x) / TILE_SIZE_IN_WORLD);
                entity->pos.y = floorf((entity->pos.y) / TILE_SIZE_IN_WORLD);
                entity->pos.z = floorf((entity->pos.z) / TILE_SIZE_IN_WORLD);
                
                if(entity->entityType == ENTITY_WORKER || entity->entityType == ENTITY_SOLDIER) {
                    EntityData* data;
                    entity->extraData = gameState->world->registries->registerEntityData(&data);
                    
                    EntityAction action{};
                    action.actionType = EntityAction::ACTION_MOVE;
                    action.targetPosition = entry.target_position;
                    action.targetPosition.x = floorf((action.targetPosition.x) / TILE_SIZE_IN_WORLD);
                    action.targetPosition.y = floorf((action.targetPosition.y) / TILE_SIZE_IN_WORLD);
                    action.targetPosition.z = floorf((action.targetPosition.z) / TILE_SIZE_IN_WORLD);
                    data->actionQueue.add(action);
                }
                
                gameState->trainingQueue.removeAt(i);
                i--;
            }
        }
    }
    // Second loop will train new entities in the training halls that weren't busy before BUT more importantly, the ones who were just freed by the first loop.
    //  Without the second loop, we may end up with non-busy training halls and units that can't be trained because there were no available halls even though we do at the end of the loop.
    for(int i = 0;i < gameState->trainingQueue.size();i++) {
        GameState::TrainingEntry& entry = gameState->trainingQueue[i];
        if(entry.responsible_entityIndex == -1) {
            // find training hall to train in
            float min_distance = -1;
            u32 min_entityIndex = -1;
            bool found=false;
            EntityData* min_data = nullptr;
            for(u32 entityIndex : gameState->training_halls){
                Entity* entity = gameState->world->entities.get(entityIndex);
                Assert(entity && entity->entityType == ENTITY_TRAINING_HALL);
                
                EntityData* data = gameState->world->registries->getEntityData(entity->extraData);
                if(data->busyTraining)
                    continue;
                float dist = glm::length(entry.target_position - entity->pos);
                if(dist < min_distance || min_entityIndex == -1) {
                    min_distance = dist;
                    min_entityIndex = entityIndex;
                    min_data = data;
                }
            }
            if(min_entityIndex != -1) {
                entry.responsible_entityIndex = min_entityIndex;
                min_data->busyTraining = true;
            }
        }
    }
    
    BucketArray<Entity>::Iterator iterator{};
    while(gameState->world->entities.iterate(iterator)) {
        Entity* entity = iterator.ptr;
        
        glm::vec3 pos = entity->pos;

        EntityStats* stats = gameState->registries.getEntityStats(entity->entityType);
        EntityData* data = gameState->registries.getEntityData(entity->extraData);

        if(!data) continue;

        bool completedAction = false;
        if(data->actionQueue.size()!=0){
            auto& action = data->actionQueue[0];
            switch(action.actionType) {
                case EntityAction::ACTION_MOVE: {
                    glm::vec3 target = action.targetPosition;

                    // pathfind here to some other place
                    Path* path = gameState->pathfinder.createPath_test(gameState->world, pos, action.targetPosition);

                    if(path && path->points.size()>0) {
                        target = path->points[0];
                    }

                    pos.y = target.y;
                    float length = glm::length(target - pos);
                    float moveLength = stats->moveSpeed * gameState->update_deltaTime;
                    glm::vec3 moveDir = glm::normalize(target - pos) * moveLength;
                    if(length < moveLength) {
                        completedAction = true;
                        entity->pos = action.targetPosition + glm::vec3(0,entity->pos.y - target.y,0);
                    } else {
                        entity->pos += moveDir;
                    }
                    gameState->pathfinder.destroyPath(path);
                    break;
                }
                case EntityAction::ACTION_GATHER: {
                    pos.y = action.targetPosition.y;
                    float length = glm::length(action.targetPosition - pos);
                    float moveLength = stats->moveSpeed * gameState->update_deltaTime;
                    glm::vec3 moveDir = glm::normalize(action.targetPosition - pos) * moveLength;
                    if(length < moveLength) {
                        // completedAction = true;
                        entity->pos = action.targetPosition + glm::vec3(0,entity->pos.y - action.targetPosition.y,0);
                        if(data->gathering)
                            data->gatherTime += gameState->update_deltaTime;
                            
                        float gatherTime = 0.2f;
                        if(data->gatherTime > gatherTime) {
                            data->gatherTime -= gatherTime;
                            Tile* tile = gameState->world->tileFromGridPosition(action.grid_x, action.grid_z);
                            if(tile && tile->tileType == action.gatherMaterial) {
                                ItemType itemType = TileToItem(action.gatherMaterial);
                                gameState->teamResources.addItem(itemType, 1);
                                
                                tile->amount--;
                                if(tile->amount == 0){
                                    gameState->addMessage("Block broken at {"+std::to_string(action.grid_x) + ", "+std::to_string(action.grid_z)+"}",5.f);
                                    CreateParticleExplosion(gameState,action.grid_x,action.grid_z,tile);
                                    tile->tileType = TILE_TERRAIN;
                                    
                                    completedAction = true;
                                }
                            } else {
                                completedAction = true;
                            }
                            if(completedAction) {
                                action.actionType = EntityAction::ACTION_SEARCH_GATHER;
                                completedAction = false;
                                action.hasTarget = false;
                            }
                            // When gathered a resource, the worker should perhaps drop it of at a storage building or on the ground
                            // before continuing mining. Right now, the resource is magically available to the player.
                            // data->dropResource = true;
                        } else {
                            data->gathering = true;
                            Particle part{};
                            
                            if(rand() % 4 == 0) {
                                part.pos = action.targetPosition + glm::vec3(0.5,0.5,0.5) + glm::vec3(RANDOMF(-0.2,0.2), RANDOMF(-0.2,0.2), RANDOMF(-0.2,0.2));
                                part.vel = glm::vec3(RANDOMF(-1.3,1.3), RANDOMF(3.f,5.f), RANDOMF(-1.3,1.3));
                                part.color = gameState->registries.getTileColor(action.gatherMaterial);
                                part.color.x += RANDOMF(-0.035,0.035);
                                part.color.y += RANDOMF(-0.02,0.02);
                                part.color.z += RANDOMF(-0.03,0.03);
                                part.size = RANDOMF(0.03,0.13f);
                                part.lifeTime = RANDOMF(0.7f,1.5f);
                                gameState->addParticle(part);
                            }
                        }
                    } else {
                        data->gathering = false;
                        entity->pos += moveDir;
                    }
                    break;
                }
                case EntityAction::ACTION_SEARCH_GATHER: {
                    if(!action.hasTarget) {
                        // find target
                        
                        // TODO: Optimize
                        struct Pos {
                            int x,y;  
                        };
                        DynamicArray<Pos> list{}; // TODO: Don't allocate memory like this
                        auto AddLayer = [&](int layer){
                            int short_edge = 1 + (layer-1) * 2;
                            int long_edge = 1 + layer * 2;
                            for(int i=0;i<long_edge;i++) {
                                list.add({-layer,i-layer});
                                list.add({layer,i-layer});
                            }
                            for(int i=0;i<short_edge;i++) {
                                list.add({i - (layer-1),-layer});
                                list.add({i - (layer-1),layer});
                            }
                        };
                            
                        int layer = 1;
                        AddLayer(layer++);
                        int search_range = 3; // 1: one layer, 2: two layers (5x5 area)
                        
                        Tile* found_tile = nullptr;
                        while(list.size()>0) {
                            int i = RANDOMF(0,list.size()-1);
                            int grid_x = list[i].x;
                            int grid_z = list[i].y;
                            grid_x += action.grid_x;
                            grid_z += action.grid_z;
                            
                            Tile* tile = gameState->world->tileFromGridPosition(grid_x, grid_z);
                            if(tile && tile->tileType == action.gatherMaterial) {
                                found_tile = tile;
                                action.grid_x = grid_x;
                                action.grid_z = grid_z;
                                action.targetPosition = glm::vec3(grid_x * TILE_SIZE_IN_WORLD,0, grid_z * TILE_SIZE_IN_WORLD);
                                action.hasTarget = true;
                                break;
                            }
                            list.removeAt(i);
                            if(list.size() == 0 && layer <= search_range) {
                                AddLayer(layer++);
                            }
                        }
                        if(!found_tile) {
                            completedAction = true;
                        }
                    } else {
                        // move to target
                        pos.y = action.targetPosition.y;
                        float length = glm::length(action.targetPosition - pos);
                        float moveLength = stats->moveSpeed * gameState->update_deltaTime;
                        glm::vec3 moveDir = glm::normalize(action.targetPosition - pos) * moveLength;
                        if(length < moveLength) {
                            // completedAction = true;
                            entity->pos = action.targetPosition + glm::vec3(0,entity->pos.y - action.targetPosition.y,0);
                            action.actionType = EntityAction::ACTION_GATHER;
                        } else {
                            data->gathering = false;
                            entity->pos += moveDir;
                        }
                    }
                    break;
                }
            }
        }
        if(completedAction) {
            data->actionQueue.removeAt(0);
        }
    }
    for(int i=0;i<gameState->particles.size();i++) {
        auto& particle = gameState->particles[i];
        particle.lifeTime -= gameState->update_deltaTime;
        if(particle.lifeTime < 0) {
            gameState->particles.removeAt(i); // TODO: Optimize, bucket array?
            i--;
            continue;
        }
        
        particle.vel.y -= gameState->gravity_acc * gameState->update_deltaTime * 0.5f;
        particle.pos += particle.vel * (float)gameState->update_deltaTime;
        particle.vel.y -= gameState->gravity_acc * gameState->update_deltaTime * 0.5f;
    }
    double update_time = engone::StopMeasure(update_start);
    // engone::log::out << "up time: "<<(update_time * 1000 * 1000) << "\n";
    gameState->avg_updateTime.addSample(update_time);
}
void DrawCube(GameState* gameState, const GameState::CubeInstance& cube, bool forceDraw = false) {
    gameState->cubeInstanceBatch[gameState->cubesToDraw] = cube;
    gameState->cubesToDraw++;
    if(gameState->cubesToDraw == gameState->cubeInstanceBatch_max || forceDraw) {
        gameState->cubeInstanceVB.setData(gameState->cubesToDraw * sizeof(GameState::CubeInstance), gameState->cubeInstanceBatch);
        gameState->cubeVA.bind();
        gameState->cubeVA.selectBuffer(2, &gameState->cubeInstanceVB);
        gameState->cubeVA.draw(&gameState->cubeIB,gameState->cubesToDraw);
        gameState->cubesToDraw = 0;
    }
}
void FlushCubeBatch(GameState* gameState) {
    if(gameState->cubesToDraw > 0) {
        gameState->cubeInstanceVB.setData(gameState->cubesToDraw * sizeof(GameState::CubeInstance), gameState->cubeInstanceBatch);
        gameState->cubeVA.bind();
        gameState->cubeVA.selectBuffer(2, &gameState->cubeInstanceVB);
        gameState->cubeVA.draw(&gameState->cubeIB,gameState->cubesToDraw);
        gameState->cubesToDraw = 0;
    }
}
GAME_API void RenderGame(GameState* gameState) {
    using namespace engone;
    
    gameState->stringAllocator_render.reset();
        
    if(gameState->inputModule.isKeyPressed(GLFW_KEY_L)) {
        gameState->locked_fps = !gameState->locked_fps;
        if(gameState->locked_fps) 
            glfwSwapInterval(1); // limit fps to your monitors frequency?
        else
            glfwSwapInterval(0);
    }
    
    gameState->cubesToDraw = 0;
    
    // glClearColor(1,0.5,0.33,1);
    glClearColor(0.1,0.5,0.33,1);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    if(gameState->inputModule.isKeyPressed(GLFW_KEY_ESCAPE)) {
        if(gameState->inputModule.isCursorLocked()) {
            gameState->inputModule.setCursorVisible(true);
            gameState->inputModule.lockCursor(false);
        } else {
            gameState->inputModule.setCursorVisible(false);
            gameState->inputModule.lockCursor(true);
        }    
    }
    if(gameState->inputModule.isCursorLocked()) {
        FirstPersonProc(gameState);
    }

    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    float speed = 7.f * gameState->current_frameTime;
    glm::vec3 lookDir = gameState->camera.getFlatLookVector();
    glm::vec3 strafeDir = gameState->camera.getRightVector();
    glm::vec3 moveDir = {0,0,0};
    if(gameState->inputModule.isKeyDown(GLFW_KEY_LEFT_SHIFT)){
        speed *= 4;
    }
    if(gameState->inputModule.isKeyDown(GLFW_KEY_W)){
        moveDir.z += 1;
    }
    if(gameState->inputModule.isKeyDown(GLFW_KEY_S)){
        moveDir.z += -1;
    }
    if(gameState->inputModule.isKeyDown(GLFW_KEY_A)){
        moveDir.x += -1;
    }
    if(gameState->inputModule.isKeyDown(GLFW_KEY_D)){
        moveDir.x += 1;
    }
    if(gameState->inputModule.isKeyDown(GLFW_KEY_SPACE)){
        moveDir.y += 1;
    }
    if(gameState->inputModule.isKeyDown(GLFW_KEY_LEFT_CONTROL)){
        moveDir.y += -1;
    }
    if(gameState->inputModule.isKeyPressed(GLFW_KEY_R)) {
        gameState->cubes.resize(0);
    }
    if(gameState->inputModule.isKeyPressed(GLFW_KEY_F11)) {
        int maximized = glfwGetWindowAttrib(gameState->window, GLFW_MAXIMIZED);
        if(maximized) {
            glfwRestoreWindow(gameState->window);
        } else {
            glfwMaximizeWindow(gameState->window);
        }
    }

    gameState->camera.setPosition(gameState->camera.getPosition() + lookDir * moveDir.z * speed + strafeDir * moveDir.x * speed + glm::vec3(0,moveDir.y * speed,0));

    // gameState->camera.setRX(gameState->camera.getRotation().x + 0.03f * gameState->current_frameTime);

    auto& ui = gameState->uiModule;
    int layout_y = 3;
    #define UI_TEXT(X,Y,H, R,G,B,A, STR) { auto text = ui.makeText(); text->x = X; text->y = Y; text->h = H; text->color = {R,G,B,A}; text->string = STR; text->length = strlen(STR); }
    static char str_fps[100];
    snprintf(str_fps, sizeof(str_fps), "%.2f ms (%d FPS)", (float)(gameState->avg_frameTime.getAvg() * 1000), (int)(1.0/gameState->avg_frameTime.getAvg()));
    UI_TEXT(5,layout_y,18, 1,1,1,1, str_fps)
    layout_y += 18;
    
    std::string string = engone::FormatTime(gameState->avg_updateTime.getAvg(), true, FormatTimeMS | FormatTimeUS | FormatTimeNS);
    static char str_upt[200];
    snprintf(str_upt, sizeof(str_upt), "update: %s", string.c_str());
    UI_TEXT(5,layout_y,18, 1,1,1,1, str_upt)
    layout_y += 18;
    
    static char str_camera[200];
    snprintf(str_camera, sizeof(str_camera), "camrot: %.2f, %.2f, %.2f", (float)(gameState->camera.getRotation().x),gameState->camera.getRotation().y,gameState->camera.getRotation().z);
    UI_TEXT(5,layout_y,18, 1,1,1,1, str_camera)
    layout_y += 18;
    
    static char str_entities[200];
    snprintf(str_entities, sizeof(str_entities), "Entities: %d, Particles: %d", gameState->world->entities.getCount(), gameState->particles.size());
    UI_TEXT(5,layout_y,18, 1,1,1,1, str_entities)
    layout_y += 18;
    
    float ratio = gameState->winWidth / gameState->winHeight;
    glm::mat4 perspectiveMatrix = glm::mat4(1);
    if (std::isfinite(ratio))
        perspectiveMatrix = glm::perspective(glm::radians(gameState->fov), ratio, gameState->zNear, gameState->zFar);

    glm::mat4 viewMatrix = gameState->camera.getViewMatrix();
    
    glm::vec3 hoveredWorldPosition = {0,0,0};
    glm::vec3 mouseDirection = {0,0,0};
    if(gameState->inputModule.isCursorLocked()) {
        mouseDirection = gameState->camera.getLookVector();
    } else {
        mouseDirection = gameState->camera.directionFromMouse(gameState->inputModule.getMouseX(), gameState->inputModule.getMouseY(), gameState->winWidth, gameState->winHeight, perspectiveMatrix);
    }
    bool validHoveredPosition = false;
    {
        float planeHeight = 0.5f;
        float distance;
        bool hit = glm::intersectRayPlane(gameState->camera.getPosition(), mouseDirection, glm::vec3(0.f,planeHeight,0.f), glm::vec3{0.f,1.f,0.f}, distance);
        if(hit){
            hoveredWorldPosition = gameState->camera.getPosition() + mouseDirection * distance;
            validHoveredPosition = true;
        }
    }
    if(gameState->inputModule.isKeyPressed(GLFW_MOUSE_BUTTON_LEFT)) {
        // select entity
        u32 entityIndex = -1;
    
        // CreateRayOfCubes(gameState, gameState->camera.getPosition(), mouseDirection, 20);
        entityIndex = gameState->world->raycast(gameState->camera.getPosition(), mouseDirection, 500.f);
        
        gameState->use_area_select = true;
        gameState->area_select_start = hoveredWorldPosition;
        
        if(entityIndex != -1){
            Entity* entity = gameState->world->entities.get(entityIndex);
            // entity->x += 0.5;

            if(gameState->inputModule.isKeyDown(GLFW_KEY_LEFT_SHIFT)) {
                bool found = false;
                for(int i=0;i<gameState->selectedEntities.size();i++){
                    if(gameState->selectedEntities[i] == entityIndex) {
                        found = true;
                        break;
                    }
                }
                if(!found) {
                    gameState->selectedEntities.add(entityIndex);
                    entity->flags |= Entity::HIGHLIGHTED;
                }
            } else {
                for(int i=0;i<gameState->selectedEntities.size();i++){
                    Entity* entity = gameState->world->entities.get(gameState->selectedEntities[i]);
                    entity->flags &= ~Entity::HIGHLIGHTED;
                }
                gameState->selectedEntities.resize(0);
                gameState->selectedEntities.add(entityIndex);
                entity->flags |= Entity::HIGHLIGHTED;
            }
        } else {
            if(gameState->inputModule.isKeyDown(GLFW_KEY_LEFT_SHIFT)) {
            } else {
                for(int i=0;i<gameState->selectedEntities.size();i++){
                    Entity* entity = gameState->world->entities.get(gameState->selectedEntities[i]);
                    entity->flags &= ~Entity::HIGHLIGHTED;
                }
                gameState->selectedEntities.resize(0);
            }
        }
    }
    if(gameState->inputModule.isKeyReleased(GLFW_MOUSE_BUTTON_LEFT) && gameState->use_area_select) {
        gameState->use_area_select = false;
        
        glm::vec3 bound = gameState->area_select_start;
        glm::vec3 bound_end = hoveredWorldPosition;
        if(bound.x > bound_end.x) {
            float t = bound.x;
            bound.x = bound_end.x;
            bound_end.x = t;
        }
        if(bound.z > bound_end.z) {
            float t = bound.z;
            bound.z = bound_end.z;
            bound_end.z = t;
        }
        
        // select units
        BucketArray<Entity>::Iterator iterator{};
        while(gameState->world->entities.iterate(iterator)) {
            Entity* entity = iterator.ptr;
            
            EntityStats* stats = gameState->registries.getEntityStats(entity->entityType);
            Assert(stats);
            glm::vec3& pos = entity->pos;
            glm::vec3& pos_end = entity->pos + stats->size;
            
            if(pos_end.x > bound.x && pos.x < bound_end.x &&
                pos_end.z > bound.z && pos.z < bound_end.z
            ) {
                bool found = false;
                for(int i=0;i<gameState->selectedEntities.size();i++){
                    if(gameState->selectedEntities[i] == iterator.index) {
                        found = true;
                        break;
                    }
                }
                if(!found) {
                    gameState->selectedEntities.add(iterator.index);
                    entity->flags |= Entity::HIGHLIGHTED;
                }
            }
        }
    }
   
    if(gameState->inputModule.isKeyPressed(GLFW_KEY_T)) {
        // TRAINGING HALL
        if(gameState->blueprintType == ENTITY_TRAINING_HALL) {
            gameState->blueprintType = ENTITY_NONE;
        } else {
            if(gameState->hasSufficientResources(ENTITY_TRAINING_HALL, false, true)) {
                gameState->blueprintType = ENTITY_TRAINING_HALL;
            }
        }
        // rendering of blueprint happens further down
    }
     if(gameState->inputModule.isKeyPressed(GLFW_KEY_Y)) {
        // TRAINGING HALL
        if(gameState->blueprintType == ENTITY_WORKER) {
            gameState->blueprintType = ENTITY_NONE;
        } else {
            if(gameState->hasSufficientResources(ENTITY_WORKER, false, true)) {
                gameState->blueprintType = ENTITY_WORKER;
            }
        }
        // rendering of blueprint happens further down
    }
    bool clearOldActions = !gameState->inputModule.isKeyDown(GLFW_KEY_LEFT_SHIFT);
    bool keepBlueprint = gameState->inputModule.isKeyDown(GLFW_KEY_LEFT_SHIFT);
    if(gameState->inputModule.isKeyPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
        if(gameState->blueprintType != ENTITY_NONE) {
            
            if(gameState->hasSufficientResources(gameState->blueprintType, true, true)) {
                
                if(gameState->blueprintType == ENTITY_TRAINING_HALL) {
                    Entity* entity;
                    u32 entityIndex = gameState->world->entities.add(nullptr, &entity);
                    entity->entityType = gameState->blueprintType;
                    
                    entity->pos = hoveredWorldPosition;
                    entity->pos.x = floorf((entity->pos.x) / TILE_SIZE_IN_WORLD);
                    entity->pos.y = floorf((entity->pos.y) / TILE_SIZE_IN_WORLD);
                    entity->pos.z = floorf((entity->pos.z) / TILE_SIZE_IN_WORLD);
                    
                    EntityData* data;
                    entity->extraData = gameState->world->registries->registerEntityData(&data);
                    data->busyTraining = false;
                    
                    gameState->training_halls.add(entityIndex);
                } else {
                    bool refund = !gameState->trainUnit(gameState->blueprintType, hoveredWorldPosition);
                    if(refund) {
                        gameState->refundResources(gameState->blueprintType);
                    } else {
                        Particle part{};
                        part.lifeTime = 1.5f;
                        part.size = 0.4f;
                        part.vel = {0,4.f,0};
                        part.color = {0.7,0.7,0.7};
                        part.pos = hoveredWorldPosition;
                        part.pos.x = floorf((part.pos.x) / TILE_SIZE_IN_WORLD);
                        part.pos.y = floorf((part.pos.y) / TILE_SIZE_IN_WORLD);
                        part.pos.z = floorf((part.pos.z) / TILE_SIZE_IN_WORLD);
                        part.pos += (0.5f - part.size/2.f) * glm::vec3(1);
                        gameState->addParticle(part);
                    }
                }
                if(!keepBlueprint) {
                    gameState->blueprintType = ENTITY_NONE;
                }
            }
        } else {
            if(validHoveredPosition){
                CreateRayOfCubes(gameState, hoveredWorldPosition, {0.f,1.f,0.f},5);
                // CreateRayOfCubes(gameState, gameState->camera.getPosition(), mouseDirection,20);
                    
                int grid_x,grid_z;
                Tile* tile = gameState->world->tileFromWorldPosition(hoveredWorldPosition.x, hoveredWorldPosition.z,grid_x,grid_z);

                for(int i=0;i<gameState->selectedEntities.size();i++){
                    Entity* entity = gameState->world->entities.get(gameState->selectedEntities[i]);
                    if(entity->extraData != 0) {
                        auto data = gameState->registries.getEntityData(entity->extraData);
                        if(clearOldActions) {
                            data->actionQueue.resize(0);
                        }
                        data->actionQueue.add({});
                        auto& action = data->actionQueue.last();
                        if(tile && (tile->tileType == TILE_WOOD || tile->tileType == TILE_STONE || tile->tileType == TILE_IRON)) {
                            action.actionType = EntityAction::ACTION_GATHER;
                            action.targetPosition = hoveredWorldPosition
                             - glm::vec3(0.5f,0.5f,0.5f);
                            action.gatherMaterial = tile->tileType;
                            action.grid_x = grid_x;
                            action.grid_z = grid_z;
                        } else {
                            action.actionType = EntityAction::ACTION_MOVE;
                            action.targetPosition = hoveredWorldPosition
                             - glm::vec3(0.5f,0.5f,0.5f);
                        }
                    }
                }
            }
        }
    }

    gameState->cubeShader.bind();

    glm::vec3 lightPos = { glm::cos(gameState->game_runtime) * 2, 7, 2 * glm::sin(gameState->game_runtime) };
    gameState->cubeShader.setVec3("uLightPos",lightPos);
    gameState->cubeShader.setMat4("uProj", perspectiveMatrix * gameState->camera.getViewMatrix());
    
    if(gameState->use_area_select) {
        GameState::CubeInstance inst;
        inst.r = 0.05;
        inst.g = 0.75;
        inst.b = 0.05;
        
        float thick = 0.1;
        
        glm::vec3 a = gameState->area_select_start;
        glm::vec3 b = hoveredWorldPosition;
        if(a.x > b.x) {
            float t = a.x;
            a.x = b.x;
            b.x = t;
        }
        if(a.z > b.z) {
            float t = a.z;
            a.z = b.z;
            b.z = t;
        }
        a.x -= thick/2;
        b.z -= thick/2;
        b.x -= thick/2;
        a.z -= thick/2;
        float w = b.x - a.x;
        float d = b.z - a.z;
        w += thick;
        d += thick;
        
        if(w < thick)
            w = thick;
        if(d < thick)
            d = thick;
        
        inst.sy = thick;
        inst.y = a.y;
        
        inst.sx = w;
        inst.sz = thick;
        inst.x = a.x;
        inst.z = a.z;
        DrawCube(gameState, inst);
        
        inst.sx = w;
        inst.sz = thick;
        inst.x = a.x;
        inst.z = b.z;
        DrawCube(gameState, inst);
        
        inst.sx = thick;
        inst.sz = d;
        inst.x = a.x;
        inst.z = a.z;
        DrawCube(gameState, inst);
        
        inst.sx = thick;
        inst.sz = d;
        inst.x = b.x;
        inst.z = a.z;
        DrawCube(gameState, inst);
        
        
        // inst.sy = 0.5;
        // inst.y = a.y;
        
        // inst.sx = 0.5;
        // inst.sz = 0.5;
        // inst.x = hoveredWorldPosition.x;
        // inst.z = hoveredWorldPosition.z;
        // DrawCube(gameState, inst);
    }
    
    if(gameState->blueprintType != ENTITY_NONE) {
        GameState::CubeInstance inst{};
        auto stats = gameState->registries.getEntityStats(gameState->blueprintType);
        
        glm::vec3 pos = hoveredWorldPosition;
        
        inst.sx = stats->size.x;
        inst.sy = stats->size.y;
        inst.sz = stats->size.z;
        
        pos.x = floorf((pos.x) / TILE_SIZE_IN_WORLD);
        pos.y = floorf((pos.y) / TILE_SIZE_IN_WORLD);
        pos.z = floorf((pos.z) / TILE_SIZE_IN_WORLD);
        
        inst.x = pos.x - inst.sx / 2 + 0.5;
        inst.y = pos.y - inst.sy / 2 + 0.5;
        inst.z = pos.z - inst.sz / 2 + 0.5;
        inst.r = stats->color.x * 1.5f;
        inst.g = stats->color.y * 1.5f;
        inst.b = stats->color.z * 1.6f;
        DrawCube(gameState, inst);
    }

    RenderWorld(gameState, gameState->world);

    glm::vec3 start = {0,0,0};
    glm::vec3 end = {3,0,6};
    
    Path* path = gameState->pathfinder.createPath_test(gameState->world, start, end);
    for(const glm::vec3& node : path->points) {
        GameState::CubeInstance inst{};
        auto stats = gameState->registries.getEntityStats(gameState->blueprintType);
        
        inst.sx = 0.7;
        inst.sy = 0.2;
        inst.sz = 0.7;
        
        inst.x = node.x - inst.sx / 2 + 0.5;
        inst.y = node.y - inst.sy / 2 + 0.5;
        inst.z = node.z - inst.sz / 2 + 0.5;

        inst.r = 0.0;
        inst.g = 0.0;
        inst.b = 0.0;
        DrawCube(gameState, inst);
    }

    // RENDER CUBES
    for(int i=0;i<gameState->cubes.size();i++){
        Cube& cube = gameState->cubes[i];
        
        float r = (i % 3) / 3.f;
        float g = (2 + i % 5) / 5.f;
        float b = (4 + i % 9) / 9.f;
        //float r = 1.f;
        //float g = 1.f;
        //float b = 1.f;
        
        DrawCube(gameState, {cube.pos.x,cube.pos.y,cube.pos.z,cube.size.x,cube.size.y,cube.size.z,r,g,b});
    }
    FlushCubeBatch(gameState);
    
    for(int i=0;i<gameState->particles.size();i++){
        auto& particle = gameState->particles[i];
        DrawCube(gameState, {particle.pos.x, particle.pos.y,particle.pos.z,particle.size,particle.size,particle.size,particle.color.r,particle.color.g,particle.color.b});
    }
    FlushCubeBatch(gameState);
    
    RenderBaseUI(gameState);
}
void RenderWorld(GameState* gameState, World* world) {
    int cubeCount = 0;
    BucketArray<Chunk>::Iterator chunkIterator{};
    while(world->chunks.iterate(chunkIterator)) {
        Chunk* chunk = chunkIterator.ptr;

        for(int i=0;i<Chunk::TILES_PER_CHUNK;i++) {
            Tile* tile = chunk->tiles + i;
            int tileX = i % Chunk::TILES_PER_SIDE;
            int tileY = i / Chunk::TILES_PER_SIDE;
            if(tile->tileType == TILE_EMPTY)
                continue;
            
            if(tile->tileType == TILE_TERRAIN) {
                float pos_y = -8.f;
                float size_y = (float)((i32)tile->height+128) / 16.f;
                glm::vec3 color = {tile->red/(255.f), tile->green/(255.f), tile->blue/(255.f)};
                
                DrawCube(gameState,{
                    (float)chunk->x + tileX, pos_y, (float)chunk->z + tileY,
                    1.f,size_y,1.f, // size
                    color.r,color.g,color.b});
            } else {
                float pos_y = -8.f;
                float size_y = (float)((i32)tile->height+128) / 16.f;
                glm::vec3 color = {tile->red/(255.f), tile->green/(255.f), tile->blue/(255.f)};
                DrawCube(gameState,{
                    (float)chunk->x + tileX, pos_y, (float)chunk->z + tileY,
                    1.f,size_y,1.f, // size
                    color.r,color.g,color.b});
                    
                float fore_pos_y = pos_y + size_y;
                float fore_size_y = 1.f;
                glm::vec3 fore_color = gameState->registries.getTileColor(tile->tileType);
                
                DrawCube(gameState,{
                    (float)chunk->x + tileX, fore_pos_y, (float)chunk->z + tileY,
                    1.f,fore_size_y,1.f, // size
                    fore_color.r,fore_color.g,fore_color.b});
            }
        }
    }
    FlushCubeBatch(gameState);

    BucketArray<Entity>::Iterator entityIterator{};
    while(world->entities.iterate(entityIterator)) {
        Entity* entity = entityIterator.ptr;
        
        EntityStats* stats = gameState->registries.getEntityStats(entity->entityType);
        glm::vec3 color = stats->color;
        if(entity->flags & Entity::HIGHLIGHTED) {
            color = (color + glm::vec3(0.1f,0.1f,0.1f)) * 2.f;
        }
        glm::vec3 size = stats->size;
        
        DrawCube(gameState, {
            entity->pos.x + (1.f-size.x)/2.f, entity->pos.y + (1.f-size.y)/2.f, entity->pos.z + (1.f-size.z)/2.f,
            size.x, size.y, size.z, // size
            color.r,color.g,color.b});
            
    }
    FlushCubeBatch(gameState);
}
void GameState::addParticle(const Particle& particle){
    particles.add(particle);
}
void GameState::addMessage(const std::string& text, float time, const glm::vec3& color) {
    messages.add({text, time, color});
}
bool GameState::trainUnit(EntityType entityType, const glm::vec3& target) {
    if(training_halls.size() == 0) {
        addMessage("Cannot train units, place a training hall first!", 5.f, MSG_COLOR_RED);
        return false;
    }
    trainingQueue.add({});
    TrainingEntry& unit = trainingQueue.last();
    unit.type = entityType;
    unit.target_position = target;
    switch(entityType) {
        case ENTITY_WORKER: {
            unit.remaining_time = 3.f;
            break;
        }
        case ENTITY_SOLDIER: {
            unit.remaining_time = 5.f;
            break;
        }
    }
    unit.responsible_entityIndex = -1;
    // where the unit should be trained is decided in update function
    return true;
}
bool GameState::hasSufficientResources(EntityType entityType, bool useResources, bool logMissingResources){
    bool comma = false;
    bool sufficient = true;
    int written = 0;
    char temp[200];
    for(ItemType itemType=(ItemType)(ITEM_NONE + 1);itemType<ITEM_TYPE_MAX;itemType=(ItemType)(itemType+1)){
        int available_amount = teamResources.getAmount(itemType);
        int required_amount = entityCosts[entityType].amounts[itemType];
        
        if(available_amount < required_amount) {
            if(logMissingResources) {
                if(sufficient) {
                    written += snprintf(temp + written,sizeof(temp) - written,"Insufficent resources for '%s' (missing ", ToString(entityType));
                }
                if(comma)
                    written += snprintf(temp + written, sizeof(temp) - written, ", ");
                written += snprintf(temp + written,sizeof(temp)-written,"%d %s", required_amount - available_amount, ToString(itemType));
                comma = true;
            }
            sufficient = false;
        }
    }
    if(sufficient) {
        if(useResources) {
            for(ItemType itemType=(ItemType)(ITEM_NONE + 1);itemType<ITEM_TYPE_MAX;itemType=(ItemType)(itemType+1)){
                int required_amount = entityCosts[entityType].amounts[itemType];
                teamResources.removeItem(itemType, required_amount);
            }
        }
        return true;
    }
    if(written!=0){
        addMessage(std::string(temp), 6.f, GameState::MSG_COLOR_RED);   
    }
    return false;
}
bool GameState::refundResources(EntityType entityType){
    for(ItemType itemType=(ItemType)(ITEM_NONE + 1);itemType<ITEM_TYPE_MAX;itemType=(ItemType)(itemType+1)){
        int required_amount = entityCosts[entityType].amounts[itemType];
        teamResources.addItem(itemType, required_amount);
    }
    return true;
}
void FirstPersonProc(GameState* state) {
    const float cameraSensitivity = 0.3f;
    if (state->inputModule.m_lastMouseX != -1) {
        if (state->inputModule.isCursorLocked()) {
            float rawX = -(state->inputModule.getMouseX() - state->inputModule.m_lastMouseX) * (glm::pi<float>() / 360.0f) * cameraSensitivity;
            float rawY = -(state->inputModule.getMouseY() - state->inputModule.m_lastMouseY) * (glm::pi<float>() / 360.0f) * cameraSensitivity;

            glm::vec3 rot = state->camera.getRotation(); // get a copy of rotation
            rot.y += rawX;
            rot.x += rawY;
            // clamp up and down directions.
            if (rot.x > glm::pi<float>() / 2) {
                rot.x = glm::pi<float>() / 2;
            }
            if (rot.x < -glm::pi<float>() / 2) {
                rot.x = -glm::pi<float>() / 2;
            }
            rot.x = remainder(rot.x, glm::pi<float>() * 2);
            rot.y = remainder(rot.y, glm::pi<float>() * 2);
            state->camera.setRotation(rot); // set the new rotation
            //log::out << "FIRST PERSON\n";
        }
    }
}
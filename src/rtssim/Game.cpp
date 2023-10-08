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

    glfwSwapInterval(1); // limit fps to your monitors frequency?

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
                    pos.y = action.targetPosition.y;
                    float length = glm::length(action.targetPosition - pos);
                    float moveLength = stats->moveSpeed * gameState->update_deltaTime;
                    glm::vec3 moveDir = glm::normalize(action.targetPosition - pos) * moveLength;
                    if(length < moveLength) {
                        completedAction = true;
                        entity->pos = action.targetPosition + glm::vec3(0,entity->pos.y - action.targetPosition.y,0);
                    } else {
                        entity->pos += moveDir;
                    }
                    break;
                }
                case EntityAction::ACTION_GATHER: {
                    // if(data->dropResource) {
                        
                    // } else {
                        pos.y = action.targetPosition.y;
                        float length = glm::length(action.targetPosition - pos);
                        float moveLength = stats->moveSpeed * gameState->update_deltaTime;
                        glm::vec3 moveDir = glm::normalize(action.targetPosition - pos) * moveLength;
                        if(length < moveLength) {
                            // completedAction = true;
                            entity->pos = action.targetPosition + glm::vec3(0,entity->pos.y - action.targetPosition.y,0);
                            if(data->gathering)
                                data->gatherTime += gameState->update_deltaTime;
                                
                            float gatherTime = 1.f;
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
                                        
                                        // TODO: Switch to action "ACTION_SEARCH_AND_GATHER", this includes gathering and pathfinding to a resource to gather
                                        completedAction = true;
                                    }
                                } else {
                                    // TODO: Switch to action "ACTION_SEARCH_AND_GATHER", this includes gathering and pathfinding to a resource to gather
                                    completedAction = true;
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
                    // }
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
    
    gameState->cubesToDraw = 0;
    
    // glClearColor(1,0.5,0.33,1);
    glClearColor(0.1,0.5,0.33,1);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    auto& ui = gameState->uiModule;

    #define UI_TEXT(X,Y,H, R,G,B,A, STR) { auto text = ui.makeText(); text->x = X; text->y = Y; text->h = H; text->color = {R,G,B,A}; text->string = STR; text->length = strlen(STR); }

    static char str_fps[100];
    snprintf(str_fps, sizeof(str_fps), "%.2f ms (%d FPS)", (float)(gameState->avg_frameTime.getAvg() * 1000), (int)(1.0/gameState->avg_frameTime.getAvg()));
    UI_TEXT(5,3,18, 1,1,1,1, str_fps)
    
    std::string string = engone::FormatTime(gameState->avg_updateTime.getAvg(), true, FormatTimeMS | FormatTimeUS | FormatTimeNS);
    static char str_upt[200];
    snprintf(str_upt, sizeof(str_upt), "update: %s", string.c_str());
    UI_TEXT(5,3 + 18 + 18,18, 1,1,1,1, str_upt)

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

    static char str_camera[200];
    snprintf(str_camera, sizeof(str_camera), "camrot: %.2f, %.2f, %.2f", (float)(gameState->camera.getRotation().x),gameState->camera.getRotation().y,gameState->camera.getRotation().z);
    UI_TEXT(5,3+18,18, 1,1,1,1, str_camera)

    float ratio = gameState->winWidth / gameState->winHeight;
    glm::mat4 perspectiveMatrix = glm::mat4(1);
    if (std::isfinite(ratio))
        perspectiveMatrix = glm::perspective(glm::radians(gameState->fov), ratio, gameState->zNear, gameState->zFar);

    glm::mat4 viewMatrix = gameState->camera.getViewMatrix();
    
    glm::vec3 hoveredWorldPosition = {0,0,0};
    bool validHoveredPosition = false;
    {
        glm::vec3 dir = {0,0,0};
        // select entity
        if(gameState->inputModule.isCursorLocked()) {
            dir = gameState->camera.getLookVector();
        } else {
            dir = RaycastFromMouse(gameState->inputModule.getMouseX(), gameState->inputModule.getMouseY(), gameState->winWidth, gameState->winHeight, viewMatrix, perspectiveMatrix);
        }
        float planeHeight = 0.5f;
        float distance;
        bool hit = glm::intersectRayPlane(gameState->camera.getPosition(), dir, glm::vec3(0.f,planeHeight,0.f), glm::vec3{0.f,1.f,0.f}, distance);
        if(hit){
            hoveredWorldPosition = gameState->camera.getPosition() + dir * distance;
            // hoveredWorldPosition.y ;
            validHoveredPosition = true;
        }
    }
    if(gameState->inputModule.isKeyPressed(GLFW_MOUSE_BUTTON_LEFT)) {
        u32 entityIndex = -1;
        glm::vec3 dir = {0,0,0};
        // select entity
        if(gameState->inputModule.isCursorLocked()) {
            dir = gameState->camera.getLookVector();
        } else {
            dir = RaycastFromMouse(gameState->inputModule.getMouseX(), gameState->inputModule.getMouseY(), gameState->winWidth, gameState->winHeight, viewMatrix, perspectiveMatrix);
        }
        // CreateRayOfCubes(gameState, gameState->camera.getPosition(), dir, 20);
        entityIndex = gameState->world->raycast(gameState->camera.getPosition(), dir, 20.f);
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
                gameState->use_area_select = true;
                gameState->area_select_start = hoveredWorldPosition;
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
                Entity* entity;
                u32 entityIndex = gameState->world->entities.add(nullptr, &entity);
                entity->entityType = gameState->blueprintType;
                
                entity->pos = hoveredWorldPosition;
                entity->pos.x = floorf((entity->pos.x) / TILE_SIZE_IN_WORLD);
                entity->pos.y = floorf((entity->pos.y) / TILE_SIZE_IN_WORLD);
                entity->pos.z = floorf((entity->pos.z) / TILE_SIZE_IN_WORLD);
                
                if(entity->entityType == ENTITY_WORKER || entity->entityType == ENTITY_SOLDIER) {
                    EntityData* data;
                    entity->extraData = gameState->world->registries->registerEntityData(&data);
                }
                
                if(!keepBlueprint) {
                    gameState->blueprintType = ENTITY_NONE;
                }
            }
        } else {
            if(validHoveredPosition){
                CreateRayOfCubes(gameState, hoveredWorldPosition, {0.f,1.f,0.f},5);
                    
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
    // gameState->cubeShader.bind();
    gameState->cubeShader.setVec3("uLightPos",lightPos);
    gameState->cubeShader.setMat4("uProj", perspectiveMatrix * gameState->camera.getViewMatrix());

    // gameState->cubeVA.draw(&gameState->cubeIB);
    
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
glm::vec3 RaycastFromMouse(int mx, int my, int winWidth, int winHeight, const glm::mat4& viewMatrix, const glm::mat4& perspectiveMatrix) {
    // screen/window space
    float screen_x = (float)mx / winWidth * 2.f - 1.f;
    float screen_y = 1.f - (float)my / winHeight * 2.f;
    glm::vec4 screen_point = glm::vec4(screen_x, screen_y, 1.0f, 1.0f);

    glm::vec3 ray_dir = glm::inverse(viewMatrix) * (glm::inverse(perspectiveMatrix) * screen_point);
    ray_dir = glm::normalize(ray_dir);
    return ray_dir;
    // https://gamedev.stackexchange.com/questions/200114/constructing-a-world-ray-from-mouse-coordinates
}
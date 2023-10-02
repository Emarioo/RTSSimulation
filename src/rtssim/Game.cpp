#include "rtssim/Game.h"

#include "rtssim/CubeShader.glsl"

GameState* GameState::Create(){
    GameState* state = TRACK_ALLOC(GameState);
    new(state)GameState();
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

void StartGame() {
    using namespace engone;
    // fprintf(stderr, "fast\n");
    // log::out << "Start game!\n";

    Assert(glfwInit());
    // if (!glfwInit()) {
    //     log::out << "NOPE!\n";
    //     return;
    // }
 
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
 
    GLFWwindow* window = glfwCreateWindow(640, 480, "RTS Simulation", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return;
    }
 
    glfwMakeContextCurrent(window);
    glewInit();

    glfwSwapInterval(1); // limit fps to your monitors frequency?

    GameState* gameState = GameState::Create();
    gameState->window = window;

    SetupRendering(gameState);

    // gameState->cubes.add({{0,0,0},{1,1,1}});
    // gameState->cubes.add({{0,-1.5,0},{0.7,0.5,1}});
    // gameState->cubes.add({{0.3,0,0},{0.1,0.1,0.1}});

    gameState->world = World::CreateTest(&gameState->registries);

    #define HOT_RELOAD_ORIGIN "bin/game.dll"
    #define HOT_RELOAD_IN_USE "bin/hotloaded.dll"
    #define HOT_RELOAD_ORIGIN_PDB "bin/game.pdb"
    #define HOT_RELOAD_IN_USE_PDB "bin/hotloaded.pdb"
    #define HOT_RELOAD_TIME 2

    GameProcedure updateProc = UpdateGame;
    GameProcedure renderProc = RenderGame;

    void* hot_reload_dll = nullptr;
    double last_dll_write = 0;
 
    auto lastTime = engone::StartMeasure();

    // glEnable(GL_BLEND);
    // glEnable(GL_CULL_FACE);
    // glEnable(GL_DEPTH_TEST);
    // glCullFace(GL_FRONT);

    gameState->uiModule.init();
    gameState->inputModule.init(gameState->window);

    gameState->camera.setPosition(2,4,5);
    gameState->camera.setRotation(0.3f,3.0f,0);

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

        updateAccumulation += frame_deltaTime;

        gameState->sum_frameCount++;
        gameState->sum_frameTime+=frame_deltaTime;
        
        sec_timer += frame_deltaTime;
        if(sec_timer >= 0.5) {
            sec_timer -= 0.5;
            gameState->avg_frameTime = gameState->sum_frameTime / gameState->sum_frameCount;
            gameState->sum_frameCount = 1; // don't set to 0 because you may accidently divide by zero
            gameState->sum_frameTime = frame_deltaTime;
        }
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
                if(hot_reload_dll)
                    engone::UnloadDynamicLibrary(hot_reload_dll);
                hot_reload_dll = nullptr;
                engone::FileCopy(HOT_RELOAD_ORIGIN, HOT_RELOAD_IN_USE);
                engone::FileCopy(HOT_RELOAD_ORIGIN_PDB, HOT_RELOAD_IN_USE_PDB);
                hot_reload_dll = engone::LoadDynamicLibrary(HOT_RELOAD_IN_USE);

                updateProc = (GameProcedure)engone::GetFunctionPointer(hot_reload_dll, "UpdateGame");
                renderProc = (GameProcedure)engone::GetFunctionPointer(hot_reload_dll, "RenderGame");
            }
        }
        // #else
        // UpdateGame(gameState);
        // RenderGame(gameState);
        #endif
        if(updateProc){
            while(updateAccumulation>gameState->update_deltaTime){
                updateAccumulation-=gameState->update_deltaTime;
                updateProc(gameState);
            }
        } else {
            // UpdateGame(gameState);
        }
        if(renderProc) {
            renderProc(gameState);
        } else {
            // RenderGame(gameState);
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
 
    glfwDestroyWindow(window);
    glfwTerminate();

    GameState::Destroy(gameState);
}

GAME_API void UpdateGame(GameState* gameState) {

    BucketArray<Entity>::Iterator iterator{};
    while(gameState->world->entities.iterate(iterator)) {
        Entity* entity = iterator.ptr;
        
        glm::vec3 pos = {entity->x, 0, entity->z};

        EntityStats* stats = gameState->registries.getEntityStats(entity->entityType);
        EntityData* data = gameState->registries.getEntityData(entity->extraData);

        bool completedAction = false;
        if(data->actionQueue.size()!=0){
            auto& action = data->actionQueue[0];
            switch(action.actionType) {
                case EntityAction::ACTION_MOVE: {
                    pos.y = action.movePosition.y;
                    float length = glm::length(action.movePosition - pos);
                    float moveLength = stats->moveSpeed * gameState->update_deltaTime;
                    glm::vec3 moveDir = glm::normalize(action.movePosition - pos) * moveLength;
                    if(length < moveLength) {
                        completedAction = true;
                        entity->x = action.movePosition.x;
                        entity->z = action.movePosition.z;
                    } else {
                        entity->x += moveDir.x;
                        entity->z += moveDir.z;
                    }
                    break;
                }
            }
        }
        if(completedAction) {
            data->actionQueue.removeAt(0);
        }
    }
}
GAME_API void RenderGame(GameState* gameState) {
    using namespace engone;
    
    // glClearColor(1,0.5,0.33,1);
    glClearColor(0.1,0.5,0.33,1);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    auto& ui = gameState->uiModule;

    #define UI_TEXT(X,Y,H, R,G,B,A, STR) { auto text = ui.makeText(); text->x = X; text->y = Y; text->h = H; text->color = {R,G,B,A}; text->string = STR; text->length = strlen(STR); }

    static char str_fps[100];
    snprintf(str_fps, sizeof(str_fps), "%.2f ms (%d FPS)", (float)(gameState->avg_frameTime * 1000), (int)(1.0/gameState->avg_frameTime));
    UI_TEXT(5,3,18, 1,1,1,1, str_fps)

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

    float speed = 3.f * gameState->current_frameTime;
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
        perspectiveMatrix = glm::perspective(gameState->fov, ratio, gameState->zNear, gameState->zFar);

    glm::mat4 viewMatrix = gameState->camera.getViewMatrix();

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
            for(int i=0;i<gameState->selectedEntities.size();i++){
                Entity* entity = gameState->world->entities.get(gameState->selectedEntities[i]);
                entity->flags &= ~Entity::HIGHLIGHTED;
            }
            gameState->selectedEntities.resize(0);
        }
    }
    if(gameState->inputModule.isKeyPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
        glm::vec3 dir = {0,0,0};
        // select entity
        if(gameState->inputModule.isCursorLocked()) {
            dir = gameState->camera.getLookVector();
        } else {
            dir = RaycastFromMouse(gameState->inputModule.getMouseX(), gameState->inputModule.getMouseY(), gameState->winWidth, gameState->winHeight, viewMatrix, perspectiveMatrix);
        }
        bool clearOldActions = !gameState->inputModule.isKeyDown(GLFW_KEY_LEFT_SHIFT);
        float planeHeight = 0.5f;
        float distance;
        bool hit = glm::intersectRayPlane(gameState->camera.getPosition(), dir, glm::vec3(0.f,planeHeight,0.f), glm::vec3{0.f,1.f,0.f}, distance);
        if(hit){
            glm::vec3 movePosition = gameState->camera.getPosition() + dir * distance;
            movePosition.y += 1;

            CreateRayOfCubes(gameState, movePosition, {0.f,1.f,0.f},5);

            for(int i=0;i<gameState->selectedEntities.size();i++){
                Entity* entity = gameState->world->entities.get(gameState->selectedEntities[i]);
                if(entity->extraData != 0) {
                    auto data = gameState->registries.getEntityData(entity->extraData);
                    if(clearOldActions) {
                        data->actionQueue.resize(0);
                    }
                    data->actionQueue.add({});
                    auto& action = data->actionQueue.last();
                    action.actionType = EntityAction::ACTION_MOVE;
                    action.movePosition = movePosition - glm::vec3(0.5f,0.5f,0.5f);
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

    RenderWorld(gameState, gameState->world);

    // RENDER CUBES
    int cubeCount = 0;
    for(int i=0;i<gameState->cubes.size();i++){
        Cube& cube = gameState->cubes[i];
        
        float r = (i % 3) / 3.f;
        float g = (2 + i % 5) / 5.f;
        float b = (4 + i % 9) / 9.f;
        //float r = 1.f;
        //float g = 1.f;
        //float b = 1.f;
        gameState->cubeInstanceBatch[cubeCount] = {cube.pos.x,cube.pos.y,cube.pos.z,cube.size.x,cube.size.y,cube.size.z,r,g,b};

        cubeCount++;

        if(cubeCount == gameState->cubeInstanceBatch_max || i+1 == gameState->cubes.size()) {
            // log::out << "draw "<<cubeCount<<"\n";
            gameState->cubeInstanceVB.setData(cubeCount * sizeof(GameState::CubeInstance), gameState->cubeInstanceBatch);
            gameState->cubeVA.bind();
            gameState->cubeVA.selectBuffer(2, &gameState->cubeInstanceVB);
            gameState->cubeVA.draw(&gameState->cubeIB,cubeCount);
            cubeCount = 0;
        }
    }
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

            glm::vec3 color = gameState->registries.getTileColor(tile->tileType);

            gameState->cubeInstanceBatch[cubeCount] = {
                (float)chunk->x + tileX, (float)tile->height / 16.f, (float)chunk->z + tileY,
                1.f,1.f,1.f, // size
                color.r,color.g,color.b};
            cubeCount++;
            if(cubeCount == gameState->cubeInstanceBatch_max) {
                gameState->cubeInstanceVB.setData(cubeCount * sizeof(GameState::CubeInstance), gameState->cubeInstanceBatch);
                gameState->cubeVA.bind();
                gameState->cubeVA.selectBuffer(2, &gameState->cubeInstanceVB);
                gameState->cubeVA.draw(&gameState->cubeIB,cubeCount);
                cubeCount = 0;
            }
        }
    }
     if(cubeCount != 0) {
        // log::out << "draw "<<cubeCount<<"\n";
        gameState->cubeInstanceVB.setData(cubeCount * sizeof(GameState::CubeInstance), gameState->cubeInstanceBatch);
        gameState->cubeVA.bind();
        gameState->cubeVA.selectBuffer(2, &gameState->cubeInstanceVB);
        gameState->cubeVA.draw(&gameState->cubeIB,cubeCount);
        cubeCount = 0;
    }

    BucketArray<Entity>::Iterator entityIterator{};
    while(world->entities.iterate(entityIterator)) {
        Entity* entity = entityIterator.ptr;
        
        EntityStats* stats = gameState->registries.getEntityStats(entity->entityType);
        glm::vec3 color = stats->color;
        if(entity->flags & Entity::HIGHLIGHTED) {
            color = (color + glm::vec3(0.1f,0.1f,0.1f)) * 2.f;
        }
        glm::vec3 size = stats->size;
        float yPos = 1;

        gameState->cubeInstanceBatch[cubeCount] = {
            entity->x + (1.f-size.x)/2.f, yPos + (1.f-size.y)/2.f, entity->z + (1.f-size.z)/2.f,
            size.x, size.y, size.z, // size
            color.r,color.g,color.b};
        cubeCount++;
        if(cubeCount == gameState->cubeInstanceBatch_max) {
            gameState->cubeInstanceVB.setData(cubeCount * sizeof(GameState::CubeInstance), gameState->cubeInstanceBatch);
            gameState->cubeVA.bind();
            gameState->cubeVA.selectBuffer(2, &gameState->cubeInstanceVB);
            gameState->cubeVA.draw(&gameState->cubeIB,cubeCount);
            cubeCount = 0;
        }
    }
    if(cubeCount != 0) {
        // log::out << "draw "<<cubeCount<<"\n";
        gameState->cubeInstanceVB.setData(cubeCount * sizeof(GameState::CubeInstance), gameState->cubeInstanceBatch);
        gameState->cubeVA.bind();
        gameState->cubeVA.selectBuffer(2, &gameState->cubeInstanceVB);
        gameState->cubeVA.draw(&gameState->cubeIB,cubeCount);
        cubeCount = 0;
    }
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
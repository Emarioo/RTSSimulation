#pragma once

#include "Engone/Util/Array.h"
#include "Engone/Camera.h"
#include "Engone/Rendering/Buffer.h"
#include "Engone/Util/Tracker.h"
#include "Engone/UIModule.h"
#include "Engone/InputModule.h"

#include "rtssim/World.h"
#include "rtssim/Registry.h"

#ifdef USE_HOT_RELOAD
#define GAME_API extern "C" __declspec(dllexport)
#else
#define GAME_API
#endif

struct Cube {
    glm::vec3 pos;
    glm::vec3 size;
};

struct GameState {
    static GameState* Create();
    static void Destroy(GameState* gameState);

    // Application stuff
    GLFWwindow* window=nullptr;
    float winWidth=0, winHeight=0;

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

    engone::Shader cubeShader{};
    float fov = 90.f, zNear = 0.1f, zFar = 400.f;
    engone::Camera camera{};

    engone::UIModule uiModule{};

    double update_deltaTime = 1.0f/60.0f; // fixed
    double current_frameTime = 1.0f/60.0f;
    double sum_frameTime = 0;
    int sum_frameCount = 0;
    double avg_frameTime = 0.016;
    double game_runtime = 0;

    // Actual game
    Registries registries{};
    World* world = nullptr;

    DynamicArray<u32> selectedEntities{};
};
typedef void (*GameProcedure)(GameState* gameState);

GAME_API void RenderGame(GameState* gameState);
GAME_API void UpdateGame(GameState* gameState);

void StartGame();
void FirstPersonProc(GameState* gameState);

void RenderWorld(GameState* gameState, World* world);

glm::vec3 RaycastFromMouse(int mx, int my, int winWidth, int winHeight, const glm::mat4& viewMatrix, const glm::mat4& perspectiveMatrix);
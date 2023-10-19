#pragma once

#include "rtssim/World.h"

#include "Engone/Util/Array.h"

struct Path {
    DynamicArray<glm::vec3> points;

    void add(int x, int z) {
        points.add({x * TILE_SIZE_IN_WORLD,0,z* TILE_SIZE_IN_WORLD});
    }
};
struct Pathfinder {
    Path* createPath_test(World* world, const glm::vec3& start, const glm::vec3& end);
    void destroyPath(Path* path);
};
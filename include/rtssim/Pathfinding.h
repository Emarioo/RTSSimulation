#pragma once

// #include "rtssim/World.h"
#include "Engone/Util/Array.h"

/*
Pathfinding API
    Requirements:
        Global class to keep track of all pathfindings in progress.
        Individual class for each instance of pathfinding.
        Function to start a pathfinding.
        Function to run the pathfinding some amount of steps.
        Function to destroy or reset a pathfinding.
*/

struct World;
struct Path {
    Path() = default;
    void add(int x, int z);
    struct Node {
        int x, z, prevNode = -1, curCost = 0, remCost = 0; 
    };

    DynamicArray<glm::vec3> finalPoints;
    DynamicArray<Node> nearbyNodes{}; // unexplored nodes
    DynamicArray<Node> visitedNodes{}; // explored nodes

    int steps = 0;
    int start_x, start_z;
    int end_x, end_z;
    glm::vec3 realGoal;
    bool finished = false;

    int costToEnd(const Node& node) {
        int dx = (end_x - node.x);
        int dz = (end_z - node.z);
        // return abs(dx) + abs(dz); // cardinal cost (square search)
        // return dx*dx + dz*dz; // squared magnitude cost (circular search)
        return (int)sqrtf(dx*dx + dz*dz); // magnitude cost (slow)
    };
    bool sameGoal(World* world, const glm::vec3& end);
    int pathIndex = -1;
};
struct Pathfinder {
    Pathfinder() = default;
    // TODO: Some functions in here could be moved to Path but I have not in case
    //  they will need data from this global class in the future.
    // TODO: Look into reusage of a path's allocations. An entity can release a path without
    //  cleaning it's allocations and make that path available for other entities to use.
    //  You would need an array of available paths. If none are then you do bucketArray.add.

    // pointer will not be invalidated unless you cleanup this class.
    Path* startPath(World* world, const glm::vec3& start, const glm::vec3& end);
    Path* getPath(int pathIndex) {
        return m_paths.get(pathIndex);
    }
    void restartPath(Path* path, World* world, const glm::vec3& start, const glm::vec3& end);
    void destroyPath(Path* path);
    // @param stepsToTake A value of -1 will process steps until goal is reached (unless unreachable).
    // @return Processed steps. Zero may indicate an unreachable goal or a finished state. Use path->finished to know which.
    int stepPath(World* world, Path* path, int stepsToTake = -1);

    // Path* getPath(int index);
    
    BucketArray<Path> m_paths{100};
};
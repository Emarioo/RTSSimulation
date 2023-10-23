#include "rtssim/Pathfinding.h"

#include "rtssim/World.h"

Path* Pathfinder::startPath(World* world, const glm::vec3& start, const glm::vec3& end) {
    Assert(world);
    Path* path = nullptr;
    int index = m_paths.add(nullptr, &path);
    if(index == -1)
        return nullptr;
    new(path)Path();
    path->pathIndex = index;
    restartPath(path, world, start, end);
    return path;
}
void Pathfinder::restartPath(Path* path, World* world, const glm::vec3& start, const glm::vec3& end) {
    Assert(path);
    Assert(world);
    world->gridPosFromWorld(start, path->start_x, path->start_z, true);
    world->gridPosFromWorld(end, path->end_x, path->end_z, true);
    path->realGoal = end;

    path->nearbyNodes.resize(0);
    path->visitedNodes.resize(0);
    
    path->nearbyNodes.add({path->start_x, path->start_z});
    path->nearbyNodes.last().remCost = path->costToEnd(path->nearbyNodes.last());

    path->steps = 0;
    path->finished = false;
    path->finalPoints.resize(0);
}
void Path::add(int x, int z) {
    finalPoints.add({x * TILE_SIZE_IN_WORLD,0,z* TILE_SIZE_IN_WORLD});
}
bool Path::sameGoal(World* world, const glm::vec3& end) {
        // int sx, sz;
        int ex, ez;
        // world->gridPosFromWorld(start, sx, sz);
        world->gridPosFromWorld(end, ex, ez, true);
        // return sx == start_x && sz == start_z &&
        return ex == end_x && ez == end_z;
    }
int Pathfinder::stepPath(World* world, Path* path, int stepsToTake) {
    if(path->finished)
        return 0; // zero steps taken
    
    auto FindNode_nearby = [&](const Path::Node& n) {
        for(int i=0;i<path->nearbyNodes.size();i++){
            auto& node = path->nearbyNodes[i];
            if(node.x == n.x && node.z == n.z) {
                return i;
            }
        }
        return -1;
    };
    auto FindNode_visited = [&](const Path::Node& n) {
        int index = -1;
        for(int i=0;i<path->visitedNodes.size();i++){
            auto& node = path->visitedNodes[i];
            if(node.x == n.x && node.z == n.z) {
                return i;
            }
        }
        return -1;
    };

    int processedSteps = 0;
    if(stepsToTake==-1){
        stepsToTake = 0x7FFF'FFFF;
    }

    // A* algorithm (similar at least)
    while (path->nearbyNodes.size()>0 && processedSteps < stepsToTake) {
        // Pick best/closest node
        int minDist = 0;
        int index = -1;
        for(int i=0;i<path->nearbyNodes.size();i++) {
            auto& node = path->nearbyNodes[i];
            if(minDist > node.remCost || index == -1) {
                minDist = node.remCost;
                index = i;
            }
        }
        Assert(index != -1); // happens if nearbyNodes is zero which shouldn't be possible beacuse while condition checks it

        path->visitedNodes.add(path->nearbyNodes[index]);
        auto& node = path->visitedNodes.last();
        int nodeIndex = path->visitedNodes.size()-1;
        path->nearbyNodes.removeAt(index);

        path->steps++;
        processedSteps++;
        
        if(node.x == path->end_x && node.z == path->end_z) {
            // reached goal
            path->finished = true;
            break;
        }
        
        // Add nearby nodes to list (if they aren't already)
        Path::Node neighbours[4]{
            {node.x - 1, node.z},
            {node.x + 1, node.z},
            {node.x, node.z - 1},
            {node.x, node.z + 1},
            // {node.x - 1, node.z-1},
            // {node.x + 1, node.z-1},
            // {node.x+1, node.z - 1},
            // {node.x-1, node.z + 1},
        };
        for(int j=0;j<sizeof(neighbours)/sizeof(*neighbours);j++) {
            auto& closeNode = neighbours[j];

            // Check if node/tile is open
            Tile* tile = world->tileFromGridPosition(closeNode.x, closeNode.z);
            if(!tile || tile->occupied)
                continue;

            int index_nearby = FindNode_nearby(closeNode);
            int index_visited = FindNode_visited(closeNode);
            int newCurCost = node.curCost + 1;
            if(index_visited != -1) {
                auto& visited = path->visitedNodes[index_visited];
                if(newCurCost < visited.curCost) {
                    visited.prevNode = nodeIndex;
                    visited.curCost = newCurCost;
                }
            } else if (index_nearby != -1) {
                auto& nearby = path->nearbyNodes[index_nearby];
                if(newCurCost < nearby.curCost) {
                    nearby.prevNode = nodeIndex;
                    nearby.curCost = newCurCost;
                }
            } else {
                path->nearbyNodes.add(closeNode);
                path->nearbyNodes.last().remCost = path->costToEnd(path->nearbyNodes.last());
                path->nearbyNodes.last().prevNode = nodeIndex;
                path->nearbyNodes.last().curCost = newCurCost;
            }
        }
    }
    if(processedSteps == 0) {
        return 0; // unreachable goal
    }
    if(!path->finished) {
        return processedSteps; // we need more steps to finish
    }

    // We have finished. Construct the final path.
    int index_nextNode = path->visitedNodes.size()-1;
    while(index_nextNode != -1) {
        auto& node = path->visitedNodes[index_nextNode];
        path->add(node.x, node.z);
        index_nextNode = node.prevNode;
    }
    // reverse
    for(int j=0;j<path->finalPoints.size()/2;j++){
        glm::vec3 t = path->finalPoints[j];
        path->finalPoints[j] = path->finalPoints[path->finalPoints.size()-1-j];
        path->finalPoints[path->finalPoints.size()-1-j] = t;
    }
    path->finalPoints.add(path->realGoal);
    // Remove unnecessary nodes, straight nodes,
    return processedSteps;
}
void Pathfinder::destroyPath(Path* path) {
    int index = path->pathIndex;
    path->~Path();
    m_paths.removeAt(index);
}
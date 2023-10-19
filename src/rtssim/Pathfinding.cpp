#include "rtssim/Pathfinding.h"

Path* Pathfinder::createPath_test(World* world, const glm::vec3& start, const glm::vec3& end) {
    int start_x, start_z;
    int end_x, end_z;
    world->tileFromWorldPosition(start.x,start.z, start_x, start_z);
    world->tileFromWorldPosition(end.x,end.z,end_x,end_z);

    struct Node { int x, z, prevNode = -1, curCost = 0, remCost = 0; };
    
    auto CostToEnd = [&](const Node& node) {
        return abs(end_x - node.x) + abs(end_z - node.z);
    };

    DynamicArray<Node> nearbyNodes; // unexplored nodes
    DynamicArray<Node> visitedNodes; // explored nodes
    
    nearbyNodes.add({start_x, start_z});
    nearbyNodes.last().remCost = CostToEnd(nearbyNodes.last());

    auto FindNode_nearby = [&](const Node& n) {
        for(int i=0;i<nearbyNodes.size();i++){
            Node& node = nearbyNodes[i];
            if(node.x == n.x && node.z == n.z) {
                return i;
            }
        }
        return -1;
    };
    auto FindNode_visited = [&](const Node& n) {
        int index = -1;
        for(int i=0;i<visitedNodes.size();i++){
            Node& node = visitedNodes[i];
            if(node.x == n.x && node.z == n.z) {
                return i;
            }
        }
        return -1;
    };

    while (nearbyNodes.size()>0) {
        // Pick best/closest node
        int minDist = 0;
        int index = -1;
        for(int i=0;i<nearbyNodes.size();i++) {
            Node& node = nearbyNodes[i];
            if(minDist > node.remCost || index == -1) {
                minDist = node.remCost;
                index = i;
            }
        }
        Assert(index != -1);
        Node& node = nearbyNodes[index];
        nearbyNodes.removeAt(index);

        visitedNodes.add(node);
        int nodeIndex = visitedNodes.size()-1;
        // node.cost = ?
        
        // Is node the end?
        if(node.x == end_x && node.z == end_z) {
            // end
            break;
        }
        
        // Add nearby nodes to list (if they aren't already)
        Node neighbours[4]{
            {node.x - 1, node.z - 1},
            {node.x + 1, node.z - 1},
            {node.x - 1, node.z + 1},
            {node.x + 1, node.z + 1},
        };
        for(int j=0;j<sizeof(neighbours)/sizeof(*neighbours);j++) {
            Node& closeNode = neighbours[j];

            // Check if node/tile is open
            Tile* tile = world->tileFromGridPosition(closeNode.x, closeNode.z);
            if(!tile || tile->occupied)
                continue;

            int index_nearby = FindNode_nearby(closeNode);
            int index_visited = FindNode_visited(closeNode);
            int newCurCost = node.curCost + 1;
            if(index_visited != -1) {
                Node& visited = visitedNodes[index_visited];
                if(newCurCost < visited.curCost) {
                    visited.prevNode = nodeIndex;
                    visited.curCost = newCurCost;
                }
            } else if (index_nearby != -1) {
                Node& nearby = nearbyNodes[index_nearby];
                if(newCurCost < nearby.curCost) {
                    nearby.prevNode = nodeIndex;
                    nearby.curCost = newCurCost;
                }
            } else {
                nearbyNodes.add(closeNode);
                nearbyNodes.last().remCost = CostToEnd(nearbyNodes.last());
                nearbyNodes.last().prevNode = nodeIndex;
                nearbyNodes.last().curCost = newCurCost;
            }
        }
    }
    // Construct path from nodes
    Path* path = new Path();
    int index_nextNode = visitedNodes.size()-1;
    while(index_nextNode != -1) {
        Node& node = visitedNodes[index_nextNode];
        path->add(node.x, node.z);
        index_nextNode = node.prevNode;
    }
    // reverse
    // for(int j=0;j<path->points.size()/2;j++){
    //     glm::vec3 t = path->points[j];
    //     path->points[j] = path->points[path->points.size()-1-j];
    //     path->points[path->points.size()-1-j] = t;
    // }
    // Remove unnecessary nodes, straight nodes, 
    return path;
}
void Pathfinder::destroyPath(Path* path) {
    delete path;
}
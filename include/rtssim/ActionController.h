#pragma once

#include "Engone/Util/Array.h"

// struct Group : DynamicArray<u32> {
//     Group() = default;
//     ~Group() { cleanup(); }

//     // adds element if it doesn't exist
//     void add(u32 element) {
//         for(int i=0;i<used;i++){
//             if(element == _ptr[i]) {
//                 return;
//             }
//         }
//         DynamicArray<u32>::add(element);
//     }
//     // removes the value, not the value at a specified index.
//     void remove(u32 element) {
//         int index = -1;
//         for(int i=0;i<used;i++){
//             if(element == _ptr[i]) {
//                 index = i;
//                 break;
//             }
//         }
//         if(index == -1)
//             return;
//         DynamicArray<u32>::removeAt(index);
//     }
// };

// enum ActionType : u32 {
//     ACTION_MOVE,
// };
// struct EntityAction {
//     ActionType actionType;
//     u32 entityIndex;
//     union {
//         glm::vec3 movePosition;
//     };
// };
// struct ActionController {
//     ~ActionController() { cleanup(); }
//     void cleanup();
//     void addMove(Group* group, glm::vec3 position);

// };
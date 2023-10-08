#pragma once

enum ItemType : u32 {
    ITEM_NONE = 0,
    ITEM_WOOD = 1,  
    ITEM_STONE,  
    ITEM_IRON,
    ITEM_TYPE_MAX,
};

const char* ToString(ItemType itemType);

struct Item {
    ItemType type=ITEM_NONE;
    int amount=0;
};

struct Recipe {
    static const int ITEM_MAX = 10;
    int item_count=0;
    Item items[ITEM_MAX];
};
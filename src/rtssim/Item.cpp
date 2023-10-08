#include "rtssim/Item.h"

static const char* item_names[ITEM_TYPE_MAX]{
    "None",
    "Wood",
    "Stone",
    "Iron",
};

const char* ToString(ItemType itemType) {
    return item_names[itemType];
}
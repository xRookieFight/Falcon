#pragma once

#include "Item/Item.h"

#include <string>
#include <vector>

class VanillaItems {
public:
    static Item AIR();

    static Item STICK();

    static std::vector<Item> getAll();

    static const Item *fromIdentifier(const std::string &identifier);
};

#include "Item/VanillaItems.h"

#include "Item/ItemTypeIds.h"

Item VanillaItems::AIR() {
    return Item(ItemTypeIds::AIR, "minecraft:air", "Air");
}

Item VanillaItems::STICK() {
    return Item(ItemTypeIds::STICK, "minecraft:stick", "Stick");
}

std::vector<Item> VanillaItems::getAll() {
    std::vector<Item> items;
    items.push_back(AIR());
    items.push_back(STICK());
    return items;
}

const Item *VanillaItems::fromIdentifier(const std::string &identifier) {
    static const std::vector<Item> items = getAll();

    for (const Item &item: items) {
        if (item.getIdentifier() == identifier)
            return &item;
    }

    return nullptr;
}

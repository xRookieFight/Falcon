#pragma once

#include "Block/Block.h"
#include "Block/BlockTypeIds.h"

class CraftingTableBlock final : public Block {
public:
    CraftingTableBlock()
            : Block(BlockTypeIds::CRAFTING_TABLE, "minecraft:crafting_table", "Crafting Table") {}
};

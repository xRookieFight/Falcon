#pragma once

#include "Block/Block.h"
#include "Block/BlockTypeIds.h"

class SlimeBlock final : public Block {
public:
    SlimeBlock() : Block(BlockTypeIds::SLIME, "minecraft:slime", "Slime") {}
};

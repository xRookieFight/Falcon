#pragma once

#include "block/Block.h"
#include "block/BlockTypeIds.h"

class SlimeBlock final : public Block {
public:
    SlimeBlock() : Block(BlockTypeIds::SLIME, "minecraft:slime", "Slime") {}
};

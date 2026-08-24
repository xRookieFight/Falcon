#pragma once

#include "Block/Block.h"
#include "Block/BlockTypeIds.h"

class AirBlock final : public Block {
public:
    AirBlock() : Block(BlockTypeIds::AIR, "minecraft:air", "Air") {}
};

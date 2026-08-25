#pragma once

#include "block/Block.h"
#include "block/BlockTypeIds.h"

class AirBlock final : public Block {
public:
    AirBlock() : Block(BlockTypeIds::AIR, "minecraft:air", "Air") {}
};

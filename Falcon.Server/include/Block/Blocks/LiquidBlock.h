#pragma once

#include "Block/Block.h"

class LiquidBlock : public Block {
public:
    LiquidBlock(int32_t typeId, const std::string &identifier, const std::string &name)
            : Block(typeId, identifier, name) {}
};

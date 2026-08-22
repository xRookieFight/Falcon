#pragma once

#include "Block/Block.h"

#include <string>
#include <vector>

class VanillaBlocks {
public:
    static Block AIR();

    static Block BEDROCK();

    static Block GRASS();

    static Block STONE();

    static std::vector<Block> getAll();

    static const Block *fromIdentifier(const std::string &identifier);
};

#pragma once

#include <cstdint>

class BlockTypeIds {
public:
    static const int32_t AIR = 10000;
    static const int32_t BEDROCK = 10032;
    static const int32_t GRASS = 10282;
    static const int32_t STONE = 10496;

    static const int32_t FIRST_UNUSED_BLOCK_ID = 11000;
};

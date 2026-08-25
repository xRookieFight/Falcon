#pragma once

#include "block/blocks/LiquidBlock.h"

class BubbleColumnBlock : public LiquidBlock {
public:
    explicit BubbleColumnBlock(const BlockState &state) : LiquidBlock(state) {}

    int getTickRate() const override { return 5; }
};

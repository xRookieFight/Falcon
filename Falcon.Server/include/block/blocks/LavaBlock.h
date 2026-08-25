#pragma once

#include "block/blocks/LiquidBlock.h"

class LavaBlock : public LiquidBlock {
public:
    explicit LavaBlock(const BlockState &state) : LiquidBlock(state) {}
    explicit LavaBlock(const LiquidBlock &block) : LiquidBlock(block) {}

    int getTickRate() const override { return 30; }
    int getFlowDecayPerBlock() const override { return 2; }
    int getMinAdjacentSourcesToFormSource() const override { return 0; }
    const char *getBucketFillSound() const override { return "bucket_fill_lava"; }
    const char *getBucketEmptySound() const override { return "bucket_empty_lava"; }
};

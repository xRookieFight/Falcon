#pragma once

#include "Level/Generator/Nether/Feature/NetherFeature.h"

#include <vector>

class BlockManager;

class TwistingVinesClusterFeature : public NetherFeature {
public:
    const char *name() const override;

    void apply(ChunkGenerateContext &context) override;

private:
    static std::vector<int32_t> _getHighestWorkableBlocks(BlockManager &manager, int32_t x, int32_t z);

    static int32_t _getHighestEndingBlock(BlockManager &manager, int32_t x, int32_t y, int32_t z);
};

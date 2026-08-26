#pragma once

#include "Level/Generator/Feature/Tree/LegacyNetherTree.h"

class LegacyWarpedTree : public LegacyNetherTree {
protected:
    BlockState getTrunkBlockState() const override;

    BlockState getLeafBlockState() const override;
};

#pragma once

#include "Level/Generator/Feature/Tree/LegacyNetherTree.h"

class LegacyCrimsonTree : public LegacyNetherTree {
protected:
    BlockState getTrunkBlockState() const override;

    BlockState getLeafBlockState() const override;
};

#pragma once

#include "Level/Generator/Feature/Tree/LegacyTreeObject.h"

class LegacyJungleTree : public LegacyTreeObject {
public:
    TreeWoodType getType() const override;

    void placeObject(BlockManager &manager, int32_t x, int32_t y, int32_t z, IRandom &random) override;
};

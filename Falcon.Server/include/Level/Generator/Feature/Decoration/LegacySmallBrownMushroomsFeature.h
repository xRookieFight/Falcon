#pragma once

#include "Level/Generator/Feature/Decoration/LegacySmallMushroomsFeature.h"

class LegacySmallBrownMushroomsFeature : public LegacySmallMushroomsFeature {
public:
    const char *name() const override;

    BlockState getSourceBlock() const override;

    int32_t getBase() const override;

    int32_t getRandom() const override;
};

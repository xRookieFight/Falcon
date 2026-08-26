#pragma once

#include "Level/Generator/Feature/Terrain/CaveFeature.h"

class ExtraUndergroundCaveFeature : public CaveFeature {
public:
    const char *name() const override;

protected:
    float getCaveProbability() const override;

    int32_t getCaveMaxY() const override;
};

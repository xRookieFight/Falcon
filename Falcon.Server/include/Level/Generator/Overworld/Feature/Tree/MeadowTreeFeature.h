#pragma once

#include "Level/Generator/Overworld/Feature/Tree/PlainsTreeFeature.h"

class MeadowTreeFeature : public PlainsTreeFeature {
public:
    static const char *NAME;

    const char *name() const override;

    float getBeeNestChance() const override;

    bool canSpawnHere(int32_t biomeId) const override;
};

#pragma once

#include "Level/Generator/Nether/Feature/NyliumVegetationFeature.h"

class CrimsonVegetationFeature : public NyliumVegetationFeature {
public:
    int32_t getBiomeId() const override;

    const char *getNyliumIdentifier() const override;

    const BlockState &getFungusState() const override;

    const BlockState &getRootsState() const override;

    const char *name() const override;
};

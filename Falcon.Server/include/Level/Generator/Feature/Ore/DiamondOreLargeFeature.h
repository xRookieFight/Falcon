#pragma once

#include "Level/Generator/Feature/Ore/DiamondOreFeature.h"

class DiamondOreLargeFeature : public DiamondOreFeature {
public:
    int32_t getClusterCount() const override;

    int32_t getClusterSize() const override;

    float getSkipAir() const override;

    bool isRare() const override;

    const char *name() const override;
};

#pragma once

#include "Level/Generator/Feature/Ore/CopperOreFeature.h"

class CopperOreDripstoneCavesFeature : public CopperOreFeature {
public:
    int32_t getClusterSize() const override;

    const char *name() const override;
};

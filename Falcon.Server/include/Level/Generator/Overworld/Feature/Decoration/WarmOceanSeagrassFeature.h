#pragma once

#include "Level/Generator/Overworld/Feature/Decoration/SeagrassFeature.h"

class WarmOceanSeagrassFeature : public SeagrassFeature {
public:
    const char *name() const override;

    int32_t getBase() const override;
};

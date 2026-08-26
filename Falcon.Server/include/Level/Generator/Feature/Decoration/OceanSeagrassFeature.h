#pragma once

#include "Level/Generator/Feature/Decoration/SeagrassFeature.h"

class OceanSeagrassFeature : public SeagrassFeature {
public:
    const char *name() const override;
};

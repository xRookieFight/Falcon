#pragma once

#include "Level/Generator/Overworld/Feature/Ore/GraniteOreUpperFeature.h"

class AndesiteOreUpperFeature : public GraniteOreUpperFeature {
public:
    const BlockState &getState(const BlockState &original) const override;

    const char *name() const override;
};

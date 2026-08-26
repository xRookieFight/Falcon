#pragma once

#include "Level/Generator/Feature/Ore/GraniteOreLowerFeature.h"

class DioriteOreLowerFeature : public GraniteOreLowerFeature {
public:
    const BlockState &getState(const BlockState &original) const override;

    const char *name() const override;
};

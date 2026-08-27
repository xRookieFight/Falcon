#pragma once

#include "Level/Generator/Feature/FeatureBases.h"

class LegacySwampFoliageFeature : public MultiFeatureWrapper {
public:
    const char *name() const override;

    std::vector<const char *> getFeatures() const override;
};

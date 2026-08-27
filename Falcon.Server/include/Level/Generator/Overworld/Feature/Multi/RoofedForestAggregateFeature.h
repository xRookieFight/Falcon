#pragma once

#include "Level/Generator/Feature/FeatureBases.h"

class RoofedForestAggregateFeature : public MultiFeatureWrapper {
public:
    const char *name() const override;

    std::vector<const char *> getFeatures() const override;
};

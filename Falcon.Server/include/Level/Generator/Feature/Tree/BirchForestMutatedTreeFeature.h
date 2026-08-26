#pragma once

#include "Level/Generator/Feature/FeatureBases.h"

class BirchForestMutatedTreeFeature : public GriddedFeature {
public:
    static const char *NAME;

    const char *name() const override;

    ObjectGeneratorPtr getGenerator(IRandom &random) override;
};

#pragma once

#include "Level/Generator/Feature/FeatureBases.h"

class MesaPlateauStoneTreeFeature : public MultiFeatureWrapper {
public:
    static const char *NAME;

    const char *name() const override;

    std::vector<const char *> getFeatures() const override;
};

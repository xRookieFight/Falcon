#pragma once

#include "Level/Generator/Feature/Tree/SavannaTreeFeature.h"

class SavannaMutatedTreeFeature : public SavannaTreeFeature {
public:
    static const char *NAME;

    const char *name() const override;

    ObjectGeneratorPtr getGenerator(IRandom &random) override;
};

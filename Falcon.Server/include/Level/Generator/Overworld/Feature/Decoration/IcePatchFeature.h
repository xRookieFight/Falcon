#pragma once

#include "Level/Generator/Feature/FeatureBases.h"

class IcePatchFeature : public CountGenerateFeature {
public:
    const char *name() const override;

    int32_t getBase() const override;

    int32_t getRandom() const override;

    void populate(ChunkGenerateContext &context, IRandom &random) override;
};

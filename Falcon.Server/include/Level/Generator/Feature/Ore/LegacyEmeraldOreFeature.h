#pragma once

#include "Level/Generator/Feature/FeatureBases.h"

class LegacyEmeraldOreFeature : public CountGenerateFeature {
public:
    int32_t getBase() const override;

    int32_t getRandom() const override;

    void populate(ChunkGenerateContext &context, IRandom &random) override;

    const char *name() const override;
};

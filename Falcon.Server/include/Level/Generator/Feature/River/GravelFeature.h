#pragma once

#include "Level/Generator/Feature/River/DiscGenerateFeature.h"

class GravelFeature : public DiscGenerateFeature {
public:
    const BlockState &getSourceBlock() const override;

    int32_t getMinRadius() const override;

    int32_t getMaxRadius() const override;

    int32_t getRadiusY() const override;

    int32_t getBase() const override;

    const char *name() const override;
};

#pragma once

#include "block/components/BlockBehavior.h"

class SlimeBlockBehavior final : public BlockBehavior {
public:
    float getFrictionFactor() const override { return 0.8f; }

    bool onEntityLand(Actor &actor, float downwardVelocity) const override;

    std::optional<float> getFallDamage(const Actor &actor, float vanillaFallDamage) const override;
};

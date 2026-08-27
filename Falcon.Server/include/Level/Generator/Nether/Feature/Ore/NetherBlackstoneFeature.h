#pragma once

#include "Level/Generator/Nether/Feature/Ore/NetherGravelFeature.h"

class NetherBlackstoneFeature : public NetherGravelFeature {
public:
    const BlockState &getState() const override;

    const char *name() const override;
};

#pragma once

#include "Block/BlockState.h"
#include "Level/Generator/Nether/Feature/NetherFeature.h"

class NetherGravelFeature : public NetherFeature {
public:
    virtual const BlockState &getState() const;

    virtual int32_t getClusterCount() const;

    virtual int32_t getClusterSize() const;

    virtual int32_t getMinHeight() const;

    virtual int32_t getMaxHeight() const;

    const char *name() const override;

    void apply(ChunkGenerateContext &context) override;
};

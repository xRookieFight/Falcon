#pragma once

#include "Level/Generator/Feature/IFeature.h"
#include "Level/Generator/Feature/ObjectGenerator.h"

class MonsterRoomFeature : public IFeature {
public:
    const char *name() const override;

    void apply(ChunkGenerateContext &context) override;

    static void setGeneratorFactory(ObjectGeneratorPtr (*factory)());
};

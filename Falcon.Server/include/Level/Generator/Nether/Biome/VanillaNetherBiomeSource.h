#pragma once

#include "Level/Generator/Biome/BiomeSource.h"
#include "Level/Generator/Nether/Biome/NetherBiomeResult.h"
#include "Level/Generator/Noise/NormalNoise.h"
#include "Level/Generator/Random/IRandom.h"

#include <cstdint>

class VanillaNetherBiomeSource : public BiomeSource {
public:
    explicit VanillaNetherBiomeSource(IRandom &random);

    ~VanillaNetherBiomeSource() override = default;

    int32_t pickBiomeId(int32_t x, int32_t y, int32_t z) const override;

    NetherBiomeResult pick(int32_t x, int32_t y, int32_t z) const;

private:
    NormalNoisePtr mTemperatureNoise;
    NormalNoisePtr mHumidityNoise;
};

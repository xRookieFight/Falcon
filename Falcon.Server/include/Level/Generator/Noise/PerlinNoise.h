#pragma once

#include "Level/Generator/Noise/PerlinNoiseSampler.h"
#include "Level/Generator/Random/IRandom.h"

#include <memory>
#include <vector>

class PerlinNoise {
public:
    PerlinNoise(IRandom &random, int32_t firstOctave, const std::vector<double> &amplitudes);

    double getValue(double x, double y, double z) const;

    double maxValue() const;

private:
    static void _skipOctave(IRandom &random);

    double _edgeValue(double noiseValue) const;

    std::vector<std::shared_ptr<PerlinNoiseSampler>> mNoiseLevels;
    std::vector<double> mAmplitudes;
    double mLowestFreqValueFactor;
    double mLowestFreqInputFactor;
    double mMaxValue;
};

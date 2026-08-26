#pragma once

#include "Level/Generator/Noise/NormalNoise.h"

class NoiseHolder {
public:
    NoiseHolder();

    explicit NoiseHolder(NormalNoisePtr noise);

    double getValue(double x, double y, double z) const;

    double maxValue() const;

private:
    NormalNoisePtr mNoise;
};

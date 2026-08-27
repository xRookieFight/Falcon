#pragma once

#include "Level/Generator/Density/DensityFunction.h"
#include "Level/Generator/Random/IRandom.h"

class DensityNether {
public:
    static DensityFunctionPtr base3dNoise(IRandom &random);

    static DensityFunctionPtr finalDensity(DensityFunctionPtr base3dNoise);
};

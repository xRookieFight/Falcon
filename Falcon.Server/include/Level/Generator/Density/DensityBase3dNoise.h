#pragma once

#include "Level/Generator/Density/DensityCommon.h"
#include "Level/Generator/Random/IRandom.h"

class DensityBase3dNoise {
public:
    static DensityFunctionPtr oldBlendedNoise(IRandom &random, double xzScale, double yScale, double xzFactor,
                                              double yFactor, double smearScaleMultiplier);

    static DensityFunctionPtr overworld(IRandom &random);
};

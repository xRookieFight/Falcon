#pragma once

#include "Level/Generator/Density/DensityCommon.h"

class DensityErosion {
public:
    static DensityFunctionPtr overworldErosion(const NormalNoisePtr &erosion, DensityFunctionPtr shiftX,
                                               DensityFunctionPtr shiftZ);

    static DensityFunctionPtr overworldErosion(const NormalNoisePtr &erosion, const NormalNoisePtr &shiftNoise);
};

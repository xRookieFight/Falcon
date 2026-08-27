#pragma once

#include "Level/Generator/Density/DensityCommon.h"

class DensitySlopedCheese {
public:
    static DensityFunctionPtr overworldSlopedCheese(DensityFunctionPtr depth, DensityFunctionPtr jaggedness,
                                                    DensityFunctionPtr factor, DensityFunctionPtr base3dNoise,
                                                    const NormalNoisePtr &jaggedNoise);
};

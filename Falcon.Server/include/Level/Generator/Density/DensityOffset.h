#pragma once

#include "Level/Generator/Density/DensityCommon.h"

class DensityOffset {
public:
    static DensityFunctionPtr overworldOffset(const DensityFunctionPtr &continents, const DensityFunctionPtr &erosion,
                                              const DensityFunctionPtr &ridgesFolded);

private:
    static DensityFunctionPtr _offsetSpline(const DensityFunctionPtr &continents, const DensityFunctionPtr &erosion,
                                            const DensityFunctionPtr &ridgesFolded);
};

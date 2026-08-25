#pragma once

#include "core/utility/UUID.h"

#include <cstdint>
#include <string>

class DimensionDefinition {
public:
    std::string mId;
    int32_t mMaximumHeight = 0;
    int32_t mMinimumHeight = 0;
    int32_t mGeneratorType = 0;
    int32_t mDimensionType = 0;
    Uuid mPackId;
};

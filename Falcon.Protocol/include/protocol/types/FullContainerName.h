#pragma once

#include "protocol/types/ContainerSlotType.h"

#include <cstdint>

class FullContainerName {
public:
    ContainerSlotType mContainer = ContainerSlotType::Unknown;
    bool mHasDynamicId = false;
    int32_t mDynamicId = 0;
};

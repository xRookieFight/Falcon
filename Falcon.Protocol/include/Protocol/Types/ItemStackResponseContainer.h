#pragma once

#include "protocol/types/FullContainerName.h"
#include "protocol/types/ItemStackResponseSlot.h"

#include <vector>

class ItemStackResponseContainer {
public:
    FullContainerName mContainerName;
    std::vector<ItemStackResponseSlot> mItems;
};

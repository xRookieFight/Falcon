#pragma once

#include "block/components/BlockBehavior.h"

#include <string>

class BlockComponentFactory {
public:
    static const BlockBehavior &getBehavior(const std::string &identifier);
};

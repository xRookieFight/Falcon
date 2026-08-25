#pragma once

#include "Core/Math/Vector3i.h"
#include "Protocol/Types/ItemStack.h"

#include <cstdint>

enum class FurnaceKind : uint8_t {
    Furnace,
    BlastFurnace,
    Smoker
};

class FurnaceInventory {
public:
    static const int SLOT_INPUT = 0;
    static const int SLOT_FUEL = 1;
    static const int SLOT_OUTPUT = 2;
    static const int SIZE = 3;

    static int cookDuration(FurnaceKind kind);

    static bool matches(const Vector3i &position, const Vector3i &other) { return position == other; }
};

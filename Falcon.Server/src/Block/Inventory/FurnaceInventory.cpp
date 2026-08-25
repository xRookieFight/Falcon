#include "Block/Inventory/FurnaceInventory.h"

int FurnaceInventory::cookDuration(FurnaceKind kind) {
    return kind == FurnaceKind::Furnace ? 200 : 100;
}

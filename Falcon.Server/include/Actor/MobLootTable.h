#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct MobDrop {
    std::string mItemIdentifier;
    int32_t mCount;
};

namespace MobLootTable {

std::vector<MobDrop> getMobDrops(const std::string &identifier, bool onFire);

bool hasMobDrops(const std::string &identifier);

}

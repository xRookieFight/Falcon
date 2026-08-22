#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

struct ItemNetworkIdEntry {
    const char *mIdentifier;
    int32_t mNetworkId;
    bool mComponentBased;
    int32_t mVersion;
    const unsigned char *mComponentNbt;
    size_t mComponentNbtSize;
};

class ItemNetworkIdTable {
public:
    static const ItemNetworkIdEntry *getEntries();

    static size_t getCount();

    static const ItemNetworkIdEntry *find(const std::string &identifier);
};

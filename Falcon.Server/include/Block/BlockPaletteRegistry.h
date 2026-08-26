#pragma once

#include "Core/NBT/Tag.h"

#include <cstdint>
#include <string>
#include <unordered_map>

class BlockPaletteRegistry {
public:
    static BlockPaletteRegistry &getInstance();

    void initialize();

    bool isLoaded() const;

    const Tag *getDefaultStates(const std::string &identifier) const;

    bool getDefaultNetworkId(const std::string &identifier, int32_t &out) const;

private:
    BlockPaletteRegistry();

    bool mLoaded;
    std::unordered_map<std::string, Tag> mDefaultStates;
    std::unordered_map<std::string, int32_t> mDefaultNetworkId;
};

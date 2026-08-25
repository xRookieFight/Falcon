#pragma once

#include "Block/Block.h"
#include "Core/Math/Vector3i.h"

#include <string>

class ServerNetworkHandler;
class ServerPlayer;

class RedstoneRepeaterBlock final : public Block {
public:
    explicit RedstoneRepeaterBlock(const Block &block) : Block(block)
    {
    }

    bool onInteract(ServerNetworkHandler &owner, ServerPlayer &player, const Vector3i &position,
                    const BlockState &state) const override;

    static bool matches(const std::string &identifier);
};

class RedstoneComparatorBlock final : public Block {
public:
    explicit RedstoneComparatorBlock(const Block &block) : Block(block)
    {
    }

    bool onInteract(ServerNetworkHandler &owner, ServerPlayer &player, const Vector3i &position,
                    const BlockState &state) const override;

    static bool matches(const std::string &identifier);
};

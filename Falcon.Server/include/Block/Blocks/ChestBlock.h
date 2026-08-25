#pragma once

#include "Block/Block.h"

#include <string>

class ChestBlockActor;
class Level;

class ChestBlock : public Block {
public:
    explicit ChestBlock(const Block &base);

    static bool matches(const std::string &identifier);

    bool onInteract(ServerNetworkHandler &owner, ServerPlayer &player, const Vector3i &position,
                    const BlockState &state) const override;

    static ChestBlockActor &getOrCreate(Level &level, const Vector3i &position);

    static void onPlaced(Level &level, const Vector3i &position);

    static void onBroken(ServerNetworkHandler &owner, const Vector3i &position);
};

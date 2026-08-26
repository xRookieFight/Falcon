#pragma once

#include "Block/Block.h"

#include <memory>
#include <string>

class BlockActor;
class ItemStack;
class Level;
class ServerNetworkHandler;

class ItemFrameBlock : public Block {
public:
    explicit ItemFrameBlock(const Block &base);

    static bool matches(const std::string &identifier);

    static bool isGlow(const std::string &identifier);

    static std::unique_ptr<BlockActor> createBlockActor(const std::string &identifier);

    static bool canPlaceOn(Level &level, const Vector3i &position, int blockFace);

    bool onInteract(ServerNetworkHandler &owner, ServerPlayer &player, const Vector3i &position,
                    const BlockState &state) const override;

    BlockState applyPlacementOrientation(const BlockState &state, const BlockPlacementContext &context) const override;

    static bool onPunch(ServerNetworkHandler &owner, ServerPlayer &player, const Vector3i &position,
                        const BlockState &state);

    static void onPlaced(ServerNetworkHandler &owner, const Vector3i &position, const BlockState &state);

    static void onBroken(ServerNetworkHandler &owner, const Vector3i &position);
};

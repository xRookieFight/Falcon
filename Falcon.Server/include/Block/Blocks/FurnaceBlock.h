#pragma once

#include "block/Block.h"
#include "block/BlockTypeIds.h"
#include "block/inventory/FurnaceInventory.h"

#include "core/math/Vector3i.h"
#include "protocol/types/ContainerType.h"

class ServerNetworkHandler;
class ServerPlayer;

class FurnaceBlock final : public Block {
public:
    FurnaceBlock()
          : Block(BlockTypeIds::FURNACE, "minecraft:furnace", "Furnace") {}

    static bool matches(const BlockState &state);
    static FurnaceKind kind(const BlockState &state);
    static ContainerType containerType(const BlockState &state);
    static bool onInteract(ServerNetworkHandler &owner, ServerPlayer &player,
                           const Vector3i &position, const BlockState &state);

    int getFuelTime() const { return 0; }
};

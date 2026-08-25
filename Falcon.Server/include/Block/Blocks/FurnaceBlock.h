#pragma once

#include "Block/Block.h"
#include "Block/BlockTypeIds.h"
#include "Block/Inventory/FurnaceInventory.h"

#include "Core/Math/Vector3i.h"
#include "Protocol/Types/ContainerType.h"

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

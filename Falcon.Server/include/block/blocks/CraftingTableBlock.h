#pragma once

#include "block/Block.h"
#include "block/BlockTypeIds.h"

#include "core/math/Vector3i.h"

class ServerNetworkHandler;
class ServerPlayer;

class CraftingTableBlock final : public Block {
public:
    CraftingTableBlock()
          : Block(BlockTypeIds::CRAFTING_TABLE, "minecraft:crafting_table", "Crafting Table") {}

    static bool onInteract(ServerNetworkHandler &owner, ServerPlayer &player, const Vector3i &position);

    static bool matches(const BlockState &state) { return state.mName == "minecraft:crafting_table"; }

    int getFuelTime() const { return 300; }
};

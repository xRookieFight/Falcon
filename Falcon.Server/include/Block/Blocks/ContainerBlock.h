#pragma once

#include "Block/Block.h"
#include "Protocol/Types/ContainerType.h"

#include <memory>
#include <string>

class BlockActor;
class ItemStack;
class Level;
class ServerNetworkHandler;

enum class ContainerBlockKind {
    Barrel,
    ShulkerBox,
    EnderChest,
    Hopper,
    Dispenser,
    Dropper,
    BrewingStand,
    Beacon,
    EnchantTable,
    Crafter,
    Campfire,
    Lectern,
    ChiseledBookshelf,
    DecoratedPot,
    Jukebox,
    Shelf
};

struct ContainerBlockDefinition {
    ContainerBlockKind mKind;
    const char *mBlockActorId;
    ContainerType mContainerType;
    bool mOpensWindow;
    bool mDropsContentsOnBreak;
};

class ContainerBlock : public Block {
public:
    explicit ContainerBlock(const Block &base);

    static bool matches(const std::string &identifier);

    static const ContainerBlockDefinition *findDefinition(const std::string &identifier);

    static std::unique_ptr<BlockActor> createBlockActor(ContainerBlockKind kind);

    bool onInteract(ServerNetworkHandler &owner, ServerPlayer &player, const Vector3i &position,
                    const BlockState &state) const override;

    static void onPlaced(ServerNetworkHandler &owner, const Vector3i &position, const std::string &identifier,
                         const ItemStack &usedItem, int blockFace);

    static void onBroken(ServerNetworkHandler &owner, const Vector3i &position, const std::string &identifier);

    static bool keepsContentsInItem(const std::string &identifier);

    static void writeContentsToItem(const Vector3i &position, ItemStack &item);
};

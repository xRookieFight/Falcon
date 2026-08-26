#pragma once

#include "Core/Math/Vector3i.h"
#include "Inventory/Container.h"
#include "Protocol/Types/ContainerType.h"
#include "Protocol/Types/ItemStack.h"

#include <cstdint>
#include <array>
#include <vector>

class NetworkIdentifier;
class PacketSender;
class ServerPlayer;
class ServerNetworkHandler;
class ChestBlockActor;
enum class FurnaceKind : uint8_t;

class InventoryManager {
public:
    enum class InventoryId : int {
        Inventory = 0,
        Offhand = 1,
        Armor = 2,
        Cursor = 3,
        CraftingInput = 4,
        CraftingTableInput = 5,
        FurnaceInput = 6
    };

    static const int CONTAINER_ID_NONE = -1;
    static const int CONTAINER_ID_INVENTORY = 0;
    static const int CONTAINER_ID_FIRST = 1;
    static const int CONTAINER_ID_LAST = 100;
    static const int CONTAINER_ID_OFFHAND = 119;
    static const int CONTAINER_ID_ARMOR = 120;
    static const int CONTAINER_ID_UI = 124;
    static constexpr int CONTAINER_ID_REGISTRY = 125;

    static const int UI_SLOT_CURSOR = 0;

    InventoryManager();

    void attach(ServerPlayer *player, ServerNetworkHandler *owner);

    void syncAll();

    void syncContents(InventoryId inventory);

    void syncSlot(InventoryId inventory, int slot);

    void syncBundle(const ItemStack &bundle, int32_t bundleId, const std::vector<ItemStack> &contents);

    void syncBundles();

    void syncSelectedHotbarSlot();

    void onClientSelectHotbarSlot(int slot);

    void onClientOpenMainInventory();

    bool onClientOpenCraftingTable(const Vector3i &position);

    bool onClientOpenFurnace(const Vector3i &position, FurnaceKind kind, ContainerType type);

    bool openContainer(ContainerType type, const Vector3i &position);

    bool isCraftingTableOpen() const { return mCraftingTableWindowId != CONTAINER_ID_NONE; }

    bool isFurnaceOpen() const { return mFurnaceWindowId != CONTAINER_ID_NONE; }

    int getCraftingTableWindowId() const { return mCraftingTableWindowId; }

    int getFurnaceWindowId() const { return mFurnaceWindowId; }

    const Vector3i &getFurnacePosition() const { return mFurnacePosition; }

    FurnaceKind getFurnaceKind() const { return mFurnaceKind; }

    ContainerType getFurnaceContainerType() const { return mFurnaceContainerType; }

    void tickFurnace(ServerNetworkHandler &owner);

    static void tickStoredFurnaces(ServerNetworkHandler &owner);

    static void onFurnaceBroken(ServerNetworkHandler &owner, const Vector3i &position);

    bool onClientOpenChest(const Vector3i &position);

    bool isContainerOpen() const { return mContainerWindowId != CONTAINER_ID_NONE; }

    int getContainerWindowId() const { return mContainerWindowId; }

    const Vector3i &getContainerPosition() const { return mContainerPosition; }

    Container *getContainer();

    void syncCraftingTableState(const std::vector<ItemStack> &recipeOutputs,
                                const std::vector<uint32_t> &recipeSourceIndices);

    void syncCraftingPreview(const std::vector<ItemStack> &recipeOutputs,
                             const std::vector<uint32_t> &recipeSourceIndices);

    void onCurrentWindowRemove();

    void onClientRemoveWindow(int windowId);

    int getCurrentWindowId() const {
        return mFurnaceWindowId != CONTAINER_ID_NONE
               ? mFurnaceWindowId
               : (mCraftingTableWindowId != CONTAINER_ID_NONE
                      ? mCraftingTableWindowId
                      : (mMainInventoryWindowId != CONTAINER_ID_NONE ? mMainInventoryWindowId : mLastInventoryNetworkId));
    }

private:
    int _getNewWindowId();

    int _getWindowId(InventoryId inventory) const;

    int _getSize(InventoryId inventory) const;

    void _animateChest(ChestBlockActor &chest, bool open);

    void _sendContentPackets(int containerId, const Container &container);

    void _sendContentPackets(int containerId, InventoryId inventory);

    void _sendSlotPackets(int containerId, int netSlot, InventoryId inventory, int slot);

    void _sendOutputPacket(const ItemStack &item);

    void _storeFurnaceState(bool clearLocal);

    ServerPlayer *mPlayer;
    PacketSender *mSender;
    ServerNetworkHandler *mOwner = nullptr;
    int mLastInventoryNetworkId;
    ContainerType mCurrentWindowType;
    int mMainInventoryWindowId;
    int mCraftingTableWindowId;
    Vector3i mCraftingTablePosition;
    int mContainerWindowId = CONTAINER_ID_NONE;
    Vector3i mContainerPosition;
    int mFurnaceWindowId;
    Vector3i mFurnacePosition;
    FurnaceKind mFurnaceKind;
    ContainerType mFurnaceContainerType;
    int mFurnaceBurnTime;
    int mFurnaceMaxBurnTime;
    int mFurnaceCookTime;
    std::array<ItemStack, 3> mFurnaceObservedItems;
    int mClientSelectedHotbarSlot;
    bool mHasPendingCloseWindow;
    int mPendingCloseWindowId;
    bool mHasPendingOpenMainInventory;
};

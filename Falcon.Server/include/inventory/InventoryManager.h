#pragma once

#include "core/math/Vector3i.h"
#include "protocol/types/ContainerType.h"
#include "protocol/types/ItemStack.h"

#include <cstdint>
#include <array>
#include <vector>

class NetworkIdentifier;
class PacketSender;
class ServerPlayer;
class ServerNetworkHandler;
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

    static const int UI_SLOT_CURSOR = 0;

    InventoryManager();

    void attach(ServerPlayer *player, PacketSender *sender);

    void syncAll();

    void syncContents(InventoryId inventory);

    void syncSlot(InventoryId inventory, int slot);

    void syncSelectedHotbarSlot();

    void onClientSelectHotbarSlot(int slot);

    void onClientOpenMainInventory();

    bool onClientOpenCraftingTable(const Vector3i &position);

    bool onClientOpenFurnace(const Vector3i &position, FurnaceKind kind, ContainerType type);

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

    void _sendContentPackets(int containerId, InventoryId inventory);

    void _sendSlotPackets(int containerId, int netSlot, InventoryId inventory, int slot);

    void _sendOutputPacket(const ItemStack &item);

    void _storeFurnaceState(bool clearLocal);

    ServerPlayer *mPlayer;
    PacketSender *mSender;
    int mLastInventoryNetworkId;
    ContainerType mCurrentWindowType;
    int mMainInventoryWindowId;
    int mCraftingTableWindowId;
    Vector3i mCraftingTablePosition;
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

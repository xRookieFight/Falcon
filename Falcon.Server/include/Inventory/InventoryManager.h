#pragma once

#include "Protocol/Types/ContainerType.h"

#include <cstdint>

class NetworkIdentifier;
class PacketSender;
class ServerPlayer;

class InventoryManager {
public:
    enum class InventoryId : int {
        Inventory = 0,
        Offhand = 1,
        Armor = 2,
        Cursor = 3
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

    void onCurrentWindowRemove();

    void onClientRemoveWindow(int windowId);

    int getCurrentWindowId() const { return mLastInventoryNetworkId; }

private:
    int _getNewWindowId();

    int _getWindowId(InventoryId inventory) const;

    int _getSize(InventoryId inventory) const;

    void _sendContentPackets(int containerId, InventoryId inventory);

    void _sendSlotPackets(int containerId, int netSlot, InventoryId inventory, int slot);

    ServerPlayer *mPlayer;
    PacketSender *mSender;
    int mLastInventoryNetworkId;
    ContainerType mCurrentWindowType;
    int mMainInventoryWindowId;
    int mClientSelectedHotbarSlot;
    bool mHasPendingCloseWindow;
    int mPendingCloseWindowId;
    bool mHasPendingOpenMainInventory;
};

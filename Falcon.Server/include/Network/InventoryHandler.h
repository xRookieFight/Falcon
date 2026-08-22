#pragma once

class ServerNetworkHandler;
class ServerPlayer;
class NetworkIdentifier;
class MobEquipmentPacket;
class PlayerHotbarPacket;
class ItemStackRequestPacket;
class InventoryTransactionPacket;
class ContainerClosePacket;

class InventoryHandler {
public:
    static void sendInventory(ServerNetworkHandler &owner, ServerPlayer &player);

    static void sendArmorContent(ServerNetworkHandler &owner, ServerPlayer &player);

    static void sendOffhandContent(ServerNetworkHandler &owner, ServerPlayer &player);

    static void sendHeldItem(ServerNetworkHandler &owner, ServerPlayer &player);

    static void handleMobEquipment(ServerNetworkHandler &owner, ServerPlayer &player,
                                   const MobEquipmentPacket &packet);

    static void handlePlayerHotbar(ServerNetworkHandler &owner, ServerPlayer &player,
                                   const PlayerHotbarPacket &packet);

    static void handleOpenInventory(ServerPlayer &player);

    static void handleItemStackRequest(ServerNetworkHandler &owner, const NetworkIdentifier &id, ServerPlayer &player,
                                       const ItemStackRequestPacket &packet);

    static void handleTransaction(ServerNetworkHandler &owner, ServerPlayer &player,
                                  const InventoryTransactionPacket &packet);

    static void handleContainerClose(ServerPlayer &player, const ContainerClosePacket &packet);
};

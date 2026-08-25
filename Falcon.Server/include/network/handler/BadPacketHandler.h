#pragma once

#include <string>

class ServerPlayer;
class PlayerAuthInputPacket;
class InventoryTransactionPacket;
class ItemStackRequestPacket;
class MobEquipmentPacket;
class RequestChunkRadiusPacket;

class BadPacketHandler {
public:
    static bool inspect(ServerPlayer &player, const PlayerAuthInputPacket &packet, std::string &outReason);

    static bool inspect(ServerPlayer &player, const InventoryTransactionPacket &packet, std::string &outReason);

    static bool inspect(ServerPlayer &player, const ItemStackRequestPacket &packet, std::string &outReason);

    static bool inspect(ServerPlayer &player, const MobEquipmentPacket &packet, std::string &outReason);

    static bool inspect(ServerPlayer &player, const RequestChunkRadiusPacket &packet, std::string &outReason);
};

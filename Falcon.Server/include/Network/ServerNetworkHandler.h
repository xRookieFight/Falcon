#pragma once

#include "Level/BiomeRegistry.h"
#include "Level/Level.h"
#include "Network/NetworkHandler.h"
#include "Network/PingedCompatibleServer.h"
#include "Network/RakNetInstance.h"
#include "Protocol/NetworkPacketHandler.h"
#include "Protocol/PacketCodecContext.h"
#include "Protocol/Types/BlockDefinitionRegistry.h"
#include "Protocol/Types/ItemDefinitionRegistry.h"
#include "Command/CommandMap.h"
#include "Network/PacketSender.h"
#include "Protocol/Types/CommandOriginData.h"
#include "Player/PlayerDataProvider.h"
#include "Server/OpList.h"
#include "Server/PropertiesSettings.h"
#include "Server/ResourcePackManager.h"
#include "Entity/ServerPlayer.h"
#include "Protocol/Types/ContainerSlotType.h"
#include "Protocol/Packets/CreativeContentPacket.h"

#include <mutex>
#include <queue>
#include <unordered_map>
#include <memory>
#include <string>

class ItemUseTransaction;

class ServerNetworkHandler : public NetworkHandler::Listener,
                             public NetworkPacketHandler,
                             public RakPeerHelper::IPSupportInterface,
                             public PacketSender {
public:
    ServerNetworkHandler(const std::string &serverName, const std::string &subName, int maxPlayers);

    ~ServerNetworkHandler() override;

    bool startServerListening(const ConnectionDefinition &definition);

    void stopServerListening();

    void tick();

    void setMotd(const std::string &serverName, const std::string &subName);

    void setProtocolVersion(int protocolVersion, const std::string &gameVersion);

    void setProperties(const PropertiesSettings &properties);

    const PropertiesSettings &getProperties() const { return mProperties; }

    int getActivePlayerCount() const { return (int) mNetworkHandler->getConnectionCount(); }

    int getMaxPlayers() const { return mMaxPlayers; }

    NetworkHandler &getNetworkHandler() { return *mNetworkHandler; }

    BlockDefinitionRegistry &getBlockDefinitions() { return mBlockDefinitions; }

    ItemDefinitionRegistry &getItemDefinitions() { return mItemDefinitions; }

    const PacketCodecContext &getCodecContext() const { return mCodecContext; }

    ServerPlayer *getPlayerByName(const std::string &name);

    void setPlayerGameMode(ServerPlayer &player, int gameMode);

    void setPlayerOp(ServerPlayer &player, bool isOp);

    OpList &getOpList() { return mOps; }

    void sendCommandOutput(ServerPlayer &player, const CommandOriginData &origin, const std::string &message);

    void broadcastSystemMessage(const std::string &message);

    void sendPacketTo(const NetworkIdentifier &id, const Packet &packet) override;

    void queueConsoleCommand(const std::string &commandLine);

private:
    bool onValidateIncomingConnection(const NetworkIdentifier &id) override;

    void onNewIncomingConnection(const NetworkIdentifier &id) override;

    void onConnectionClosed(const NetworkIdentifier &id, DisconnectFailReason reason,
                            const std::string &message) override;

    void onDataReceived(const NetworkIdentifier &id, const std::string &data) override;

    void handle(const NetworkIdentifier &id, const RequestNetworkSettingsPacket &packet) override;

    void handle(const NetworkIdentifier &id, const LoginPacket &packet) override;

    void handle(const NetworkIdentifier &id, const ResourcePackClientResponsePacket &packet) override;

    void handle(const NetworkIdentifier &id, const ResourcePackChunkRequestPacket &packet) override;

    void handle(const NetworkIdentifier &id, const SetLocalPlayerAsInitializedPacket &packet) override;

    void handle(const NetworkIdentifier &id, const PlayerAuthInputPacket &packet) override;

    void handle(const NetworkIdentifier &id, const CommandRequestPacket &packet) override;

    void handle(const NetworkIdentifier &id, const TextPacket &packet) override;

    void handle(const NetworkIdentifier &id, const PacketViolationWarningPacket &packet) override;

    void handle(const NetworkIdentifier &id, const RequestChunkRadiusPacket &packet) override;

    void handle(const NetworkIdentifier &id, const RequestAbilityPacket &packet) override;

    void handle(const NetworkIdentifier &id, const MobEquipmentPacket &packet) override;

    void handle(const NetworkIdentifier &id, const PlayerHotbarPacket &packet) override;

    void handle(const NetworkIdentifier &id, const ItemStackRequestPacket &packet) override;

    void handle(const NetworkIdentifier &id, const InventoryTransactionPacket &packet) override;

    void handle(const NetworkIdentifier &id, const ContainerClosePacket &packet) override;

    void handle(const NetworkIdentifier &id, const InteractPacket &packet) override;

    ServerPlayer *_getPlayer(const NetworkIdentifier &id);

    int _getServerViewDistance() const;

    void _registerVanillaDefinitions();

    void _sendStartGame(ServerPlayer &player);

    void _breakBlock(ServerPlayer &player, const Vector3i &position);

    void _placeBlock(ServerPlayer &player, const ItemUseTransaction &transaction);

    void _savePlayerData(const ServerPlayer &player);

    void _loadPlayerData(ServerPlayer &player);

    void _sendEntityData(ServerPlayer &player);

    void _sendAbilities(ServerPlayer &player);

    void _addToPlayerList(ServerPlayer &player);

    void _removeFromPlayerList(ServerPlayer &player);

    void _sendAttributes(ServerPlayer &player);

    void _sendChunks(ServerPlayer &player);

    void _checkTerrainReady(ServerPlayer &player);

    void _sendBiomeDefinitions(ServerPlayer &player);

    void _sendItemComponents(ServerPlayer &player);

    void _sendActorIdentifiers(ServerPlayer &player);

    void _sendCraftingData(ServerPlayer &player);

    void _buildCreativeContent();

    void _sendCreativeContent(ServerPlayer &player);

    void _sendInventory(ServerPlayer &player);

    void _sendArmorContent(ServerPlayer &player);

    void _sendOffhandContent(ServerPlayer &player);

    void _sendHeldItem(ServerPlayer &player);

    void _sendAvailableCommands(ServerPlayer &player);

    void _disconnect(const NetworkIdentifier &id, const std::string &reason);

    void onReceiveIPSupport(RakPeerHelper::IPSupport support) override;

    void _updateServerAnnouncement();

    std::unique_ptr<NetworkHandler> mNetworkHandler;
    RakNetInstance *mRakNetInstance;
    BlockDefinitionRegistry mBlockDefinitions;
    ItemDefinitionRegistry mItemDefinitions;
    PacketCodecContext mCodecContext;

    PropertiesSettings mProperties;
    static const int DEFAULT_VIEW_DISTANCE = 4;
    static const int MAX_VIEW_DISTANCE = 12;
    static const unsigned CHUNKS_PER_TICK = 32;
    static const size_t SPAWN_CHUNK_THRESHOLD = 56;

    Level mLevel;
    BiomeRegistry mBiomes;
    std::vector<CreativeItemGroup> mCreativeGroups;
    std::vector<CreativeItemData> mCreativeItems;
    PlayerDataProvider mPlayerData;
    OpList mOps;
    ResourcePackManager mResourcePacks;
    CommandMap mCommands;
    PingedCompatibleServer mAnnouncement;
    int mMaxPlayers;
    bool mIsListening;

    std::unordered_map<NetworkIdentifier, ServerPlayer, NetworkIdentifier::Hasher> mPlayers;
    uint64_t mNextRuntimeId;

    std::mutex mConsoleQueueMutex;
    std::queue<std::string> mConsoleQueue;
};

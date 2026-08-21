#pragma once

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
#include "Server/PropertiesSettings.h"
#include "Entity/ServerPlayer.h"

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

    void sendCommandOutput(ServerPlayer &player, const CommandOriginData &origin, const std::string &message);

    void broadcastSystemMessage(const std::string &message);

    void sendPacketTo(const NetworkIdentifier &id, const Packet &packet) override;

private:
    bool onValidateIncomingConnection(const NetworkIdentifier &id) override;

    void onNewIncomingConnection(const NetworkIdentifier &id) override;

    void onConnectionClosed(const NetworkIdentifier &id, DisconnectFailReason reason,
                            const std::string &message) override;

    void onDataReceived(const NetworkIdentifier &id, const std::string &data) override;

    void handle(const NetworkIdentifier &id, const RequestNetworkSettingsPacket &packet) override;

    void handle(const NetworkIdentifier &id, const LoginPacket &packet) override;

    void handle(const NetworkIdentifier &id, const ResourcePackClientResponsePacket &packet) override;

    void handle(const NetworkIdentifier &id, const SetLocalPlayerAsInitializedPacket &packet) override;

    void handle(const NetworkIdentifier &id, const PlayerAuthInputPacket &packet) override;

    void handle(const NetworkIdentifier &id, const CommandRequestPacket &packet) override;

    void handle(const NetworkIdentifier &id, const TextPacket &packet) override;

    ServerPlayer *_getPlayer(const NetworkIdentifier &id);

    void _sendStartGame(ServerPlayer &player);

    void _breakBlock(ServerPlayer &player, const Vector3i &position);

    void _placeBlock(ServerPlayer &player, const ItemUseTransaction &transaction);

    void _savePlayerData(const ServerPlayer &player);

    void _loadPlayerData(ServerPlayer &player);

    void _sendEntityData(ServerPlayer &player);

    void _sendAttributes(ServerPlayer &player);

    void _sendChunks(ServerPlayer &player);

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

    Level mLevel;
    PlayerDataProvider mPlayerData;
    CommandMap mCommands;
    PingedCompatibleServer mAnnouncement;
    int mMaxPlayers;
    bool mIsListening;

    std::unordered_map<NetworkIdentifier, ServerPlayer, NetworkIdentifier::Hasher> mPlayers;
    uint64_t mNextRuntimeId;
};

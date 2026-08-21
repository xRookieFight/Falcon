#include "Network/ServerNetworkHandler.h"

#include "Core/Debug/BedrockLog.h"
#include "Network/ConnectionRequest.h"
#include "Core/Utility/ReadOnlyBinaryStream.h"
#include "Protocol/MinecraftPackets.h"
#include "Protocol/Packets/DisconnectPacket.h"
#include "Protocol/Packets/LoginPacket.h"
#include "Protocol/Packets/NetworkSettingsPacket.h"
#include "Protocol/Packets/PlayStatusPacket.h"
#include "Protocol/Packets/RequestNetworkSettingsPacket.h"
#include "Protocol/Packets/ResourcePackClientResponsePacket.h"
#include "Protocol/Packets/ResourcePackStackPacket.h"
#include "Protocol/Packets/ResourcePacksInfoPacket.h"
#include "Protocol/Packets/SetLocalPlayerAsInitializedPacket.h"
#include "Protocol/Packets/StartGamePacket.h"

ServerNetworkHandler::ServerNetworkHandler(const std::string &serverName, const std::string &subName, int maxPlayers)
        : mRakNetInstance(nullptr), mMaxPlayers(maxPlayers), mIsListening(false), mNextRuntimeId(1) {
    std::unique_ptr<RakNetInstance> instance(new RakNetInstance(*this, true));
    mRakNetInstance = instance.get();

    mNetworkHandler.reset(new NetworkHandler(std::move(instance)));
    mNetworkHandler->addListener(this);

    mAnnouncement.mServerName = serverName;
    mAnnouncement.mSubName = subName;
    mAnnouncement.mGameMode = "Survival";
    mAnnouncement.mGameModeId = 1;
    mAnnouncement.mMaxPlayers = maxPlayers;
}

ServerNetworkHandler::~ServerNetworkHandler() {
    stopServerListening();
    mNetworkHandler->removeListener(this);
}

void ServerNetworkHandler::setMotd(const std::string &serverName, const std::string &subName) {
    mAnnouncement.mServerName = serverName;
    mAnnouncement.mSubName = subName;

    if (mIsListening)
        _updateServerAnnouncement();
}

void ServerNetworkHandler::setProtocolVersion(int protocolVersion, const std::string &gameVersion) {
    mAnnouncement.mProtocolVersion = protocolVersion;
    mAnnouncement.mGameVersion = gameVersion;

    if (mIsListening)
        _updateServerAnnouncement();
}

void ServerNetworkHandler::setProperties(const PropertiesSettings &properties) {
    mProperties = properties;

    switch (properties.getGameType()) {
        case GameType::Creative:
            mAnnouncement.mGameMode = "Creative";
            break;
        case GameType::Adventure:
            mAnnouncement.mGameMode = "Adventure";
            break;
        default:
            mAnnouncement.mGameMode = "Survival";
            break;
    }

    mAnnouncement.mGameModeId = (int) properties.getGameType();

    if (mIsListening)
        _updateServerAnnouncement();
}

bool ServerNetworkHandler::startServerListening(const ConnectionDefinition &definition) {
    if (mIsListening)
        return false;

    if (!mNetworkHandler->host(definition)) {
        LOG_ERROR(LogAreaID::Network, "Failed to bind UDP port %u", definition.mPort);
        return false;
    }

    mMaxPlayers = definition.mMaxNumPlayers;
    mAnnouncement.mMaxPlayers = mMaxPlayers;
    mIsListening = true;

    _updateServerAnnouncement();
    return true;
}

void ServerNetworkHandler::stopServerListening() {
    if (!mIsListening)
        return;

    mNetworkHandler->disconnect();
    mIsListening = false;
}

void ServerNetworkHandler::tick() {
    if (!mIsListening)
        return;

    mNetworkHandler->runEvents();
    _updateServerAnnouncement();
}

void ServerNetworkHandler::_updateServerAnnouncement() {
    mAnnouncement.mCurrentPlayers = getActivePlayerCount();
    mRakNetInstance->announceServer(mAnnouncement);
}

bool ServerNetworkHandler::onValidateIncomingConnection(const NetworkIdentifier &id) {
    (void) id;
    return getActivePlayerCount() < mMaxPlayers;
}

void ServerNetworkHandler::onNewIncomingConnection(const NetworkIdentifier &id) {
    LOG_INFO(LogAreaID::Network, "Player connected: %s", id.toString().c_str());
}

void ServerNetworkHandler::onConnectionClosed(const NetworkIdentifier &id, DisconnectFailReason reason,
                                              const std::string &message) {
    (void) message;
    mPlayers.erase(id);
    LOG_INFO(LogAreaID::Network, "Player disconnected: %s, reason: %s", id.toString().c_str(), toString(reason));
}

void ServerNetworkHandler::onDataReceived(const NetworkIdentifier &id, const std::string &data) {
    try {
        ReadOnlyBinaryStream stream(data);

        unsigned char senderSubId;
        unsigned char clientSubId;
        const MinecraftPacketIds packetId = Packet::peekId(stream, senderSubId, clientSubId);

        std::shared_ptr<Packet> packet = MinecraftPackets::createPacket(packetId);
        if (!packet) {
            LOG_TRACE(LogAreaID::Network, "Unhandled packet id %d from %s", (int) packetId, id.getAddress().c_str());
            return;
        }

        packet->mSenderSubId = senderSubId;
        packet->mClientSubId = clientSubId;
        packet->read(stream);

        LOG_TRACE(LogAreaID::Network, "Received %s from %s", packet->getName(), id.getAddress().c_str());
        packet->handle(id, *this);
    } catch (const BinaryDataException &exception) {
        LOG_WARN(LogAreaID::Network, "Malformed packet from %s: %s", id.getAddress().c_str(), exception.what());
    }
}

ServerPlayer *ServerNetworkHandler::_getPlayer(const NetworkIdentifier &id) {
    auto it = mPlayers.find(id);
    return it == mPlayers.end() ? nullptr : &it->second;
}

void ServerNetworkHandler::_disconnect(const NetworkIdentifier &id, const std::string &reason) {
    DisconnectPacket disconnect;
    disconnect.mReason = 0;
    disconnect.mMessageSkipped = false;
    disconnect.mKickMessage = reason;

    mNetworkHandler->send(id, disconnect);
    mNetworkHandler->flush(id);
}

void ServerNetworkHandler::handle(const NetworkIdentifier &id, const RequestNetworkSettingsPacket &packet) {
    LOG_INFO(LogAreaID::Network, "%s requested network settings, client protocol version %d",
             id.getAddress().c_str(), packet.mProtocolVersion);

    if (packet.mProtocolVersion != mAnnouncement.mProtocolVersion) {
        PlayStatusPacket status;
        status.mStatus = packet.mProtocolVersion < mAnnouncement.mProtocolVersion
                         ? PlayStatusPacket::Status::LoginFailedClientOld
                         : PlayStatusPacket::Status::LoginFailedServerOld;

        mNetworkHandler->send(id, status);
        mNetworkHandler->flush(id);

        LOG_WARN(LogAreaID::Network, "%s uses protocol %d but the server runs %d", id.getAddress().c_str(),
                 packet.mProtocolVersion, mAnnouncement.mProtocolVersion);
        return;
    }

    NetworkSettingsPacket settings;
    settings.mCompressionThreshold = mProperties.getCompressionThreshold();
    settings.mCompressionAlgorithm = mProperties.getCompressionAlgorithm();
    settings.mClientThrottleEnabled = false;
    settings.mClientThrottleThreshold = 0;
    settings.mClientThrottleScalar = 0.0f;

    mNetworkHandler->send(id, settings);

    // the settings themselves travel uncompressed, everything after them does not
    mNetworkHandler->flush(id);
    mNetworkHandler->enableCompression(id, CompressedNetworkPeer::CompressionAlgorithm::ZLib,
                                       settings.mCompressionThreshold);

    mPlayers.erase(id);
    ServerPlayer player(id, mNextRuntimeId++);
    player.setLoginState(ServerPlayer::LoginState::NetworkSettingsSent);
    mPlayers.insert(std::make_pair(id, player));
}

void ServerNetworkHandler::handle(const NetworkIdentifier &id, const LoginPacket &packet) {
    ServerPlayer *player = _getPlayer(id);
    if (player == nullptr) {
        _disconnect(id, "Unexpected login");
        return;
    }

    ConnectionRequest request;
    if (!request.parse(packet.mAuthJwt, packet.mClientJwt)) {
        LOG_WARN(LogAreaID::Network, "%s sent a login that could not be parsed", id.getAddress().c_str());
        _disconnect(id, "Malformed login");
        return;
    }

    player->setName(request.getDisplayName());
    player->setUuid(request.getIdentity());
    player->setXuid(request.getXuid());
    player->setLoginState(ServerPlayer::LoginState::LoggedIn);

    LOG_INFO(LogAreaID::Server, "Player %s logged in, uuid %s, xuid %s", player->getName().c_str(),
             player->getUuid().c_str(), player->getXuid().c_str());

    PlayStatusPacket status;
    status.mStatus = PlayStatusPacket::Status::LoginSuccess;
    mNetworkHandler->send(id, status);

    ResourcePacksInfoPacket packs;
    packs.mForcedToAccept = false;
    packs.mWorldTemplateVersion = "";
    mNetworkHandler->send(id, packs);

    player->setLoginState(ServerPlayer::LoginState::ResourcePacksSent);
}

void ServerNetworkHandler::handle(const NetworkIdentifier &id, const ResourcePackClientResponsePacket &packet) {
    ServerPlayer *player = _getPlayer(id);
    if (player == nullptr)
        return;

    switch (packet.mStatus) {
        case ResourcePackClientResponsePacket::Status::HaveAllPacks: {
            ResourcePackStackPacket stack;
            stack.mForcedToAccept = false;
            stack.mGameVersion = mAnnouncement.mGameVersion;
            mNetworkHandler->send(id, stack);
            break;
        }

        case ResourcePackClientResponsePacket::Status::Completed:
            _sendStartGame(*player);
            break;

        case ResourcePackClientResponsePacket::Status::Refused:
            _disconnect(id, "You must accept the resource packs to join");
            break;

        default:
            break;
    }
}

void ServerNetworkHandler::_sendStartGame(ServerPlayer &player) {
    const NetworkIdentifier &id = player.getNetworkIdentifier();

    StartGamePacket startGame;
    startGame.mUniqueEntityId = player.getUniqueId();
    startGame.mRuntimeEntityId = player.getRuntimeId();
    startGame.mPlayerGameType = mProperties.getGameType();
    startGame.mPlayerPosition = Vector3f(0.5f, 70.0f, 0.5f);
    startGame.mRotation = Vector2f(0.0f, 0.0f);

    startGame.mSeed = 0;
    startGame.mDimensionId = 0;
    startGame.mGeneratorId = 1;
    startGame.mLevelGameType = mProperties.getGameType();
    startGame.mDifficulty = (int32_t) mProperties.getDifficulty();
    startGame.mDefaultSpawn = Vector3i(0, 70, 0);
    startGame.mCommandsEnabled = mProperties.getAllowCheats();
    startGame.mTexturePacksRequired = mProperties.getTexturePackRequired();
    startGame.mDefaultPlayerPermission = mProperties.getDefaultPlayerPermissionLevel();
    startGame.mChatRestrictionLevel = mProperties.getChatRestrictionLevel();
    startGame.mDisablingPlayerInteractions = mProperties.getDisablePlayerInteraction();
    startGame.mClientSideGenerationEnabled = mProperties.getClientSideChunkGenerationEnabled();
    startGame.mDisablingCustomSkins = mProperties.getDisableCustomSkins();
    startGame.mServerChunkTickRange = mProperties.getTickDistance();
    startGame.mVanillaVersion = mAnnouncement.mGameVersion;
    startGame.mLevelId = "RmFsY29u";
    startGame.mLevelName = mProperties.getLevelName();
    startGame.mMultiplayerCorrelationId = "";
    startGame.mServerEngine = "Falcon";
    startGame.mCurrentTick = 0;
    startGame.mEnchantmentSeed = 0;
    startGame.mBlockNetworkIdsHashed = mProperties.getBlockNetworkIdsAreHashes();
    startGame.mInventoriesServerAuthoritative = true;

    startGame.mGamerules.push_back(GameRuleData::ofBool("showcoordinates", true));

    mNetworkHandler->send(id, startGame);

    PlayStatusPacket spawn;
    spawn.mStatus = PlayStatusPacket::Status::PlayerSpawn;
    mNetworkHandler->send(id, spawn);

    player.setLoginState(ServerPlayer::LoginState::StartGameSent);
    LOG_INFO(LogAreaID::Server, "Sent StartGame to %s", player.getName().c_str());
}

void ServerNetworkHandler::handle(const NetworkIdentifier &id, const SetLocalPlayerAsInitializedPacket &packet) {
    (void) packet;

    ServerPlayer *player = _getPlayer(id);
    if (player == nullptr)
        return;

    player->setLoginState(ServerPlayer::LoginState::Spawned);
    LOG_INFO(LogAreaID::Server, "Player %s spawned", player->getName().c_str());
}

void ServerNetworkHandler::onReceiveIPSupport(RakPeerHelper::IPSupport support) {
    const ConnectionDefinition &definition = mRakNetInstance->getConnectionDefinition();

    if (support == RakPeerHelper::IPSupport::IPv4 || support == RakPeerHelper::IPSupport::Both)
        LOG_INFO(LogAreaID::Network, "IPv4 supported, port: %u", definition.mPort);

    if (support == RakPeerHelper::IPSupport::IPv6 || support == RakPeerHelper::IPSupport::Both)
        LOG_INFO(LogAreaID::Network, "IPv6 supported, port: %u", definition.mPortV6);
}

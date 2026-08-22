#include "Network/ServerNetworkHandler.h"

#include "Command/ConsoleCommandSender.h"
#include "Command/DeopCommand.h"
#include "Command/GameModeCommand.h"
#include "Command/OpCommand.h"
#include "Command/PlayerCommandSender.h"
#include "Core/Debug/BedrockLog.h"
#include "Network/ConnectionRequest.h"
#include "Network/LoginChainVerifier.h"
#include "Core/Utility/ReadOnlyBinaryStream.h"
#include "Protocol/MinecraftPackets.h"
#include "Protocol/Packets/DisconnectPacket.h"
#include "Protocol/Packets/LevelChunkPacket.h"
#include "Protocol/Packets/LoginPacket.h"
#include "Protocol/Packets/NetworkChunkPublisherUpdatePacket.h"
#include "Protocol/Packets/NetworkSettingsPacket.h"
#include "Protocol/Packets/PlayStatusPacket.h"
#include "Protocol/Packets/RequestChunkRadiusPacket.h"
#include "Protocol/Packets/RequestAbilityPacket.h"
#include "Protocol/Packets/ChunkRadiusUpdatedPacket.h"
#include "Protocol/Packets/RequestNetworkSettingsPacket.h"
#include "Protocol/Packets/ResourcePackClientResponsePacket.h"
#include "Protocol/Packets/ResourcePackStackPacket.h"
#include "Protocol/Packets/ResourcePacksInfoPacket.h"
#include "Protocol/Packets/SetLocalPlayerAsInitializedPacket.h"
#include "Protocol/Packets/PlayerAuthInputPacket.h"
#include "Protocol/Packets/AvailableCommandsPacket.h"
#include "Protocol/Packets/BiomeDefinitionListPacket.h"
#include "Protocol/Packets/CreativeContentPacket.h"
#include "Protocol/Packets/CommandOutputPacket.h"
#include "Protocol/Packets/CommandRequestPacket.h"
#include "Protocol/Packets/SetEntityDataPacket.h"
#include "Protocol/Packets/SetPlayerGameTypePacket.h"
#include "Protocol/Packets/PacketViolationWarningPacket.h"
#include "Protocol/Packets/PlayerListPacket.h"
#include "Protocol/Packets/TextPacket.h"
#include "Protocol/Packets/UpdateAbilitiesPacket.h"
#include "Entity/PlayerAbility.h"
#include "Protocol/Packets/UpdateBlockPacket.h"
#include "Protocol/BlockStateHasher.h"
#include "Protocol/Packets/StartGamePacket.h"

#include <cmath>
#include "Protocol/Packets/UpdateAttributesPacket.h"

ServerNetworkHandler::ServerNetworkHandler(const std::string &serverName, const std::string &subName, int maxPlayers)
        : mRakNetInstance(nullptr), mCodecContext(mBlockDefinitions, mItemDefinitions), mMaxPlayers(maxPlayers),
          mIsListening(false), mNextRuntimeId(1), mLevel("Bedrock level", DEFAULT_VIEW_DISTANCE),
          mPlayerData("players"), mOps("ops.txt") {
    std::unique_ptr<RakNetInstance> instance(new RakNetInstance(*this, true));
    mRakNetInstance = instance.get();

    mNetworkHandler.reset(new NetworkHandler(std::move(instance)));
    mNetworkHandler->addListener(this);

    mAnnouncement.mServerName = serverName;
    mAnnouncement.mSubName = subName;
    mAnnouncement.mGameMode = "Survival";
    mAnnouncement.mGameModeId = 1;
    mAnnouncement.mMaxPlayers = maxPlayers;

    mCommands.registerCommand(std::make_shared<GameModeCommand>(*this));
    mCommands.registerCommand(std::make_shared<OpCommand>(*this));
    mCommands.registerCommand(std::make_shared<DeopCommand>(*this));
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
    mLevel = Level(properties.getLevelName(), DEFAULT_VIEW_DISTANCE);

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

    {
        std::lock_guard<std::mutex> lock(mConsoleQueueMutex);
        while (!mConsoleQueue.empty()) {
            ConsoleCommandSender sender;
            mCommands.dispatch(sender, mConsoleQueue.front());
            mConsoleQueue.pop();
        }
    }

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

    ServerPlayer *player = _getPlayer(id);
    if (player != nullptr && !player->getName().empty()) {
        _savePlayerData(*player);

        if (player->isSpawned()) {
            broadcastSystemMessage("§e" + player->getName() + " left the game");
            _removeFromPlayerList(*player);
        }
    }

    mPlayers.erase(id);
    LOG_INFO(LogAreaID::Network, "Player disconnected: %s, reason: %s", id.toString().c_str(), toString(reason));
}

void ServerNetworkHandler::_savePlayerData(const ServerPlayer &player) {
    if (mPlayerData.saveData(player.getName(), player.saveNbt(mLevel.getName())))
        LOG_INFO(LogAreaID::Server, "Saved player data for %s", player.getName().c_str());
    else
        LOG_WARN(LogAreaID::Server, "Could not save player data for %s", player.getName().c_str());
}

void ServerNetworkHandler::_loadPlayerData(ServerPlayer &player) {
    player.setPosition(mLevel.getSpawnPositionForPlayer());
    player.setOp(mOps.isOp(player.getName()));

    Tag data;
    if (!mPlayerData.loadData(player.getName(), data))
        return;

    player.loadNbt(data);
    LOG_INFO(LogAreaID::Server, "Loaded player data for %s", player.getName().c_str());
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
        packet->read(stream, mCodecContext);

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

    mNetworkHandler->send(id, disconnect, mCodecContext);
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

        mNetworkHandler->send(id, status, mCodecContext);
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

    mNetworkHandler->send(id, settings, mCodecContext);

    // the settings themselves travel uncompressed, everything after them does not
    mNetworkHandler->flush(id);
    mNetworkHandler->enableCompression(id, CompressedNetworkPeer::CompressionAlgorithm::ZLib,
                                       settings.mCompressionThreshold);

    mPlayers.erase(id);
    ServerPlayer player(id, mNextRuntimeId++, this);
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

    LoginChainVerifier verifier;
    const bool verified = verifier.verify(packet.mAuthJwt);

    if (mProperties.getXboxAuthRequired() && (!verified || !verifier.isSigned())) {
        LOG_WARN(LogAreaID::Network, "%s failed Xbox Live authentication", id.getAddress().c_str());
        _disconnect(id, "You must be authenticated with Xbox Live to join");
        mPlayers.erase(id);
        return;
    }

    if (verified && verifier.isSigned()) {
        player->setName(verifier.getDisplayName());
        player->setUuid(verifier.getIdentity());
        player->setXuid(verifier.getXuid());
    } else {
        player->setName(request.getDisplayName());
        player->setUuid(request.getIdentity());
        player->setXuid(request.getXuid());
    }

    player->setSkin(request.getSkin());
    player->setBuildPlatform(request.getBuildPlatform());
    player->setLoginState(ServerPlayer::LoginState::LoggedIn);

    LOG_INFO(LogAreaID::Server, "Player %s logged in, uuid %s, xuid %s", player->getName().c_str(),
             player->getUuid().c_str(), player->getXuid().c_str());

    PlayStatusPacket status;
    status.mStatus = PlayStatusPacket::Status::LoginSuccess;
    mNetworkHandler->send(id, status, mCodecContext);

    ResourcePacksInfoPacket packs;
    packs.mForcedToAccept = false;
    packs.mWorldTemplateVersion = "";
    mNetworkHandler->send(id, packs, mCodecContext);

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
            mNetworkHandler->send(id, stack, mCodecContext);
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

    player.getFlags().applyPlayerDefaults();
    player.getAttributes() = EntityAttributes::createPlayerDefaults();
    player.setGameType((int32_t) mProperties.getGameType());
    _loadPlayerData(player);

    StartGamePacket startGame;
    startGame.mUniqueEntityId = player.getUniqueId();
    startGame.mRuntimeEntityId = player.getRuntimeId();
    startGame.mPlayerGameType = mProperties.getGameType();
    startGame.mPlayerPosition = player.getPosition();
    startGame.mRotation = Vector2f(player.getRotation().x, player.getRotation().y);

    startGame.mSeed = 0;
    startGame.mDimensionId = 0;
    startGame.mGeneratorId = 1;
    startGame.mLevelGameType = mProperties.getGameType();
    startGame.mDifficulty = (int32_t) mProperties.getDifficulty();
    startGame.mDefaultSpawn = mLevel.getSpawnPosition();
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
    startGame.mLevelName = mLevel.getName();
    startGame.mMultiplayerCorrelationId = "";
    startGame.mServerEngine = "Falcon";
    startGame.mCurrentTick = 0;
    startGame.mEnchantmentSeed = 0;
    startGame.mBlockNetworkIdsHashed = mProperties.getBlockNetworkIdsAreHashes();
    startGame.mInventoriesServerAuthoritative = true;

    startGame.mGamerules.push_back(GameRuleData::ofBool("showcoordinates", true));

    mNetworkHandler->send(id, startGame, mCodecContext);

    player.setLoginState(ServerPlayer::LoginState::StartGameSent);
    LOG_INFO(LogAreaID::Server, "Sent StartGame to %s", player.getName().c_str());

    _sendAttributes(player);
    _sendEntityData(player);
    _sendAbilities(player);
    _sendBiomeDefinitions(player);
    _sendAvailableCommands(player);
    _sendCreativeContent(player);
}

void ServerNetworkHandler::_sendEntityData(ServerPlayer &player) {
    SetEntityDataPacket entityData;
    entityData.mRuntimeEntityId = (int64_t) player.getRuntimeId();
    entityData.mTick = 0;

    EntityDataEntry flags;
    flags.mId = EntityFlags::FLAGS_DATA_ID;
    flags.mFormat = EntityDataFormat::Long;
    flags.mLongValue = player.getFlags().getLowBits();
    entityData.mMetadata.mEntries.push_back(flags);

    EntityDataEntry flags2;
    flags2.mId = EntityFlags::FLAGS_2_DATA_ID;
    flags2.mFormat = EntityDataFormat::Long;
    flags2.mLongValue = player.getFlags().getHighBits();
    entityData.mMetadata.mEntries.push_back(flags2);

    mNetworkHandler->send(player.getNetworkIdentifier(), entityData, mCodecContext);
}

void ServerNetworkHandler::setPlayerOp(ServerPlayer &player, bool isOp) {
    player.setOp(isOp);
    _sendAbilities(player);

    if (player.isSpawned())
        _sendAvailableCommands(player);

    if (isOp)
        player.sendMessage("§eYou are now an operator");
    else
        player.sendMessage("§eYou are no longer an operator");
}

void ServerNetworkHandler::queueConsoleCommand(const std::string &commandLine) {
    std::lock_guard<std::mutex> lock(mConsoleQueueMutex);
    mConsoleQueue.push(commandLine);
}

void ServerNetworkHandler::_sendAbilities(ServerPlayer &player) {
    const int32_t gameType = player.getGameType();
    const bool isCreative = gameType == (int32_t) GameType::Creative;
    const bool isSpectator = gameType == (int32_t) GameType::Spectator;
    const bool isAdventure = gameType == (int32_t) GameType::Adventure;

    const uint32_t abilitiesSet = (1u << (int) PlayerAbility::Build) | (1u << (int) PlayerAbility::Mine) |
                                  (1u << (int) PlayerAbility::DoorsAndSwitches) |
                                  (1u << (int) PlayerAbility::OpenContainers) |
                                  (1u << (int) PlayerAbility::AttackPlayers) |
                                  (1u << (int) PlayerAbility::AttackMobs) |
                                  (1u << (int) PlayerAbility::OperatorCommands) |
                                  (1u << (int) PlayerAbility::Teleport) |
                                  (1u << (int) PlayerAbility::Invulnerable) |
                                  (1u << (int) PlayerAbility::Flying) | (1u << (int) PlayerAbility::MayFly) |
                                  (1u << (int) PlayerAbility::Instabuild) |
                                  (1u << (int) PlayerAbility::WorldBuilder) | (1u << (int) PlayerAbility::NoClip) |
                                  (1u << (int) PlayerAbility::WalkSpeed) | (1u << (int) PlayerAbility::FlySpeed) |
                                  (1u << (int) PlayerAbility::VerticalFlySpeed);

    uint32_t abilityValues = 1u << (int) PlayerAbility::WalkSpeed;
    abilityValues |= 1u << (int) PlayerAbility::FlySpeed;
    abilityValues |= 1u << (int) PlayerAbility::VerticalFlySpeed;
    abilityValues |= 1u << (int) PlayerAbility::WorldBuilder;

    if (isSpectator) {
        abilityValues |= 1u << (int) PlayerAbility::Invulnerable;
        abilityValues |= 1u << (int) PlayerAbility::Flying;
        abilityValues |= 1u << (int) PlayerAbility::MayFly;
        abilityValues |= 1u << (int) PlayerAbility::NoClip;
    } else {
        abilityValues |= 1u << (int) PlayerAbility::Build;
        abilityValues |= 1u << (int) PlayerAbility::Mine;
        abilityValues |= 1u << (int) PlayerAbility::DoorsAndSwitches;
        abilityValues |= 1u << (int) PlayerAbility::OpenContainers;
        abilityValues |= 1u << (int) PlayerAbility::AttackMobs;

        if (!isAdventure)
            abilityValues |= 1u << (int) PlayerAbility::AttackPlayers;

        if (isCreative) {
            abilityValues |= 1u << (int) PlayerAbility::MayFly;
            abilityValues |= 1u << (int) PlayerAbility::Instabuild;
        }
    }

    if (player.isOp()) {
        abilityValues |= 1u << (int) PlayerAbility::Build;
        abilityValues |= 1u << (int) PlayerAbility::Mine;
        abilityValues |= 1u << (int) PlayerAbility::DoorsAndSwitches;
        abilityValues |= 1u << (int) PlayerAbility::OpenContainers;
        abilityValues |= 1u << (int) PlayerAbility::AttackPlayers;
        abilityValues |= 1u << (int) PlayerAbility::AttackMobs;
        abilityValues |= 1u << (int) PlayerAbility::OperatorCommands;
        abilityValues |= 1u << (int) PlayerAbility::Teleport;
    }

    if (player.isFlying())
        abilityValues |= 1u << (int) PlayerAbility::Flying;

    AbilityLayer layer;
    layer.mLayerType = 1;
    layer.mAbilitiesSet = abilitiesSet;
    layer.mAbilityValues = abilityValues;
    layer.mWalkSpeed = 0.1f;
    layer.mFlySpeed = 0.05f;
    layer.mVerticalFlySpeed = 1.0f;

    UpdateAbilitiesPacket abilities;
    abilities.mAbilities.mUniqueEntityId = player.getUniqueId();
    abilities.mAbilities.mPlayerPermission = (uint8_t) (player.isOp() ? PlayerPermission::Operator
                                                                     : mProperties.getDefaultPlayerPermissionLevel());
    abilities.mAbilities.mCommandPermission = (uint8_t) (player.isOp() ? CommandPermission::GameDirectors
                                                                       : CommandPermission::Any);
    abilities.mAbilities.mAbilityLayers.push_back(layer);

    mNetworkHandler->send(player.getNetworkIdentifier(), abilities, mCodecContext);
}

void ServerNetworkHandler::_sendBiomeDefinitions(ServerPlayer &player) {
    BiomeDefinitionListPacket biomes;
    biomes.mBiomes = mBiomes.getBiomes();

    mNetworkHandler->send(player.getNetworkIdentifier(), biomes, mCodecContext);
}

void ServerNetworkHandler::_sendCreativeContent(ServerPlayer &player) {
    CreativeContentPacket creative;

    mNetworkHandler->send(player.getNetworkIdentifier(), creative, mCodecContext);
}

void ServerNetworkHandler::_sendAvailableCommands(ServerPlayer &player) {
    AvailableCommandsPacket availableCommands;

    for (Command *command: mCommands.getCommands()) {
        if ((int) player.getCommandPermission() < (int) command->getRequiredPermission())
            continue;

        CommandData data;
        data.mName = command->getName();
        data.mDescription = command->getDescription();
        data.mPermission = command->getRequiredPermission();

        CommandParamData argsParameter;
        argsParameter.mName = "args";
        argsParameter.mOptional = true;
        argsParameter.mHasType = true;
        argsParameter.mType = CommandParamType::RawText;

        CommandOverloadData overload;
        overload.mParameters.push_back(argsParameter);

        data.mOverloads.push_back(overload);

        availableCommands.mCommands.push_back(data);
    }

    mNetworkHandler->send(player.getNetworkIdentifier(), availableCommands, mCodecContext);
}

void ServerNetworkHandler::_sendAttributes(ServerPlayer &player) {
    UpdateAttributesPacket attributes;
    attributes.mRuntimeEntityId = (int64_t) player.getRuntimeId();
    attributes.mTick = 0;
    attributes.mAttributes = player.getAttributes().getAll();

    mNetworkHandler->send(player.getNetworkIdentifier(), attributes, mCodecContext);
}

void ServerNetworkHandler::_sendChunks(ServerPlayer &player) {
    const NetworkIdentifier &id = player.getNetworkIdentifier();
    const std::vector<Level::ChunkPosition> chunks = mLevel.getChunksAround(0, 0);

    for (const Level::ChunkPosition &position: chunks) {
        LevelChunkPacket chunk;
        chunk.mChunkX = position.mX;
        chunk.mChunkZ = position.mZ;
        chunk.mDimension = mLevel.getDimensionId();
        chunk.mSubChunksLength = mLevel.getSubChunkCount();
        chunk.mCachingEnabled = false;
        chunk.mRequestSubChunks = false;
        chunk.mData = mLevel.getChunkData();

        mNetworkHandler->send(id, chunk, mCodecContext);
    }

    NetworkChunkPublisherUpdatePacket publisher;
    publisher.mPosition = mLevel.getSpawnPosition();
    publisher.mRadius = (uint32_t) (mLevel.getViewDistance() * 16);
    mNetworkHandler->send(id, publisher, mCodecContext);

    LOG_INFO(LogAreaID::Server, "Sent %u chunks to %s", (unsigned) chunks.size(), player.getName().c_str());
}

void ServerNetworkHandler::handle(const NetworkIdentifier &id, const SetLocalPlayerAsInitializedPacket &packet) {
    (void) packet;

    ServerPlayer *player = _getPlayer(id);
    if (player == nullptr)
        return;

    player->setLoginState(ServerPlayer::LoginState::Spawned);
    LOG_INFO(LogAreaID::Server, "Player %s spawned", player->getName().c_str());

    broadcastSystemMessage("§e" + player->getName() + " joined the game");
}

static Uuid listUuidFor(const ServerPlayer &player) {
    Uuid parsed = Uuid::fromString(player.getUuid());
    if (parsed.mostSignificantBits != 0 || parsed.leastSignificantBits != 0)
        return parsed;

    const std::hash<std::string> hasher;
    uint64_t most = hasher(player.getName() + "-most");
    const uint64_t least = hasher(player.getName() + "-least");
    if (most == 0)
        most = 1;

    return Uuid(most, least);
}

void ServerNetworkHandler::_addToPlayerList(ServerPlayer &player) {
    PlayerListPacket::Entry newEntry(listUuidFor(player));
    newEntry.mEntityId = player.getUniqueId();
    newEntry.mName = player.getName();
    newEntry.mXuid = player.getXuid();
    newEntry.mSkin = player.getSkin();
    newEntry.mBuildPlatform = player.getBuildPlatform();

    PlayerListPacket announce;
    announce.mEntries.push_back(newEntry);

    PlayerListPacket existing;

    for (auto &entry: mPlayers) {
        if (!entry.second.isSpawned())
            continue;

        mNetworkHandler->send(entry.second.getNetworkIdentifier(), announce, mCodecContext);

        if (&entry.second != &player) {
            PlayerListPacket::Entry existingEntry(listUuidFor(entry.second));
            existingEntry.mEntityId = entry.second.getUniqueId();
            existingEntry.mName = entry.second.getName();
            existingEntry.mXuid = entry.second.getXuid();
            existingEntry.mSkin = entry.second.getSkin();
            existingEntry.mBuildPlatform = entry.second.getBuildPlatform();
            existing.mEntries.push_back(existingEntry);
        }
    }

    if (!existing.mEntries.empty())
        mNetworkHandler->send(player.getNetworkIdentifier(), existing, mCodecContext);
}

void ServerNetworkHandler::_removeFromPlayerList(ServerPlayer &player) {
    PlayerListPacket::Entry removalEntry(listUuidFor(player));
    removalEntry.mAction = PlayerListPacket::Action::Remove;

    PlayerListPacket removal;
    removal.mEntries.push_back(removalEntry);

    for (auto &entry: mPlayers) {
        if (entry.second.isSpawned() && &entry.second != &player)
            mNetworkHandler->send(entry.second.getNetworkIdentifier(), removal, mCodecContext);
    }
}

void ServerNetworkHandler::broadcastSystemMessage(const std::string &message) {
    for (auto &entry: mPlayers) {
        if (entry.second.isSpawned())
            entry.second.sendMessage(message);
    }
}

void ServerNetworkHandler::sendPacketTo(const NetworkIdentifier &id, const Packet &packet) {
    mNetworkHandler->send(id, packet, mCodecContext);
}

void ServerNetworkHandler::handle(const NetworkIdentifier &id, const PlayerAuthInputPacket &packet) {
    ServerPlayer *player = _getPlayer(id);
    if (player == nullptr)
        return;

    if (!std::isfinite(packet.mPosition.x) || !std::isfinite(packet.mPosition.y) ||
        !std::isfinite(packet.mPosition.z) || !std::isfinite(packet.mRotation.x) ||
        !std::isfinite(packet.mRotation.y) || !std::isfinite(packet.mRotation.z))
        return;

    player->setPosition(packet.mPosition);
    player->setRotation(packet.mRotation);

    EntityFlags &flags = player->getFlags();
    const EntityFlags previous = flags;

    if (packet.hasInputFlag((int32_t) PlayerAuthInputData::StartSprinting))
        flags.set(EntityFlag::Sprinting, true);
    if (packet.hasInputFlag((int32_t) PlayerAuthInputData::StopSprinting))
        flags.set(EntityFlag::Sprinting, false);

    if (packet.hasInputFlag((int32_t) PlayerAuthInputData::StartSneaking))
        flags.set(EntityFlag::Sneaking, true);
    if (packet.hasInputFlag((int32_t) PlayerAuthInputData::StopSneaking))
        flags.set(EntityFlag::Sneaking, false);

    if (packet.hasInputFlag((int32_t) PlayerAuthInputData::StartSwimming))
        flags.set(EntityFlag::Swimming, true);
    if (packet.hasInputFlag((int32_t) PlayerAuthInputData::StopSwimming))
        flags.set(EntityFlag::Swimming, false);

    if (packet.hasInputFlag((int32_t) PlayerAuthInputData::StartGliding))
        flags.set(EntityFlag::Gliding, true);
    if (packet.hasInputFlag((int32_t) PlayerAuthInputData::StopGliding))
        flags.set(EntityFlag::Gliding, false);

    if (packet.hasInputFlag((int32_t) PlayerAuthInputData::StartCrawling))
        flags.set(EntityFlag::Crawling, true);
    if (packet.hasInputFlag((int32_t) PlayerAuthInputData::StopCrawling))
        flags.set(EntityFlag::Crawling, false);

    if (flags.getLowBits() != previous.getLowBits() || flags.getHighBits() != previous.getHighBits())
        _sendEntityData(*player);

    for (const PlayerBlockActionData &action: packet.mPlayerActions) {
        if (action.mAction == PlayerActionType::BlockPredictDestroy ||
            action.mAction == PlayerActionType::DimensionChangeRequestOrCreativeDestroyBlock)
            _breakBlock(*player, action.mBlockPosition);
    }

    if (packet.mHasItemUseTransaction && packet.mItemUseTransaction.mActionType == 0)
        _placeBlock(*player, packet.mItemUseTransaction);
}

void ServerNetworkHandler::_breakBlock(ServerPlayer &player, const Vector3i &position) {
    const int32_t airHash = mLevel.getAirHash();
    mLevel.setBlock(position.x, position.y, position.z, airHash);

    UpdateBlockPacket update;
    update.mBlockPosition = position;
    update.mRuntimeId = (uint32_t) airHash;
    update.mFlags = UpdateBlockPacket::Flag::All;
    update.mDataLayer = 0;
    mNetworkHandler->send(player.getNetworkIdentifier(), update, mCodecContext);

    LOG_INFO(LogAreaID::Server, "%s broke block at %d %d %d", player.getName().c_str(), position.x, position.y,
             position.z);
}

void ServerNetworkHandler::_placeBlock(ServerPlayer &player, const ItemUseTransaction &transaction) {
    if (transaction.mItemInHand.mBlockDefinition == nullptr)
        return;

    static const int offsets[6][3] = {
            {0,  -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}, {-1, 0, 0}, {1, 0, 0}
    };

    const int face = transaction.mBlockFace >= 0 && transaction.mBlockFace < 6 ? transaction.mBlockFace : 1;
    const Vector3i target(transaction.mBlockPosition.x + offsets[face][0],
                          transaction.mBlockPosition.y + offsets[face][1],
                          transaction.mBlockPosition.z + offsets[face][2]);

    const BlockDefinition &definition = *transaction.mItemInHand.mBlockDefinition;
    const int32_t blockHash = BlockStateHasher::hash(definition.getIdentifier(), definition.getState());

    mLevel.setBlock(target.x, target.y, target.z, blockHash);

    UpdateBlockPacket update;
    update.mBlockPosition = target;
    update.mRuntimeId = (uint32_t) blockHash;
    update.mFlags = UpdateBlockPacket::Flag::All;
    update.mDataLayer = 0;
    mNetworkHandler->send(player.getNetworkIdentifier(), update, mCodecContext);

    LOG_INFO(LogAreaID::Server, "%s placed %s at %d %d %d", player.getName().c_str(),
             definition.getIdentifier().c_str(), target.x, target.y, target.z);
}

ServerPlayer *ServerNetworkHandler::getPlayerByName(const std::string &name) {
    for (auto &entry: mPlayers) {
        if (entry.second.getName() == name)
            return &entry.second;
    }
    return nullptr;
}

void ServerNetworkHandler::setPlayerGameMode(ServerPlayer &player, int gameMode) {
    player.setGameType(gameMode);

    SetPlayerGameTypePacket packet;
    packet.mGamemode = gameMode;
    mNetworkHandler->send(player.getNetworkIdentifier(), packet, mCodecContext);

    _sendAbilities(player);
}

void ServerNetworkHandler::sendCommandOutput(ServerPlayer &player, const CommandOriginData &origin,
                                             const std::string &message) {
    CommandOutputPacket output;
    output.mOrigin = origin;
    output.mType = CommandOutputType::AllOutput;
    output.mSuccessCount = 1;

    CommandOutputMessage line;
    line.mInternal = false;
    line.mMessageId = message;
    output.mMessages.push_back(line);

    mNetworkHandler->send(player.getNetworkIdentifier(), output, mCodecContext);
}

void ServerNetworkHandler::handle(const NetworkIdentifier &id, const TextPacket &packet) {
    ServerPlayer *player = _getPlayer(id);
    if (player == nullptr || !player->isSpawned())
        return;

    if (packet.mType != TextPacket::Type::Chat)
        return;

    LOG_INFO(LogAreaID::Server, "<%s> %s", player->getName().c_str(), packet.mMessage.c_str());

    TextPacket chat;
    chat.mType = TextPacket::Type::Chat;
    chat.mSourceName = player->getName();
    chat.mMessage = packet.mMessage;

    for (auto &entry: mPlayers) {
        if (entry.second.isSpawned())
            mNetworkHandler->send(entry.second.getNetworkIdentifier(), chat, mCodecContext);
    }
}

void ServerNetworkHandler::handle(const NetworkIdentifier &id, const RequestChunkRadiusPacket &packet) {
    ServerPlayer *player = _getPlayer(id);
    if (player == nullptr)
        return;

    int32_t granted = packet.mRadius;
    if (granted > DEFAULT_VIEW_DISTANCE)
        granted = DEFAULT_VIEW_DISTANCE;
    if (granted < 1)
        granted = 1;

    ChunkRadiusUpdatedPacket radius;
    radius.mRadius = granted;
    mNetworkHandler->send(id, radius, mCodecContext);

    LOG_INFO(LogAreaID::Server, "%s requested chunk radius %d, granted %d", player->getName().c_str(),
             packet.mRadius, granted);

    _sendChunks(*player);

    PlayStatusPacket spawn;
    spawn.mStatus = PlayStatusPacket::Status::PlayerSpawn;
    mNetworkHandler->send(id, spawn, mCodecContext);
}

void ServerNetworkHandler::handle(const NetworkIdentifier &id, const RequestAbilityPacket &packet) {
    ServerPlayer *player = _getPlayer(id);
    if (player == nullptr)
        return;

    if (packet.mAbility != Ability::Flying || packet.mType != AbilityValueType::Boolean)
        return;

    const int32_t gameType = player->getGameType();
    const bool mayFly = gameType == (int32_t) GameType::Creative || gameType == (int32_t) GameType::Spectator;

    if (!packet.mBoolValue && !mayFly) {
        _disconnect(id, "Flying is not enabled on this server");
        mPlayers.erase(id);
        return;
    }

    player->setFlying(packet.mBoolValue);
    _sendAbilities(*player);
}

void ServerNetworkHandler::handle(const NetworkIdentifier &id, const PacketViolationWarningPacket &packet) {
    (void) id;

    const char *causeName = toString((MinecraftPacketIds) packet.mPacketCauseId);
    LOG_ERROR(LogAreaID::Network, "CLIENT REJECTED PACKET id=%d (%s), type=%d, severity=%d, context: %s",
              packet.mPacketCauseId, causeName, (int) packet.mType, (int) packet.mSeverity, packet.mContext.c_str());
}

void ServerNetworkHandler::handle(const NetworkIdentifier &id, const CommandRequestPacket &packet) {
    ServerPlayer *player = _getPlayer(id);
    if (player == nullptr)
        return;

    LOG_INFO(LogAreaID::Server, "%s issued command: %s", player->getName().c_str(), packet.mCommand.c_str());

    PlayerCommandSender sender(*this, *player, packet.mOrigin);
    mCommands.dispatch(sender, packet.mCommand);
}

void ServerNetworkHandler::onReceiveIPSupport(RakPeerHelper::IPSupport support) {
    const ConnectionDefinition &definition = mRakNetInstance->getConnectionDefinition();

    if (support == RakPeerHelper::IPSupport::IPv4 || support == RakPeerHelper::IPSupport::Both)
        LOG_INFO(LogAreaID::Network, "IPv4 supported, port: %u", definition.mPort);

    if (support == RakPeerHelper::IPSupport::IPv6 || support == RakPeerHelper::IPSupport::Both)
        LOG_INFO(LogAreaID::Network, "IPv6 supported, port: %u", definition.mPortV6);
}

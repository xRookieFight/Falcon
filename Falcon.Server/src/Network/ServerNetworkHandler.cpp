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
#include "Protocol/Packets/ResourcePackDataInfoPacket.h"
#include "Protocol/Packets/ResourcePackChunkDataPacket.h"
#include "Protocol/Packets/ResourcePackChunkRequestPacket.h"
#include "Protocol/Packets/SetLocalPlayerAsInitializedPacket.h"
#include "Protocol/Packets/PlayerAuthInputPacket.h"
#include "Protocol/Packets/AvailableCommandsPacket.h"
#include "Protocol/Packets/BiomeDefinitionListPacket.h"
#include "Protocol/Packets/CreativeContentPacket.h"
#include "Protocol/Packets/ItemComponentPacket.h"
#include "Protocol/Types/CreativeItemCategory.h"
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
#include "Protocol/Packets/SetMovementAuthorityPacket.h"
#include "Protocol/Packets/AvailableEntityIdentifiersPacket.h"
#include "Protocol/Packets/ContainerClosePacket.h"
#include "Protocol/Packets/CraftingDataPacket.h"
#include "Protocol/Packets/InteractPacket.h"
#include "Protocol/Packets/InventoryContentPacket.h"
#include "Protocol/Packets/InventorySlotPacket.h"
#include "Protocol/Packets/InventoryTransactionPacket.h"
#include "Protocol/Packets/ItemStackRequestPacket.h"
#include "Protocol/Packets/ItemStackResponsePacket.h"
#include "Protocol/Packets/MobArmorEquipmentPacket.h"
#include "Protocol/Packets/MobEquipmentPacket.h"
#include "Protocol/Packets/PlayerHotbarPacket.h"
#include "Inventory/ItemStackRequestHandler.h"
#include "Block/VanillaBlocks.h"
#include "Item/VanillaItems.h"

#include <cmath>
#include <unordered_set>
#include <utility>
#include "Protocol/Packets/UpdateAttributesPacket.h"

static const int64_t RESOURCE_PACK_CHUNK_SIZE = 1024 * 1024;

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

    mResourcePacks.loadFromDirectory("resource_packs");
    _registerVanillaDefinitions();
}

int ServerNetworkHandler::_getServerViewDistance() const {
    int distance = mProperties.getViewDistance();

    if (distance < 1)
        distance = 1;
    if (distance > MAX_VIEW_DISTANCE)
        distance = MAX_VIEW_DISTANCE;

    return distance;
}

void ServerNetworkHandler::_registerVanillaDefinitions() {
    for (const Block &block: VanillaBlocks::getAll()) {
        mBlockDefinitions.registerDefinition(std::make_shared<BlockDefinition>(
                block.getIdentifier(), block.getNetworkHash(), block.getStates()));
    }

    int runtimeId = 1;
    for (const Item &item: VanillaItems::getAll()) {
        if (item.getIdentifier() == "minecraft:air")
            continue;

        mItemDefinitions.registerDefinition(std::make_shared<ItemDefinition>(
                item.getIdentifier(), runtimeId++, false, Tag::ofCompound()));
    }

    for (const Block &block: VanillaBlocks::getAll()) {
        if (block.getIdentifier() == "minecraft:air")
            continue;

        mItemDefinitions.registerDefinition(std::make_shared<ItemDefinition>(
                block.getIdentifier(), runtimeId++, false, Tag::ofCompound()));
    }

    LOG_INFO(LogAreaID::Server, "Registered %zu block(s) and %d item definition(s)",
             VanillaBlocks::getAll().size(), runtimeId - 1);
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
    mLevel = Level(properties.getLevelName(), _getServerViewDistance());
    mLevel.openStorage("worlds");

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
    mLevel.closeStorage();
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

    for (auto &entry: mPlayers) {
        ServerPlayer &player = entry.second;

        if (player.getLoginState() < ServerPlayer::LoginState::StartGameSent)
            continue;

        if (!player.hasChunkPosition())
            continue;

        _sendChunks(player);
        _checkTerrainReady(player);
    }

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

    player.loadNbt(data, mCodecContext);
    LOG_INFO(LogAreaID::Server, "Loaded player data for %s at %.2f %.2f %.2f, rotation %.2f %.2f, mode %d",
             player.getName().c_str(), player.getPosition().x, player.getPosition().y, player.getPosition().z,
             player.getRotation().x, player.getRotation().y, player.getGameType());
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
    packs.mForcedToAccept = mProperties.getTexturePackRequired();
    packs.mWorldTemplateVersion = "";
    packs.mHasAddonPacks = false;
    packs.mScriptingEnabled = false;
    packs.mVibrantVisualsForceDisabled = false;

    for (const ResourcePack &pack: mResourcePacks.getPacks()) {
        ResourcePacksInfoPacket::Entry entry;
        entry.mPackId = pack.mUuid;
        entry.mPackVersion = pack.mVersion;
        entry.mPackSize = pack.mSize;
        entry.mContentKey = pack.mContentKey;
        entry.mContentId = pack.mUuidString;
        packs.mResourcePackInfos.push_back(entry);
    }

    mNetworkHandler->send(id, packs, mCodecContext);

    player->setLoginState(ServerPlayer::LoginState::ResourcePacksSent);
}

void ServerNetworkHandler::handle(const NetworkIdentifier &id, const ResourcePackClientResponsePacket &packet) {
    ServerPlayer *player = _getPlayer(id);
    if (player == nullptr)
        return;

    switch (packet.mStatus) {
        case ResourcePackClientResponsePacket::Status::SendPacks: {
            for (const std::string &requested: packet.mPackIds) {
                const std::string uuid = requested.substr(0, requested.find('_'));
                const ResourcePack *pack = mResourcePacks.findById(uuid);
                if (pack == nullptr)
                    continue;

                ResourcePackDataInfoPacket info;
                info.mPackId = pack->mUuid;
                info.mPackVersion = pack->mVersion;
                info.mMaxChunkSize = RESOURCE_PACK_CHUNK_SIZE;
                info.mChunkCount = (int64_t) ((pack->mSize + RESOURCE_PACK_CHUNK_SIZE - 1) / RESOURCE_PACK_CHUNK_SIZE);
                info.mCompressedPackSize = (int64_t) pack->mSize;
                info.mHash = pack->mSha256;
                info.mType = ResourcePackType::Resources;
                mNetworkHandler->send(id, info, mCodecContext);
            }
            break;
        }

        case ResourcePackClientResponsePacket::Status::HaveAllPacks: {
            ResourcePackStackPacket stack;
            stack.mForcedToAccept = mProperties.getTexturePackRequired();
            stack.mGameVersion = mAnnouncement.mGameVersion;

            for (const ResourcePack &pack: mResourcePacks.getPacks()) {
                ResourcePackStackPacket::Entry entry;
                entry.mPackId = pack.mUuidString;
                entry.mPackVersion = pack.mVersion;
                stack.mResourcePacks.push_back(entry);
            }

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

void ServerNetworkHandler::handle(const NetworkIdentifier &id, const ResourcePackChunkRequestPacket &packet) {
    const ResourcePack *pack = mResourcePacks.findById(packet.mPackId.toString());
    if (pack == nullptr) {
        LOG_WARN(LogAreaID::Network, "Client requested unknown resource pack %s", packet.mPackId.toString().c_str());
        return;
    }

    const uint64_t offset = (uint64_t) packet.mChunkIndex * (uint64_t) RESOURCE_PACK_CHUNK_SIZE;
    if (offset >= pack->mSize)
        return;

    const uint64_t remaining = pack->mSize - offset;
    const uint64_t length = remaining < (uint64_t) RESOURCE_PACK_CHUNK_SIZE ? remaining : (uint64_t) RESOURCE_PACK_CHUNK_SIZE;

    ResourcePackChunkDataPacket chunk;
    chunk.mPackId = pack->mUuid;
    chunk.mPackVersion = pack->mVersion;
    chunk.mChunkIndex = packet.mChunkIndex;
    chunk.mProgress = (int64_t) offset;
    chunk.mData = pack->mData.substr(offset, length);
    mNetworkHandler->send(id, chunk, mCodecContext);
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
    startGame.mPlayerGameType = (GameType) player.getGameType();
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

    SetMovementAuthorityPacket movementAuthority;
    movementAuthority.mMode = AuthoritativeMovementMode::Server;
    mNetworkHandler->send(id, movementAuthority, mCodecContext);

    player.setLoginState(ServerPlayer::LoginState::StartGameSent);
    LOG_INFO(LogAreaID::Server, "Sent StartGame to %s", player.getName().c_str());

    player.getInventoryManager().attach(&player, this);

    _sendItemComponents(player);
    _sendActorIdentifiers(player);
    _sendBiomeDefinitions(player);
    _sendAttributes(player);
    _sendAvailableCommands(player);
    _sendAbilities(player);
    _sendEntityData(player);

    player.getInventoryManager().syncAll();
    player.getInventoryManager().syncSelectedHotbarSlot();

    _sendCreativeContent(player);
    _sendCraftingData(player);
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

void ServerNetworkHandler::_sendItemComponents(ServerPlayer &player) {
    ItemComponentPacket components;

    for (const std::shared_ptr<ItemDefinition> &definition: mItemDefinitions.getAll()) {
        ItemComponentEntry entry;
        entry.mIdentifier = definition->getIdentifier();
        entry.mRuntimeId = (int16_t) definition->getRuntimeId();
        entry.mComponentBased = definition->isComponentBased();
        entry.mItemVersion = 0;
        entry.mComponentData = definition->getComponentData();
        components.mEntries.push_back(entry);
    }

    mNetworkHandler->send(player.getNetworkIdentifier(), components, mCodecContext);

    LOG_INFO(LogAreaID::Server, "Sent %zu item component(s) to %s", components.mEntries.size(),
             player.getName().c_str());
}

void ServerNetworkHandler::_sendActorIdentifiers(ServerPlayer &player) {
    AvailableEntityIdentifiersPacket identifiers;
    identifiers.mIdentifiers = Tag::ofCompound();
    identifiers.mIdentifiers.put("idlist", Tag::ofList(Tag::Type::Compound));

    mNetworkHandler->send(player.getNetworkIdentifier(), identifiers, mCodecContext);
}

void ServerNetworkHandler::_sendCraftingData(ServerPlayer &player) {
    CraftingDataPacket crafting;
    crafting.mCleanRecipes = true;

    mNetworkHandler->send(player.getNetworkIdentifier(), crafting, mCodecContext);
}

void ServerNetworkHandler::_buildCreativeContent() {
    if (!mCreativeItems.empty())
        return;

    CreativeItemGroup constructionGroup;
    constructionGroup.mCategory = (uint8_t) CreativeItemCategory::Construction;
    constructionGroup.mName = "itemGroup.name.construction";
    constructionGroup.mIcon = ItemStack::air();
    mCreativeGroups.push_back(constructionGroup);

    CreativeItemGroup itemsGroup;
    itemsGroup.mCategory = (uint8_t) CreativeItemCategory::Items;
    itemsGroup.mName = "itemGroup.name.items";
    itemsGroup.mIcon = ItemStack::air();
    mCreativeGroups.push_back(itemsGroup);

    int32_t netId = 1;

    for (const Block &block: VanillaBlocks::getAll()) {
        if (block.getIdentifier() == "minecraft:air")
            continue;

        std::shared_ptr<ItemDefinition> definition = mItemDefinitions.getDefinition(block.getIdentifier());
        if (definition == nullptr)
            continue;

        CreativeItemData entry;
        entry.mNetId = netId++;
        entry.mGroupIndex = 0;
        entry.mItem.mDefinition = definition;
        entry.mItem.mBlockDefinition = mBlockDefinitions.getDefinition(block.getIdentifier());
        entry.mItem.mCount = 1;
        mCreativeItems.push_back(entry);
    }

    for (const Item &item: VanillaItems::getAll()) {
        if (item.getIdentifier() == "minecraft:air")
            continue;

        std::shared_ptr<ItemDefinition> definition = mItemDefinitions.getDefinition(item.getIdentifier());
        if (definition == nullptr)
            continue;

        CreativeItemData entry;
        entry.mNetId = netId++;
        entry.mGroupIndex = 1;
        entry.mItem.mDefinition = definition;
        entry.mItem.mCount = 1;
        mCreativeItems.push_back(entry);
    }
}

void ServerNetworkHandler::_sendCreativeContent(ServerPlayer &player) {
    _buildCreativeContent();

    CreativeContentPacket creative;
    creative.mGroups = mCreativeGroups;
    creative.mItems = mCreativeItems;

    mNetworkHandler->send(player.getNetworkIdentifier(), creative, mCodecContext);

    LOG_INFO(LogAreaID::Server, "Sent %zu creative item(s) to %s", creative.mItems.size(),
             player.getName().c_str());
}

void ServerNetworkHandler::_sendInventory(ServerPlayer &player) {
    player.getInventoryManager().syncAll();
    player.getInventoryManager().syncSelectedHotbarSlot();
    _sendArmorContent(player);
    _sendOffhandContent(player);
    _sendHeldItem(player);
}

void ServerNetworkHandler::_sendArmorContent(ServerPlayer &player) {
    const PlayerInventory &inventory = player.getInventory();

    MobArmorEquipmentPacket equipment;
    equipment.mRuntimeEntityId = (int64_t) player.getRuntimeId();
    equipment.mHelmet = inventory.getArmor(PlayerInventory::ARMOR_HEAD);
    equipment.mChestplate = inventory.getArmor(PlayerInventory::ARMOR_TORSO);
    equipment.mLeggings = inventory.getArmor(PlayerInventory::ARMOR_LEGS);
    equipment.mBoots = inventory.getArmor(PlayerInventory::ARMOR_FEET);
    equipment.mBody = ItemStack::air();

    for (auto &entry: mPlayers) {
        if (entry.second.isSpawned() && &entry.second != &player)
            mNetworkHandler->send(entry.second.getNetworkIdentifier(), equipment, mCodecContext);
    }
}

void ServerNetworkHandler::_sendOffhandContent(ServerPlayer &player) {
    const ItemStack &offhand = player.getInventory().getOffhand();

    MobEquipmentPacket equipment;
    equipment.mRuntimeEntityId = (int64_t) player.getRuntimeId();
    equipment.mItem = offhand;
    equipment.mInventorySlot = PlayerInventory::OFFHAND_NETWORK_SLOT;
    equipment.mHotbarSlot = PlayerInventory::OFFHAND_NETWORK_SLOT;
    equipment.mContainerId = PlayerInventory::CONTAINER_ID_OFFHAND;

    for (auto &entry: mPlayers) {
        if (entry.second.isSpawned())
            mNetworkHandler->send(entry.second.getNetworkIdentifier(), equipment, mCodecContext);
    }
}

void ServerNetworkHandler::_sendHeldItem(ServerPlayer &player) {
    const PlayerInventory &inventory = player.getInventory();
    const int selected = inventory.getSelectedSlot();

    player.getInventoryManager().syncSelectedHotbarSlot();

    MobEquipmentPacket equipment;
    equipment.mRuntimeEntityId = (int64_t) player.getRuntimeId();
    equipment.mItem = inventory.getItemInHand();
    equipment.mInventorySlot = selected;
    equipment.mHotbarSlot = selected;
    equipment.mContainerId = PlayerInventory::CONTAINER_ID_INVENTORY;

    for (auto &entry: mPlayers) {
        if (entry.second.isSpawned() && &entry.second != &player)
            mNetworkHandler->send(entry.second.getNetworkIdentifier(), equipment, mCodecContext);
    }
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
    const Vector3f position = player.getPosition();
    const int32_t centerChunkX = (int32_t) std::floor(position.x) >> 4;
    const int32_t centerChunkZ = (int32_t) std::floor(position.z) >> 4;

    const std::vector<Level::ChunkPosition> chunks = mLevel.getChunksAround(centerChunkX, centerChunkZ);
    std::unordered_set<int64_t> &sent = player.getSentChunks();

    NetworkChunkPublisherUpdatePacket publisher;
    publisher.mPosition = Vector3i((int32_t) position.x, (int32_t) position.y, (int32_t) position.z);
    publisher.mRadius = (uint32_t) (mLevel.getViewDistance() * 16);
    mNetworkHandler->send(id, publisher, mCodecContext);

    unsigned added = 0;
    for (const Level::ChunkPosition &chunkPosition: chunks) {
        if (added >= CHUNKS_PER_TICK)
            break;

        const int64_t key = ((int64_t) chunkPosition.mX << 32) | (uint32_t) chunkPosition.mZ;
        if (!sent.insert(key).second)
            continue;

        LevelChunkPacket chunk;
        chunk.mChunkX = chunkPosition.mX;
        chunk.mChunkZ = chunkPosition.mZ;
        chunk.mDimension = mLevel.getDimensionId();
        chunk.mSubChunksLength = (uint32_t) mLevel.getChunkSubChunkCount(chunkPosition.mX, chunkPosition.mZ);
        chunk.mCachingEnabled = false;
        chunk.mRequestSubChunks = false;
        chunk.mData = mLevel.getChunkData(chunkPosition.mX, chunkPosition.mZ);

        mNetworkHandler->send(id, chunk, mCodecContext);
        added++;
    }

    player.setLastChunkPosition(centerChunkX, centerChunkZ);
    player.addSentChunkCount(added);
}

void ServerNetworkHandler::handle(const NetworkIdentifier &id, const SetLocalPlayerAsInitializedPacket &packet) {
    (void) packet;

    ServerPlayer *player = _getPlayer(id);
    if (player == nullptr)
        return;

    player->setLoginState(ServerPlayer::LoginState::Spawned);
    LOG_INFO(LogAreaID::Server, "Player %s spawned", player->getName().c_str());

    _sendInventory(*player);

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

    if (player->isSpawned()) {
        const int32_t chunkX = (int32_t) std::floor(packet.mPosition.x) >> 4;
        const int32_t chunkZ = (int32_t) std::floor(packet.mPosition.z) >> 4;

        if (!player->hasChunkPosition() || chunkX != player->getLastChunkX() ||
            chunkZ != player->getLastChunkZ())
            _sendChunks(*player);
    }

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
    mLevel.setBlockState(position.x, position.y, position.z, BlockState("minecraft:air"));

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

    mLevel.setBlockState(target.x, target.y, target.z,
                         BlockState(definition.getIdentifier(), definition.getState()));

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

    const bool mayFly = gameMode == (int32_t) GameType::Creative || gameMode == (int32_t) GameType::Spectator;
    if (!mayFly && player.isFlying())
        player.setFlying(false);

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
    const int32_t maximum = _getServerViewDistance();

    if (granted > maximum)
        granted = maximum;
    if (granted < 1)
        granted = 1;

    ChunkRadiusUpdatedPacket radius;
    radius.mRadius = granted;
    mNetworkHandler->send(id, radius, mCodecContext);

    LOG_INFO(LogAreaID::Server, "%s requested chunk radius %d, granted %d", player->getName().c_str(),
             packet.mRadius, granted);

    _sendChunks(*player);
    _checkTerrainReady(*player);
}

void ServerNetworkHandler::_checkTerrainReady(ServerPlayer &player) {
    if (player.hasSpawnChunksReady())
        return;

    if (player.getSentChunkCount() < SPAWN_CHUNK_THRESHOLD)
        return;

    player.setSpawnChunksReady(true);

    PlayStatusPacket spawn;
    spawn.mStatus = PlayStatusPacket::Status::PlayerSpawn;
    mNetworkHandler->send(player.getNetworkIdentifier(), spawn, mCodecContext);

    LOG_INFO(LogAreaID::Server, "Terrain ready for %s after %u chunks", player.getName().c_str(),
             (unsigned) player.getSentChunkCount());
}

void ServerNetworkHandler::handle(const NetworkIdentifier &id, const RequestAbilityPacket &packet) {
    ServerPlayer *player = _getPlayer(id);
    if (player == nullptr)
        return;

    if (packet.mAbility != Ability::Flying || packet.mType != AbilityValueType::Boolean)
        return;

    const int32_t gameType = player->getGameType();
    const bool mayFly = gameType == (int32_t) GameType::Creative || gameType == (int32_t) GameType::Spectator;

    if (packet.mBoolValue && !mayFly) {
        _disconnect(id, "Flying is not enabled on this server");
        mPlayers.erase(id);
        return;
    }

    player->setFlying(packet.mBoolValue);
    _sendAbilities(*player);
}

void ServerNetworkHandler::handle(const NetworkIdentifier &id, const MobEquipmentPacket &packet) {
    ServerPlayer *player = _getPlayer(id);
    if (player == nullptr || !player->isSpawned())
        return;

    if (packet.mContainerId != PlayerInventory::CONTAINER_ID_INVENTORY)
        return;

    PlayerInventory &inventory = player->getInventory();
    if (packet.mHotbarSlot < 0 || packet.mHotbarSlot >= PlayerInventory::HOTBAR_SIZE) {
        _sendHeldItem(*player);
        return;
    }

    player->getInventoryManager().onClientSelectHotbarSlot(packet.mHotbarSlot);
    inventory.setSelectedSlot(packet.mHotbarSlot);

    MobEquipmentPacket equipment;
    equipment.mRuntimeEntityId = (int64_t) player->getRuntimeId();
    equipment.mItem = inventory.getItemInHand();
    equipment.mInventorySlot = inventory.getSelectedSlot();
    equipment.mHotbarSlot = inventory.getSelectedSlot();
    equipment.mContainerId = PlayerInventory::CONTAINER_ID_INVENTORY;

    for (auto &entry: mPlayers) {
        if (entry.second.isSpawned() && &entry.second != player)
            mNetworkHandler->send(entry.second.getNetworkIdentifier(), equipment, mCodecContext);
    }
}

void ServerNetworkHandler::handle(const NetworkIdentifier &id, const PlayerHotbarPacket &packet) {
    ServerPlayer *player = _getPlayer(id);
    if (player == nullptr || !player->isSpawned())
        return;

    if (packet.mContainerId != PlayerInventory::CONTAINER_ID_INVENTORY || !packet.mSelectHotbarSlot)
        return;

    if (packet.mSelectedHotbarSlot < 0 || packet.mSelectedHotbarSlot >= PlayerInventory::HOTBAR_SIZE) {
        _sendHeldItem(*player);
        return;
    }

    player->getInventoryManager().onClientSelectHotbarSlot(packet.mSelectedHotbarSlot);
    player->getInventory().setSelectedSlot(packet.mSelectedHotbarSlot);
}

void ServerNetworkHandler::handle(const NetworkIdentifier &id, const InteractPacket &packet) {
    ServerPlayer *player = _getPlayer(id);
    if (player == nullptr || !player->isSpawned())
        return;

    if (packet.mAction != InteractPacket::Action::OpenInventory)
        return;

    if (packet.mRuntimeEntityId != player->getRuntimeId())
        return;

    player->getInventoryManager().onClientOpenMainInventory();
}

void ServerNetworkHandler::handle(const NetworkIdentifier &id, const ItemStackRequestPacket &packet) {
    ServerPlayer *player = _getPlayer(id);
    if (player == nullptr || !player->isSpawned())
        return;

    PlayerInventory &inventory = player->getInventory();

    ItemStackResponsePacket response;
    bool needsResync = false;

    for (const ItemStackRequest &request: packet.mRequests) {
        _buildCreativeContent();
        ItemStackResponseEntry entry = ItemStackRequestHandler::execute(inventory, request, mCreativeItems);
        if (entry.mResult != ItemStackRequestHandler::RESULT_OK)
            needsResync = true;

        response.mEntries.push_back(std::move(entry));
    }

    if (response.mEntries.empty())
        return;

    mNetworkHandler->send(id, response, mCodecContext);

    if (needsResync)
        _sendInventory(*player);
}

void ServerNetworkHandler::handle(const NetworkIdentifier &id, const InventoryTransactionPacket &packet) {
    ServerPlayer *player = _getPlayer(id);
    if (player == nullptr || !player->isSpawned())
        return;

    if (packet.mTransactionType != InventoryTransactionType::Normal
        && packet.mTransactionType != InventoryTransactionType::Mismatch)
        return;

    LOG_TRACE(LogAreaID::Network, "Resyncing inventory of %s after a legacy transaction", player->getName().c_str());
    _sendInventory(*player);
}

void ServerNetworkHandler::handle(const NetworkIdentifier &id, const ContainerClosePacket &packet) {
    ServerPlayer *player = _getPlayer(id);
    if (player == nullptr || !player->isSpawned())
        return;

    PlayerInventory &inventory = player->getInventory();
    if (!inventory.getCursor().isAir()) {
        inventory.addItem(inventory.getCursor());
        inventory.setCursor(ItemStack::air());
    }

    player->getInventoryManager().onClientRemoveWindow((int) packet.mWindowId);
    player->getInventoryManager().syncAll();
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

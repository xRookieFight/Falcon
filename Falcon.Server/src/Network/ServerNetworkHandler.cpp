#include "Network/ServerNetworkHandler.h"

#include "Command/ConsoleCommandSender.h"
#include "Command/DeopCommand.h"
#include "Command/EnchantCommand.h"
#include "Command/GameModeCommand.h"
#include "Command/GiveCommand.h"
#include "Command/KillCommand.h"
#include "Command/OpCommand.h"
#include "Command/PlayerCommandSender.h"
#include "Core/Debug/BedrockLog.h"
#include "Network/ConnectionRequest.h"
#include "Network/AuthKeyProvider.h"
#include "Network/BadPacketCheck.h"
#include "Network/BlockActionHandler.h"
#include "Network/ChatHandler.h"
#include "Network/InventoryHandler.h"
#include "Network/ItemEntityHandler.h"
#include "Network/LoginChainVerifier.h"
#include "Network/LoginHandler.h"
#include "Network/MovementHandler.h"
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
#include "Protocol/Packets/MovePlayerPacket.h"
#include "Protocol/Packets/CorrectPlayerMovePredictionPacket.h"
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
#include "Protocol/Packets/StartGamePacket.h"
#include "Protocol/Packets/AvailableEntityIdentifiersPacket.h"
#include "Protocol/Packets/DeathInfoPacket.h"
#include "Protocol/Packets/EntityEventPacket.h"
#include "Protocol/Packets/PlayerActionPacket.h"
#include "Protocol/Packets/RespawnPacket.h"
#include "Protocol/Packets/SetHealthPacket.h"
#include "Protocol/Packets/ContainerClosePacket.h"
#include "Protocol/Packets/CraftingDataPacket.h"
#include "Protocol/Packets/InteractPacket.h"
#include "Protocol/Packets/InventoryContentPacket.h"
#include "Protocol/Packets/InventorySlotPacket.h"
#include "Protocol/Packets/InventoryTransactionPacket.h"
#include "Protocol/Packets/ItemStackRequestPacket.h"
#include "Protocol/Packets/MobEquipmentPacket.h"
#include "Protocol/Packets/PlayerHotbarPacket.h"
#include "Block/CreativeContentTable.h"
#include "Block/VanillaBlocks.h"
#include "Item/CraftingRecipeTable.h"
#include "Item/ItemNetworkIdTable.h"
#include "Item/ItemData.h"
#include "Core/NBT/NbtIo.h"

#include <cmath>
#include <cstdlib>
#include <unordered_set>
#include <utility>
#include "Protocol/Packets/UpdateAttributesPacket.h"

static const int64_t RESOURCE_PACK_CHUNK_SIZE = 1024 * 1024;
static const float PLAYER_BASE_OFFSET = 1.62f;
static const int64_t FOOD_USE_DURATION_TICKS = 31;
static const float THROW_SPEED = 0.3f;
static const int THROW_PICKUP_DELAY = 40;
static const char *HEALTH_ATTRIBUTE = "minecraft:health";
static const float DEFAULT_MAX_HEALTH = 20.0f;
static const float ITEM_DROP_HEIGHT = 1.3f;

namespace {
    float maxHealthOf(const Entity &entity) {
        for (const AttributeData &attribute: entity.getAttributes().getAll()) {
            if (attribute.mName == HEALTH_ATTRIBUTE)
                return attribute.mMaximum;
        }

        return DEFAULT_MAX_HEALTH;
    }

    float randomUnitFloat() {
        return (float) (rand() % 1000) / 1000.0f;
    }

    Vector3f randomDropMotion() {
        return Vector3f(randomUnitFloat() * 0.2f - 0.1f, 0.2f, randomUnitFloat() * 0.2f - 0.1f);
    }
}

ServerNetworkHandler::ServerNetworkHandler(const std::string &serverName, const std::string &subName, int maxPlayers)
        : mRakNetInstance(nullptr), mCodecContext(mBlockDefinitions, mItemDefinitions), mMaxPlayers(maxPlayers),
          mIsListening(false), mKeepInventory(false), mNextRuntimeId(1),
          mLevel("Bedrock level", DEFAULT_VIEW_DISTANCE),
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
    mCommands.registerCommand(std::make_shared<GiveCommand>(*this));
    mCommands.registerCommand(std::make_shared<EnchantCommand>(*this));
    mCommands.registerCommand(std::make_shared<KillCommand>(*this));

    mResourcePacks.loadFromDirectory("resource_packs");
    _registerVanillaDefinitions();

    AuthKeyProvider::getInstance().start();
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
    LoginHandler::registerVanillaDefinitions(*this);
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

    if (!mPlayers.empty()) {
        LOG_INFO(LogAreaID::Server, "Saving data for %zu player(s)", mPlayers.size());

        for (auto &entry: mPlayers) {
            ServerPlayer &player = entry.second;

            if (!player.getName().empty())
                _savePlayerData(player);

            _disconnect(entry.first, "Server closed");
        }

        mPlayers.clear();
        mNetworkHandler->runEvents();
    }

    LOG_INFO(LogAreaID::Server, "Saving level %s", mLevel.getName().c_str());
    mLevel.closeStorage();

    AuthKeyProvider::getInstance().stop();

    mNetworkHandler->disconnect();
    mIsListening = false;

    LOG_INFO(LogAreaID::Server, "Server stopped.");
}

void ServerNetworkHandler::tick() {
    if (!mIsListening)
        return;

    mCurrentTick++;

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

        if (player.hasPendingMove()) {
            MovementHandler::handleMovement(*this, player, player.getPendingMovePosition(),
                                            player.getPendingMoveRotation());
            player.clearPendingMove();
        }

        if (player.isBreakingBlock()) {
            const Vector3i breakingPosition = player.getBreakingBlockPosition();
            const Vector3f breakingCenter((float) breakingPosition.x + 0.5f, (float) breakingPosition.y + 0.5f,
                                          (float) breakingPosition.z + 0.5f);
            const Vector3f feet = player.getPosition();
            const float dx = feet.x - breakingCenter.x;
            const float dy = feet.y - breakingCenter.y;
            const float dz = feet.z - breakingCenter.z;

            if (dx * dx + dy * dy + dz * dz > 256.0f) {
                BlockActionHandler::stopBreakingBlock(*this, player);
            } else {
                BlockActionHandler::continueBreakingBlock(*this, player);

                if (player.isBreakingBlock() && player.getBreakingFxTicker() % 5 == 0)
                    BlockActionHandler::sendBreakingFx(*this, player);
            }
        }

        if (player.isSpawned() && player.tickHunger(1, (int) mProperties.getDifficulty()))
            _sendAttributes(player);

        if (!player.hasChunkPosition())
            continue;

        _sendChunks(player);
        _checkTerrainReady(player);
    }

    ItemEntityHandler::tickItemEntities(*this);

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
    if (!mPlayerData.loadData(player.getName(), data)) {
        player.resetFallDistance();
        return;
    }

    player.loadNbt(data, mCodecContext);
    player.resetFallDistance();

    if (player.getAttributes().get(HEALTH_ATTRIBUTE) <= 0.0f) {
        player.setDead(false);
        player.getAttributes().set(HEALTH_ATTRIBUTE, maxHealthOf(player));
        player.setPosition(mLevel.getSpawnPositionForPlayer());
        player.resetFallDistance();
    }

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

        if (packetId != MinecraftPacketIds::PlayerAuthInput)
            LOG_INFO(LogAreaID::Network, "RECV %s (id %d)", packet->getName(), (int) packetId);
        packet->handle(id, *this);
    } catch (const BinaryDataException &exception) {
        LOG_WARN(LogAreaID::Network, "Malformed packet from %s: %s", id.getAddress().c_str(), exception.what());
        _disconnect(id, "Malformed packet");
    } catch (const std::exception &exception) {
        LOG_ERROR(LogAreaID::Network, "Unhandled exception while processing a packet from %s: %s",
                  id.getAddress().c_str(), exception.what());
        _disconnect(id, "Internal server error");
    } catch (...) {
        LOG_ERROR(LogAreaID::Network, "Unknown exception while processing a packet from %s", id.getAddress().c_str());
        _disconnect(id, "Internal server error");
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
    LoginHandler::handleRequestNetworkSettings(*this, id, packet);
}

void ServerNetworkHandler::handle(const NetworkIdentifier &id, const LoginPacket &packet) {
    ServerPlayer *player = _getPlayer(id);
    if (player == nullptr) {
        _disconnect(id, "Unexpected login");
        return;
    }

    LoginHandler::handleLogin(*this, id, *player, packet);
}

void ServerNetworkHandler::handle(const NetworkIdentifier &id, const ResourcePackClientResponsePacket &packet) {
    ServerPlayer *player = _getPlayer(id);
    if (player == nullptr)
        return;

    LoginHandler::handleResourcePackClientResponse(*this, id, *player, packet);
}

void ServerNetworkHandler::handle(const NetworkIdentifier &id, const ResourcePackChunkRequestPacket &packet) {
    LoginHandler::handleResourcePackChunkRequest(*this, id, packet);
}

void ServerNetworkHandler::_sendStartGame(ServerPlayer &player) {
    LoginHandler::sendStartGame(*this, player);
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
    LoginHandler::sendAbilities(*this, player);
}

void ServerNetworkHandler::_sendBiomeDefinitions(ServerPlayer &player) {
    LoginHandler::sendBiomeDefinitions(*this, player);
}

void ServerNetworkHandler::_sendItemComponents(ServerPlayer &player) {
    LoginHandler::sendItemComponents(*this, player);
}

void ServerNetworkHandler::_sendActorIdentifiers(ServerPlayer &player) {
    LoginHandler::sendActorIdentifiers(*this, player);
}

void ServerNetworkHandler::_buildCraftingData() {
    LoginHandler::buildCraftingData(*this);
}

void ServerNetworkHandler::_sendCraftingData(ServerPlayer &player) {
    LoginHandler::sendCraftingData(*this, player);
}

void ServerNetworkHandler::_buildCreativeContent() {
    LoginHandler::buildCreativeContent(*this);
}

void ServerNetworkHandler::_sendCreativeContent(ServerPlayer &player) {
    LoginHandler::sendCreativeContent(*this, player);
}

void ServerNetworkHandler::_sendInventory(ServerPlayer &player) {
    InventoryHandler::sendInventory(*this, player);
}

void ServerNetworkHandler::_sendAvailableCommands(ServerPlayer &player) {
    LoginHandler::sendAvailableCommands(*this, player);
}

void ServerNetworkHandler::_sendAttributes(ServerPlayer &player) {
    LoginHandler::sendAttributes(*this, player);
}

void ServerNetworkHandler::_sendHealth(ServerPlayer &player) {
    SetHealthPacket health;
    health.mHealth = (int32_t) std::ceil(player.getAttributes().get(HEALTH_ATTRIBUTE));

    mNetworkHandler->send(player.getNetworkIdentifier(), health, mCodecContext);

    _sendAttributes(player);
}

void ServerNetworkHandler::_broadcastEntityEvent(const Entity &entity, uint8_t eventId) {
    EntityEventPacket event;
    event.mRuntimeEntityId = entity.getRuntimeId();
    event.mEventId = eventId;
    event.mEventData = 0;
    event.mHasFirePosition = false;

    for (auto &entry: mPlayers) {
        if (entry.second.isSpawned())
            mNetworkHandler->send(entry.second.getNetworkIdentifier(), event, mCodecContext);
    }
}

void ServerNetworkHandler::_handleFallDamage(ServerPlayer &player) {
    if (player.isFlying() || player.isDead())
        return;

    const float damage = player.computeFallDamage();
    if (damage < 1.0f)
        return;

    applyDamage(player, damage, "§7" + player.getName() + " fell from a high place");
}

void ServerNetworkHandler::applyDamage(ServerPlayer &player, float amount, const std::string &deathMessage) {
    if (!player.isSpawned() || player.isDead() || amount <= 0.0f)
        return;

    const int32_t gameType = player.getGameType();
    if (gameType == (int32_t) GameType::Creative || gameType == (int32_t) GameType::Spectator)
        return;

    const float health = player.reduceHealth(amount);

    if (health <= 0.0f) {
        killPlayer(player, deathMessage);
        return;
    }

    _sendHealth(player);
    _broadcastEntityEvent(player, (uint8_t) EntityEventType::HurtAnimation);
}

void ServerNetworkHandler::killPlayer(ServerPlayer &player, const std::string &deathMessage) {
    if (player.isDead())
        return;

    player.kill();

    player.getInventoryManager().onCurrentWindowRemove();

    if (!mKeepInventory && player.getGameType() != (int32_t) GameType::Creative)
        _dropInventoryOnDeath(player);

    _sendHealth(player);
    _broadcastEntityEvent(player, (uint8_t) EntityEventType::DeathAnimation);

    const std::string message = deathMessage.empty() ? "§7" + player.getName() + " died" : deathMessage;
    broadcastSystemMessage(message);

    const Vector3f spawn = mLevel.getSpawnPositionForPlayer();

    RespawnPacket respawn;
    respawn.mPosition = Vector3f(spawn.x, spawn.y + PLAYER_BASE_OFFSET, spawn.z);
    respawn.mState = RespawnPacket::State::ServerSearching;
    respawn.mRuntimeEntityId = player.getRuntimeId();
    mNetworkHandler->send(player.getNetworkIdentifier(), respawn, mCodecContext);

    DeathInfoPacket info;
    info.mCauseAttackName = message;
    mNetworkHandler->send(player.getNetworkIdentifier(), info, mCodecContext);

    LOG_INFO(LogAreaID::Server, "%s died", player.getName().c_str());
}

void ServerNetworkHandler::_throwItem(ServerPlayer &player, const ItemStack &item) {
    if (item.isAir() || item.mCount <= 0)
        return;

    const Vector3f position = player.getPosition();
    const Vector3f dropPosition(position.x, position.y + ITEM_DROP_HEIGHT, position.z);

    const float yaw = player.getRotation().y * 3.14159265f / 180.0f;
    const float pitch = player.getRotation().x * 3.14159265f / 180.0f;

    const Vector3f motion(-std::sin(yaw) * std::cos(pitch) * THROW_SPEED,
                          -std::sin(pitch) * THROW_SPEED + 0.1f,
                          std::cos(yaw) * std::cos(pitch) * THROW_SPEED);

    dropItem(dropPosition, item, motion, THROW_PICKUP_DELAY);
}

void ServerNetworkHandler::_dropInventoryOnDeath(ServerPlayer &player) {
    PlayerInventory &inventory = player.getInventory();
    const Vector3f position = player.getPosition();
    const Vector3f dropPosition(position.x, position.y + ITEM_DROP_HEIGHT, position.z);

    for (int slot = 0; slot < PlayerInventory::CONTAINER_SIZE; slot++) {
        dropItem(dropPosition, inventory.getItem(slot), randomDropMotion(), ItemEntity::DEFAULT_PICKUP_DELAY);
    }

    for (int slot = 0; slot < PlayerInventory::ARMOR_SIZE; slot++) {
        dropItem(dropPosition, inventory.getArmor(slot), randomDropMotion(), ItemEntity::DEFAULT_PICKUP_DELAY);
    }

    dropItem(dropPosition, inventory.getOffhand(), randomDropMotion(), ItemEntity::DEFAULT_PICKUP_DELAY);
    dropItem(dropPosition, inventory.getCursor(), randomDropMotion(), ItemEntity::DEFAULT_PICKUP_DELAY);

    inventory.clear();
    inventory.setSelectedSlot(0);

    _sendInventory(player);
}

void ServerNetworkHandler::_respawnPlayer(ServerPlayer &player) {
    if (!player.isDead())
        return;

    const Vector3f spawn = mLevel.getSpawnPositionForPlayer();

    player.setDead(false);
    player.getAttributes().set(HEALTH_ATTRIBUTE, maxHealthOf(player));
    player.setPosition(spawn);
    player.setRotation(Vector3f(0.0f, 0.0f, 0.0f));
    player.setMotion(Vector3f(0.0f, 0.0f, 0.0f));
    player.setOnGround(true);
    player.clearPendingMove();
    player.resetFallDistance();

    const Vector3f eyePosition(spawn.x, spawn.y + PLAYER_BASE_OFFSET, spawn.z);

    RespawnPacket respawn;
    respawn.mPosition = eyePosition;
    respawn.mState = RespawnPacket::State::ServerReady;
    respawn.mRuntimeEntityId = player.getRuntimeId();
    mNetworkHandler->send(player.getNetworkIdentifier(), respawn, mCodecContext);

    MovePlayerPacket move;
    move.mRuntimeEntityId = (int64_t) player.getRuntimeId();
    move.mPosition = eyePosition;
    move.mRotation = player.getRotation();
    move.mMode = MovePlayerMode::Respawn;
    move.mOnGround = true;
    mNetworkHandler->send(player.getNetworkIdentifier(), move, mCodecContext);

    player.setForceMoveSync(true);

    _sendHealth(player);
    _sendEntityData(player);
    _sendAbilities(player);
    _sendInventory(player);
    ItemEntityHandler::sendItemEntitiesTo(*this, player);
    _sendChunks(player);

    LOG_INFO(LogAreaID::Server, "%s respawned at %.2f %.2f %.2f", player.getName().c_str(), spawn.x, spawn.y,
             spawn.z);
}

void ServerNetworkHandler::handle(const NetworkIdentifier &id, const PlayerActionPacket &packet) {
    ServerPlayer *player = _getPlayer(id);
    if (player == nullptr || !player->isSpawned())
        return;

    const bool legacyBreakAction = packet.mAction == PlayerActionType::StartBreak ||
                                   packet.mAction == PlayerActionType::ContinueBreak ||
                                   packet.mAction == PlayerActionType::BlockContinueDestroy ||
                                   packet.mAction == PlayerActionType::BlockPredictDestroy ||
                                   packet.mAction == PlayerActionType::AbortBreak ||
                                   packet.mAction == PlayerActionType::StopBreak;

    if (legacyBreakAction ||
        (packet.mAction == PlayerActionType::DimensionChangeRequestOrCreativeDestroyBlock &&
         player->getGameType() != (int32_t) GameType::Creative)) {
        _disconnect(id, "Bad packet: invalid break channel");
        return;
    }

    if (packet.mAction == PlayerActionType::Respawn)
        _respawnPlayer(*player);
}

void ServerNetworkHandler::handle(const NetworkIdentifier &id, const RespawnPacket &packet) {
    ServerPlayer *player = _getPlayer(id);
    if (player == nullptr || !player->isSpawned())
        return;

    if (packet.mState != RespawnPacket::State::ClientReady)
        return;

    _respawnPlayer(*player);
}

ItemEntity *ServerNetworkHandler::dropItem(const Vector3f &position, const ItemStack &item, const Vector3f &motion,
                                           int pickupDelay) {
    return ItemEntityHandler::dropItem(*this, position, item, motion, pickupDelay);
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

    LoginHandler::handleSetLocalPlayerAsInitialized(*this, *player);
}

void ServerNetworkHandler::_addToPlayerList(ServerPlayer &player) {
    LoginHandler::addToPlayerList(*this, player);
}

void ServerNetworkHandler::_removeFromPlayerList(ServerPlayer &player) {
    LoginHandler::removeFromPlayerList(*this, player);
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
    if (player == nullptr || player->isDead())
        return;

    if (!std::isfinite(packet.mPosition.x) || !std::isfinite(packet.mPosition.y) ||
        !std::isfinite(packet.mPosition.z) || !std::isfinite(packet.mRotation.x) ||
        !std::isfinite(packet.mRotation.y) || !std::isfinite(packet.mRotation.z))
        return;

    std::string badPacketReason;
    if (BadPacketCheck::inspect(*player, packet, badPacketReason)) {
        _disconnect(id, "Bad packet: " + badPacketReason);
        return;
    }

    MovementHandler::handlePlayerAuthInput(*this, id, *player, packet);
}

namespace {
    const ItemData *findEdibleData(const ItemStack &item) {
        if (item.isAir() || item.mDefinition == nullptr)
            return nullptr;

        const ItemData *data = ItemDataTable::find(item.mDefinition->getIdentifier());
        if (data == nullptr || data->mNutrition <= 0)
            return nullptr;

        return data;
    }
}

void ServerNetworkHandler::_useHeldItem(ServerPlayer &player) {
    if (findEdibleData(player.getInventory().getItemInHand()) == nullptr)
        return;

    const bool usingItem = player.getFlags().get(EntityFlag::UsingItem);

    if (!player.canEat()) {
        if (usingItem) {
            player.getFlags().set(EntityFlag::UsingItem, false);
            _sendEntityData(player);
        }
        return;
    }

    if (usingItem)
        return;

    player.getFlags().set(EntityFlag::UsingItem, true);
    player.setItemUseStartTick(mCurrentTick);
    _sendEntityData(player);
}

void ServerNetworkHandler::_consumeHeldItem(ServerPlayer &player) {
    const bool wasUsing = player.getFlags().get(EntityFlag::UsingItem);
    const int64_t heldTicks = mCurrentTick - player.getItemUseStartTick();

    if (wasUsing) {
        player.getFlags().set(EntityFlag::UsingItem, false);
        _sendEntityData(player);
    }

    PlayerInventory &inventory = player.getInventory();
    const int slot = inventory.getSelectedSlot();

    const ItemData *data = findEdibleData(inventory.getItemInHand());
    if (data == nullptr)
        return;

    if (!wasUsing || heldTicks < FOOD_USE_DURATION_TICKS) {
        player.getInventoryManager().syncSlot(InventoryManager::InventoryId::Inventory, slot);
        return;
    }

    if (!player.canEat()) {
        player.getInventoryManager().syncSlot(InventoryManager::InventoryId::Inventory, slot);
        _sendAttributes(player);
        return;
    }

    player.consumeFood(data->mNutrition, data->mSaturation);

    ItemStack remaining = inventory.getItemInHand();
    remaining.mCount -= 1;
    if (remaining.mCount <= 0)
        remaining = ItemStack::air();

    inventory.setItemInHand(std::move(remaining));
    player.getInventoryManager().syncSlot(InventoryManager::InventoryId::Inventory, slot);

    _sendAttributes(player);

    LOG_INFO(LogAreaID::Server, "%s ate %s, food is now %.1f", player.getName().c_str(), data->mIdentifier,
             player.getFood());
}

namespace {
    std::string toLowerCopy(const std::string &value) {
        std::string lowered = value;
        for (char &character: lowered) {
            if (character >= 'A' && character <= 'Z')
                character = (char) (character - 'A' + 'a');
        }
        return lowered;
    }
}

ServerPlayer *ServerNetworkHandler::getPlayerByName(const std::string &name) {
    for (auto &entry: mPlayers) {
        if (entry.second.getName() == name)
            return &entry.second;
    }

    const std::string lowered = toLowerCopy(name);

    for (auto &entry: mPlayers) {
        if (toLowerCopy(entry.second.getName()) == lowered)
            return &entry.second;
    }

    ServerPlayer *partial = nullptr;
    for (auto &entry: mPlayers) {
        const std::string candidate = toLowerCopy(entry.second.getName());
        if (candidate.compare(0, lowered.size(), lowered) != 0)
            continue;

        if (partial != nullptr)
            return nullptr;

        partial = &entry.second;
    }

    return partial;
}

std::vector<std::string> ServerNetworkHandler::getPlayerNames() const {
    std::vector<std::string> names;
    names.reserve(mPlayers.size());

    for (const auto &entry: mPlayers) {
        if (!entry.second.getName().empty())
            names.push_back(entry.second.getName());
    }

    return names;
}

std::vector<ServerPlayer *> ServerNetworkHandler::resolveTargets(CommandSender &sender,
                                                                 const std::string &selector) {
    std::vector<ServerPlayer *> targets;

    if (selector == "@a" || selector == "@e") {
        for (auto &entry: mPlayers) {
            if (entry.second.isSpawned())
                targets.push_back(&entry.second);
        }
        return targets;
    }

    if (selector == "@s" || selector == "@p") {
        ServerPlayer *self = sender.asPlayer();
        if (self != nullptr)
            targets.push_back(self);
        return targets;
    }

    if (selector == "@r") {
        for (auto &entry: mPlayers) {
            if (entry.second.isSpawned()) {
                targets.push_back(&entry.second);
                break;
            }
        }
        return targets;
    }

    ServerPlayer *named = getPlayerByName(selector);
    if (named != nullptr)
        targets.push_back(named);

    return targets;
}

void ServerNetworkHandler::setPlayerGameMode(ServerPlayer &player, int gameMode) {
    player.setGameType(gameMode);
    player.setHungerEnabled(gameMode == (int32_t) GameType::Survival);

    const bool mayFly = gameMode == (int32_t) GameType::Creative || gameMode == (int32_t) GameType::Spectator;
    if (!mayFly && player.isFlying()) {
        player.setFlying(false);
        player.setOnGround(MovementHandler::checkGroundState(*this, player.getPosition()));
    }

    _sendAbilities(player);

    SetPlayerGameTypePacket packet;
    packet.mGamemode = gameMode;
    mNetworkHandler->send(player.getNetworkIdentifier(), packet, mCodecContext);

    player.resetFallDistance();
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

    ChatHandler::broadcastChat(*this, *player, packet.mMessage);
}

void ServerNetworkHandler::handle(const NetworkIdentifier &id, const RequestChunkRadiusPacket &packet) {
    ServerPlayer *player = _getPlayer(id);
    if (player == nullptr)
        return;

    std::string badPacketReason;
    if (BadPacketCheck::inspect(*player, packet, badPacketReason)) {
        _disconnect(id, "Bad packet: " + badPacketReason);
        return;
    }

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
    LoginHandler::checkTerrainReady(*this, player);
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

    if (!packet.mBoolValue)
        player->setOnGround(MovementHandler::checkGroundState(*this, player->getPosition()));

    _sendAbilities(*player);
}

void ServerNetworkHandler::handle(const NetworkIdentifier &id, const MobEquipmentPacket &packet) {
    ServerPlayer *player = _getPlayer(id);
    if (player == nullptr || !player->isSpawned())
        return;

    std::string badPacketReason;
    if (BadPacketCheck::inspect(*player, packet, badPacketReason)) {
        _disconnect(id, "Bad packet: " + badPacketReason);
        return;
    }

    InventoryHandler::handleMobEquipment(*this, *player, packet);
}

void ServerNetworkHandler::handle(const NetworkIdentifier &id, const PlayerHotbarPacket &packet) {
    ServerPlayer *player = _getPlayer(id);
    if (player == nullptr || !player->isSpawned())
        return;

    InventoryHandler::handlePlayerHotbar(*this, *player, packet);
}

void ServerNetworkHandler::handle(const NetworkIdentifier &id, const InteractPacket &packet) {
    ServerPlayer *player = _getPlayer(id);
    if (player == nullptr || !player->isSpawned())
        return;

    if (packet.mAction != InteractPacket::Action::OpenInventory)
        return;

    if (packet.mRuntimeEntityId != player->getRuntimeId())
        return;

    InventoryHandler::handleOpenInventory(*player);
}

void ServerNetworkHandler::handle(const NetworkIdentifier &id, const ItemStackRequestPacket &packet) {
    ServerPlayer *player = _getPlayer(id);
    if (player == nullptr || !player->isSpawned())
        return;

    std::string badPacketReason;
    if (BadPacketCheck::inspect(*player, packet, badPacketReason)) {
        _disconnect(id, "Bad packet: " + badPacketReason);
        return;
    }

    InventoryHandler::handleItemStackRequest(*this, id, *player, packet);
}

void ServerNetworkHandler::handle(const NetworkIdentifier &id, const InventoryTransactionPacket &packet) {
    ServerPlayer *player = _getPlayer(id);
    if (player == nullptr || !player->isSpawned())
        return;

    std::string badPacketReason;
    if (BadPacketCheck::inspect(*player, packet, badPacketReason)) {
        _disconnect(id, "Bad packet: " + badPacketReason);
        return;
    }

    InventoryHandler::handleTransaction(*this, *player, packet);
}

void ServerNetworkHandler::handle(const NetworkIdentifier &id, const ContainerClosePacket &packet) {
    ServerPlayer *player = _getPlayer(id);
    if (player == nullptr || !player->isSpawned())
        return;

    InventoryHandler::handleContainerClose(*player, packet);
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

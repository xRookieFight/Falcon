#include "Network/LoginHandler.h"

#include "Block/Components/CreativeContentTable.h"
#include "Block/Blocks/VanillaBlocks.h"
#include "Command/Command.h"
#include "Core/Debug/BedrockLog.h"
#include "Core/NBT/NbtIo.h"
#include "Core/Utility/ReadOnlyBinaryStream.h"
#include "Actor/ActorAttributes.h"
#include "Actor/PlayerAbility.h"
#include "Actor/ServerPlayer.h"
#include "Item/CraftingRecipeTable.h"
#include "Item/ItemNetworkIdTable.h"
#include "Level/Level.h"
#include "Network/ConnectionRequest.h"
#include "Network/InventoryHandler.h"
#include "Network/ItemActorHandler.h"
#include "Network/LoginChainVerifier.h"
#include "Network/NetworkHandler.h"
#include "Network/ServerNetworkHandler.h"
#include "Protocol/Packets/AvailableCommandsPacket.h"
#include "Protocol/Packets/AvailableActorIdentifiersPacket.h"
#include "Protocol/Packets/BiomeDefinitionListPacket.h"
#include "Protocol/Packets/CreativeContentPacket.h"
#include "Protocol/Packets/ItemRegistryPacket.h"
#include "Protocol/Packets/LoginPacket.h"
#include "Protocol/Packets/PlayerListPacket.h"
#include "Protocol/Packets/PlayStatusPacket.h"
#include "Protocol/Packets/NetworkSettingsPacket.h"
#include "Protocol/Packets/RequestNetworkSettingsPacket.h"
#include "Protocol/Packets/ResourcePackChunkDataPacket.h"
#include "Protocol/Packets/ResourcePackChunkRequestPacket.h"
#include "Protocol/Packets/ResourcePackClientResponsePacket.h"
#include "Protocol/Packets/ResourcePackDataInfoPacket.h"
#include "Protocol/Packets/ResourcePackStackPacket.h"
#include "Protocol/Packets/ResourcePacksInfoPacket.h"
#include "Protocol/Packets/StartGamePacket.h"
#include "Protocol/Packets/SetTimePacket.h"
#include "Protocol/Packets/UpdateAbilitiesPacket.h"
#include "Protocol/Packets/UpdateAttributesPacket.h"
#include "Server/PropertiesSettings.h"
#include "Server/ResourcePackManager.h"

namespace {
    const int64_t RESOURCE_PACK_CHUNK_SIZE = 1024 * 1024;
    const float PLAYER_BASE_OFFSET = 1.62f;
    const size_t SPAWN_CHUNK_THRESHOLD = 56;

    Uuid listUuidFor(const ServerPlayer &player) {
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

    std::string packRecipeId(uint32_t value) {
        std::string result(4, '\0');
        result[0] = (char) (value >> 24);
        result[1] = (char) (value >> 16);
        result[2] = (char) (value >> 8);
        result[3] = (char) value;
        return result;
    }
}

void LoginHandler::registerVanillaDefinitions(ServerNetworkHandler &owner) {
    for (const Block &block: VanillaBlocks::getAll()) {
        owner.getBlockDefinitions().registerDefinition(std::make_shared<BlockDefinition>(
                block.getIdentifier(), block.getNetworkHash(), block.getStates()));
    }

    for (size_t index = 0; index < ItemNetworkIdTable::getCount(); ++index) {
        const ItemNetworkIdEntry &entry = ItemNetworkIdTable::getEntries()[index];

        Tag components = Tag::ofCompound();
        if (entry.mComponentNbt != nullptr) {
            ReadOnlyBinaryStream stream(std::string((const char *) entry.mComponentNbt, entry.mComponentNbtSize));
            components = NbtIo::readTag(stream, NbtVariant::LittleEndian);
        }

        owner.getItemDefinitions().registerDefinition(std::make_shared<ItemDefinition>(
                entry.mIdentifier, entry.mNetworkId, entry.mComponentBased, components));
    }

    LOG_TRACE(LogAreaID::Server, "Registered %zu block(s) and %zu item definition(s)",
             VanillaBlocks::getAll().size(), owner.getItemDefinitions().size());
}

void LoginHandler::handleRequestNetworkSettings(ServerNetworkHandler &owner, const NetworkIdentifier &id,
                                                const RequestNetworkSettingsPacket &packet) {
    LOG_INFO(LogAreaID::Network, "%s requested network settings, client protocol version %d",
             id.getAddress().c_str(), packet.mProtocolVersion);

    if (packet.mProtocolVersion != owner.getAnnouncement().mProtocolVersion) {
        PlayStatusPacket status;
        status.mStatus = packet.mProtocolVersion < owner.getAnnouncement().mProtocolVersion
                         ? PlayStatusPacket::Status::LoginFailedClientOld
                         : PlayStatusPacket::Status::LoginFailedServerOld;

        owner.getNetworkHandler().send(id, status, owner.getCodecContext());
        owner.getNetworkHandler().flush(id);

        LOG_WARN(LogAreaID::Network, "%s uses protocol %d but the server runs %d", id.getAddress().c_str(),
                 packet.mProtocolVersion, owner.getAnnouncement().mProtocolVersion);
        return;
    }

    NetworkSettingsPacket settings;
    settings.mCompressionThreshold = owner.getProperties().getCompressionThreshold();
    settings.mCompressionAlgorithm = owner.getProperties().getCompressionAlgorithm();
    settings.mClientThrottleEnabled = false;
    settings.mClientThrottleThreshold = 0;
    settings.mClientThrottleScalar = 0.0f;

    owner.getNetworkHandler().send(id, settings, owner.getCodecContext());

    owner.getNetworkHandler().flush(id);
    owner.getNetworkHandler().enableCompression(id, CompressedNetworkPeer::CompressionAlgorithm::ZLib,
                                                settings.mCompressionThreshold);

    owner.getPlayers().erase(id);
    auto inserted = owner.getPlayers().try_emplace(id, id, owner.allocateRuntimeId(), &owner);
    ServerPlayer &player = inserted.first->second;
    player.setLoginState(ServerPlayer::LoginState::NetworkSettingsSent);
}

void LoginHandler::handleLogin(ServerNetworkHandler &owner, const NetworkIdentifier &id, ServerPlayer &player,
                               const LoginPacket &packet) {
    ConnectionRequest request;
    if (!request.parse(packet.mAuthJwt, packet.mClientJwt)) {
        LOG_WARN(LogAreaID::Network, "%s sent a login that could not be parsed", id.getAddress().c_str());
        owner._disconnect(id, "Malformed login");
        return;
    }

    LoginChainVerifier verifier;
    const bool verified = verifier.verify(packet.mAuthJwt, packet.mClientJwt);

    if (!verified) {
        LOG_WARN(LogAreaID::Network, "%s sent a login that failed verification: %s", id.getAddress().c_str(),
                 verifier.getFailureReason().c_str());
        owner._disconnect(id, verifier.getFailureReason());
        owner.getPlayers().erase(id);
        return;
    }

    if (owner.getProperties().getXboxAuthRequired() && !verifier.isSigned()) {
        LOG_WARN(LogAreaID::Network, "%s failed Xbox Live authentication", id.getAddress().c_str());
        owner._disconnect(id, "You must be authenticated with Xbox Live to join");
        owner.getPlayers().erase(id);
        return;
    }

    if (verifier.isSigned()) {
        player.setName(verifier.getDisplayName());
        player.setUuid(verifier.getIdentity());
        player.setXuid(verifier.getXuid());
    } else {
        const std::string &name = verifier.getDisplayName().empty() ? request.getDisplayName()
                                                                    : verifier.getDisplayName();
        const std::string &uuid = verifier.getIdentity().empty() ? request.getIdentity() : verifier.getIdentity();

        player.setName(name);
        player.setUuid(uuid);
        player.setXuid("");
    }

    player.setSkin(request.getSkin());
    player.setBuildPlatform(request.getBuildPlatform());
    player.setLoginState(ServerPlayer::LoginState::LoggedIn);

    LOG_INFO(LogAreaID::Server, "Player %s logged in, uuid %s, xuid %s", player.getName().c_str(),
             player.getUuid().c_str(), player.getXuid().c_str());

    PlayStatusPacket status;
    status.mStatus = PlayStatusPacket::Status::LoginSuccess;
    owner.getNetworkHandler().send(id, status, owner.getCodecContext());

    ResourcePacksInfoPacket packs;
    packs.mForcedToAccept = owner.getProperties().getTexturePackRequired();
    packs.mWorldTemplateVersion = "";
    packs.mHasAddonPacks = false;
    packs.mScriptingEnabled = false;
    packs.mVibrantVisualsForceDisabled = false;

    for (const ResourcePack &pack: owner.getResourcePacks().getPacks()) {
        ResourcePacksInfoPacket::Entry entry;
        entry.mPackId = pack.mUuid;
        entry.mPackVersion = pack.mVersion;
        entry.mPackSize = pack.mSize;
        entry.mContentKey = pack.mContentKey;
        entry.mContentId = pack.mUuidString;
        packs.mResourcePackInfos.push_back(entry);
    }

    owner.getNetworkHandler().send(id, packs, owner.getCodecContext());

    player.setLoginState(ServerPlayer::LoginState::ResourcePacksSent);
}

void LoginHandler::handleResourcePackClientResponse(ServerNetworkHandler &owner, const NetworkIdentifier &id,
                                                    ServerPlayer &player,
                                                    const ResourcePackClientResponsePacket &packet) {
    switch (packet.mStatus) {
        case ResourcePackClientResponsePacket::Status::SendPacks: {
            for (const std::string &requested: packet.mPackIds) {
                const std::string uuid = requested.substr(0, requested.find('_'));
                const ResourcePack *pack = owner.getResourcePacks().findById(uuid);
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
                owner.getNetworkHandler().send(id, info, owner.getCodecContext());
            }
            break;
        }

        case ResourcePackClientResponsePacket::Status::HaveAllPacks: {
            ResourcePackStackPacket stack;
            stack.mForcedToAccept = owner.getProperties().getTexturePackRequired();
            stack.mGameVersion = owner.getAnnouncement().mGameVersion;

            for (const ResourcePack &pack: owner.getResourcePacks().getPacks()) {
                ResourcePackStackPacket::Entry entry;
                entry.mPackId = pack.mUuidString;
                entry.mPackVersion = pack.mVersion;
                stack.mResourcePacks.push_back(entry);
            }

            owner.getNetworkHandler().send(id, stack, owner.getCodecContext());
            break;
        }

        case ResourcePackClientResponsePacket::Status::Completed:
            sendStartGame(owner, player);
            break;

        case ResourcePackClientResponsePacket::Status::Refused:
            owner._disconnect(id, "You must accept the resource packs to join");
            break;

        default:
            break;
    }
}

void LoginHandler::handleResourcePackChunkRequest(ServerNetworkHandler &owner, const NetworkIdentifier &id,
                                                  const ResourcePackChunkRequestPacket &packet) {
    const ResourcePack *pack = owner.getResourcePacks().findById(packet.mPackId.toString());
    if (pack == nullptr) {
        LOG_WARN(LogAreaID::Network, "Client requested unknown resource pack %s", packet.mPackId.toString().c_str());
        return;
    }

    const uint64_t offset = (uint64_t) packet.mChunkIndex * (uint64_t) RESOURCE_PACK_CHUNK_SIZE;
    if (offset >= pack->mSize)
        return;

    const uint64_t remaining = pack->mSize - offset;
    const uint64_t length = remaining < (uint64_t) RESOURCE_PACK_CHUNK_SIZE ? remaining
                                                                            : (uint64_t) RESOURCE_PACK_CHUNK_SIZE;

    ResourcePackChunkDataPacket chunk;
    chunk.mPackId = pack->mUuid;
    chunk.mPackVersion = pack->mVersion;
    chunk.mChunkIndex = packet.mChunkIndex;
    chunk.mProgress = (int64_t) offset;
    chunk.mData = pack->mData.substr(offset, length);
    owner.getNetworkHandler().send(id, chunk, owner.getCodecContext());
}

void LoginHandler::sendStartGame(ServerNetworkHandler &owner, ServerPlayer &player) {
    const NetworkIdentifier &id = player.getNetworkIdentifier();

    player.getFlags().applyPlayerDefaults();
    player.getAttributes() = ActorAttributes::createPlayerDefaults();
    player.setGameType((int32_t) owner.getProperties().getGameType());
    owner._loadPlayerData(player);
    player.setHungerEnabled(player.getGameType() == (int32_t) GameType::Survival);

    StartGamePacket startGame;
    startGame.mUniqueActorId = player.getUniqueId();
    startGame.mRuntimeActorId = player.getRuntimeId();
    startGame.mPlayerGameType = (GameType) player.getGameType();
    startGame.mPlayerPosition = Vector3f(player.getPosition().x, player.getPosition().y + PLAYER_BASE_OFFSET,
                                        player.getPosition().z);
    startGame.mRotation = Vector2f(player.getRotation().x, player.getRotation().y);

    startGame.mSeed = 0;
    startGame.mDimensionId = 0;
    startGame.mGeneratorId = 1;
    startGame.mLevelGameType = owner.getProperties().getGameType();
    startGame.mDifficulty = (int32_t) owner.getProperties().getDifficulty();
    startGame.mDefaultSpawn = owner.getLevel().getSpawnPosition();
    startGame.mCommandsEnabled = owner.getProperties().getAllowCheats();
    startGame.mTexturePacksRequired = owner.getProperties().getTexturePackRequired();
    startGame.mDefaultPlayerPermission = owner.getProperties().getDefaultPlayerPermissionLevel();
    startGame.mChatRestrictionLevel = owner.getProperties().getChatRestrictionLevel();
    startGame.mDisablingPlayerInteractions = owner.getProperties().getDisablePlayerInteraction();
    startGame.mClientSideGenerationEnabled = owner.getProperties().getClientSideChunkGenerationEnabled();
    startGame.mDisablingCustomSkins = owner.getProperties().getDisableCustomSkins();
    startGame.mServerChunkTickRange = owner.getProperties().getTickDistance();
    startGame.mVanillaVersion = owner.getAnnouncement().mGameVersion;
    startGame.mLevelId = "RmFsY29u";
    startGame.mLevelName = owner.getLevel().getName();
    startGame.mMultiplayerCorrelationId = "";
    startGame.mServerEngine = "Falcon";
    startGame.mCurrentTick = 0;
    startGame.mEnchantmentSeed = 0;
    startGame.mBlockNetworkIdsHashed = owner.getProperties().getBlockNetworkIdsAreHashes();
    startGame.mInventoriesServerAuthoritative = true;

    startGame.mGamerules.push_back(GameRuleData::ofBool("showcoordinates", true));

    owner.getNetworkHandler().send(id, startGame, owner.getCodecContext());

    SetTimePacket time;
    time.mTime = (int32_t) owner.getLevel().getDayTime();
    owner.getNetworkHandler().send(id, time, owner.getCodecContext());

    player.setLoginState(ServerPlayer::LoginState::StartGameSent);
    LOG_INFO(LogAreaID::Server, "Sent StartGame to %s", player.getName().c_str());

    player.getInventoryManager().attach(&player, &owner);

    sendItemComponents(owner, player);
    sendActorIdentifiers(owner, player);
    sendBiomeDefinitions(owner, player);
    sendAttributes(owner, player);
    sendAvailableCommands(owner, player);
    sendAbilities(owner, player);
    owner._sendEntityData(player);

    player.getInventoryManager().syncAll();
    player.getInventoryManager().syncSelectedHotbarSlot();

    sendCreativeContent(owner, player);
    owner._sendCraftingData(player);

    addToPlayerList(owner, player);
}

void LoginHandler::sendAbilities(ServerNetworkHandler &owner, ServerPlayer &player) {
    const int32_t gameType = player.getGameType();
    const bool isCreative = gameType == (int32_t) GameType::Creative;
    const bool isSpectator = gameType == (int32_t) GameType::Spectator;
    const bool isAdventure = gameType == (int32_t) GameType::Adventure;

    const uint32_t abilitiesSet = (1u << ((int) PlayerAbility::VerticalFlySpeed + 1)) - 1u;

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
    abilities.mAbilities.mUniqueActorId = player.getUniqueId();
    abilities.mAbilities.mPlayerPermission = (uint8_t) (player.isOp() ? PlayerPermission::Operator
                                                                     : owner.getProperties().getDefaultPlayerPermissionLevel());
    abilities.mAbilities.mCommandPermission = (uint8_t) (player.isOp() ? CommandPermission::GameDirectors
                                                                       : CommandPermission::Any);
    abilities.mAbilities.mAbilityLayers.push_back(layer);

    owner.getNetworkHandler().send(player.getNetworkIdentifier(), abilities, owner.getCodecContext());
}

void LoginHandler::sendBiomeDefinitions(ServerNetworkHandler &owner, ServerPlayer &player) {
    BiomeDefinitionListPacket biomes;
    biomes.mBiomes = owner.getBiomes().getBiomes();

    owner.getNetworkHandler().send(player.getNetworkIdentifier(), biomes, owner.getCodecContext());
}

void LoginHandler::sendItemComponents(ServerNetworkHandler &owner, ServerPlayer &player) {
    ItemRegistryPacket components;

    for (const std::shared_ptr<ItemDefinition> &definition: owner.getItemDefinitions().getAll()) {
        ItemComponentEntry entry;
        entry.mIdentifier = definition->getIdentifier();
        entry.mRuntimeId = (int16_t) definition->getRuntimeId();
        entry.mComponentBased = definition->isComponentBased();

        const ItemNetworkIdEntry *source = ItemNetworkIdTable::find(definition->getIdentifier());
        entry.mItemVersion = source == nullptr ? 0 : source->mVersion;

        entry.mComponentData = definition->getComponentData();
        components.mEntries.push_back(entry);
    }

    owner.getNetworkHandler().send(player.getNetworkIdentifier(), components, owner.getCodecContext());

    LOG_INFO(LogAreaID::Server, "Sent %zu item component(s) to %s", components.mEntries.size(),
             player.getName().c_str());
}

void LoginHandler::sendActorIdentifiers(ServerNetworkHandler &owner, ServerPlayer &player) {
    AvailableActorIdentifiersPacket identifiers;
    identifiers.mIdentifiers = Tag::ofCompound();
    identifiers.mIdentifiers.put("idlist", Tag::ofList(Tag::Type::Compound));

    owner.getNetworkHandler().send(player.getNetworkIdentifier(), identifiers, owner.getCodecContext());
}

void LoginHandler::buildCraftingData(ServerNetworkHandler &owner) {
    if (!owner.getRecipeOutputsMutable().empty())
        return;

    const CraftingIngredientData *ingredients = CraftingRecipeTable::getIngredients();
    const CraftingOutputData *outputs = CraftingRecipeTable::getOutputs();
    const CraftingRecipeData *recipes = CraftingRecipeTable::getRecipes();

    CraftingDataPacket &cached = owner.getCachedCraftingData();
    std::vector<ItemStack> &recipeOutputs = owner.getRecipeOutputsMutable();

    for (size_t index = 0; index < CraftingRecipeTable::getRecipeCount(); ++index) {
        const CraftingRecipeData &source = recipes[index];

        CraftingRecipeEntry entry;
        entry.mWidth = source.mWidth;
        entry.mHeight = source.mHeight;
        entry.mUuid = Uuid();
        entry.mBlockName = "crafting_table";
        entry.mPriority = 50;
        entry.mSymmetric = source.mWidth > 0;
        entry.mRecipeNetId = (int32_t) recipeOutputs.size() + 1;
        entry.mRecipeId = packRecipeId((uint32_t) entry.mRecipeNetId);

        for (uint32_t i = 0; i < source.mIngredientCount; ++i) {
            const CraftingIngredientData &ingredient = ingredients[source.mIngredientOffset + i];

            RecipeIngredientEntry parsed;
            if (ingredient.mItemId != nullptr) {
                parsed.mHasItem = true;
                parsed.mItemId = ingredient.mItemId;
                parsed.mAuxValue = ingredient.mAuxValue < 0 ? 0x7fff : ingredient.mAuxValue;
                parsed.mCount = ingredient.mCount;
            } else {
                parsed.mCount = 0;
            }

            entry.mInputs.push_back(parsed);
        }

        ItemStack resolvedOutput;

        for (uint32_t i = 0; i < source.mOutputCount; ++i) {
            const CraftingOutputData &output = outputs[source.mOutputOffset + i];
            const ItemNetworkIdEntry *networkEntry = ItemNetworkIdTable::find(output.mItemId);

            if (networkEntry == nullptr)
                continue;

            RecipeOutputEntry parsedOutput;
            parsedOutput.mRuntimeId = networkEntry->mNetworkId;
            parsedOutput.mCount = output.mCount;
            parsedOutput.mMeta = 0;
            parsedOutput.mIsShield = std::string(output.mItemId) == "minecraft:shield";
            const auto blockDefinition = owner.getBlockDefinitions().getDefinition(output.mItemId);
            parsedOutput.mBlockRuntimeId = blockDefinition == nullptr ? 0 : blockDefinition->getRuntimeId();
            entry.mOutputs.push_back(parsedOutput);

            if (i == 0) {
                resolvedOutput.mDefinition = owner.getItemDefinitions().getDefinition(output.mItemId);
                resolvedOutput.mBlockDefinition = owner.getBlockDefinitions().getDefinition(output.mItemId);
                resolvedOutput.mCount = output.mCount;
            }
        }

        if (entry.mOutputs.empty())
            continue;

        if (source.mWidth > 0)
            cached.mShapedRecipes.push_back(entry);
        else
            cached.mShapelessRecipes.push_back(entry);

        recipeOutputs.push_back(resolvedOutput);
    }

    cached.mCleanRecipes = true;
}

void LoginHandler::sendCraftingData(ServerNetworkHandler &owner, ServerPlayer &player) {
    buildCraftingData(owner);

    owner.getNetworkHandler().send(player.getNetworkIdentifier(), owner.getCachedCraftingData(),
                                   owner.getCodecContext());
}

void LoginHandler::buildCreativeContent(ServerNetworkHandler &owner) {
    if (!owner.getCreativeItemsMutable().empty())
        return;

    for (size_t index = 0; index < CreativeContentTable::getGroupCount(); ++index) {
        const CreativeGroupEntry &source = CreativeContentTable::getGroups()[index];

        CreativeItemGroup group;
        group.mCategory = source.mCategory;
        group.mName = source.mName;
        group.mIcon = ItemStack::air();

        std::shared_ptr<ItemDefinition> icon = owner.getItemDefinitions().getDefinition(source.mIconIdentifier);
        if (icon != nullptr) {
            group.mIcon.mDefinition = icon;
            group.mIcon.mBlockDefinition = owner.getBlockDefinitions().getDefinition(source.mIconIdentifier);
            group.mIcon.mCount = 1;
        }

        owner.getCreativeGroups().push_back(group);
    }

    int32_t netId = 1;

    for (size_t index = 0; index < CreativeContentTable::getEntryCount(); ++index) {
        const CreativeEntry &source = CreativeContentTable::getEntries()[index];

        std::shared_ptr<ItemDefinition> definition = owner.getItemDefinitions().getDefinition(source.mIdentifier);
        if (definition == nullptr)
            continue;

        CreativeItemData entry;
        entry.mNetId = netId++;
        entry.mGroupIndex = source.mGroupIndex;
        entry.mItem.mDefinition = definition;
        entry.mItem.mCount = 1;
        entry.mItem.mDamage = source.mDamage;

        if (source.mNbt != nullptr) {
            ReadOnlyBinaryStream stream(std::string((const char *) source.mNbt, source.mNbtSize));
            entry.mItem.mTag = NbtIo::readTag(stream, NbtVariant::LittleEndian);
        }

        if (source.mIsBlock)
            entry.mItem.mBlockDefinition = owner.getBlockDefinitions().getDefinition(source.mIdentifier);

        owner.getCreativeItemsMutable().push_back(entry);
    }
}

void LoginHandler::sendCreativeContent(ServerNetworkHandler &owner, ServerPlayer &player) {
    buildCreativeContent(owner);

    CreativeContentPacket creative;
    creative.mGroups = owner.getCreativeGroups();
    creative.mItems = owner.getCreativeItemsMutable();

    owner.getNetworkHandler().send(player.getNetworkIdentifier(), creative, owner.getCodecContext());

    LOG_INFO(LogAreaID::Server, "Sent %zu creative item(s) to %s", creative.mItems.size(), player.getName().c_str());
}

void LoginHandler::sendAvailableCommands(ServerNetworkHandler &owner, ServerPlayer &player) {
    AvailableCommandsPacket availableCommands;

    for (Command *command: owner.getCommands().getCommands()) {
        if ((int) player.getCommandPermission() < (int) command->getRequiredPermission())
            continue;

        CommandData data;
        data.mName = command->getName();
        data.mDescription = command->getDescription();
        data.mPermission = command->getRequiredPermission();

        data.mOverloads = command->getOverloads();

        availableCommands.mCommands.push_back(data);
    }

    owner.getNetworkHandler().send(player.getNetworkIdentifier(), availableCommands, owner.getCodecContext());
}

void LoginHandler::sendAttributes(ServerNetworkHandler &owner, ServerPlayer &player) {
    UpdateAttributesPacket attributes;
    attributes.mRuntimeActorId = (int64_t) player.getRuntimeId();
    attributes.mTick = 0;
    attributes.mAttributes = player.getAttributes().getAll();

    owner.getNetworkHandler().send(player.getNetworkIdentifier(), attributes, owner.getCodecContext());
}

void LoginHandler::addToPlayerList(ServerNetworkHandler &owner, ServerPlayer &player) {
    PlayerListPacket::Entry newEntry(listUuidFor(player));
    newEntry.mActorId = player.getUniqueId();
    newEntry.mName = player.getName();
    newEntry.mXuid = player.getXuid();
    newEntry.mSkin = player.getSkin();
    newEntry.mBuildPlatform = player.getBuildPlatform();

    PlayerListPacket announce;
    announce.mEntries.push_back(newEntry);

    PlayerListPacket existing;

    for (auto &entry: owner.getPlayers()) {
        if (!entry.second.isSpawned())
            continue;

        owner.getNetworkHandler().send(entry.second.getNetworkIdentifier(), announce, owner.getCodecContext());

        if (&entry.second != &player) {
            PlayerListPacket::Entry existingEntry(listUuidFor(entry.second));
            existingEntry.mActorId = entry.second.getUniqueId();
            existingEntry.mName = entry.second.getName();
            existingEntry.mXuid = entry.second.getXuid();
            existingEntry.mSkin = entry.second.getSkin();
            existingEntry.mBuildPlatform = entry.second.getBuildPlatform();
            existing.mEntries.push_back(existingEntry);
        }
    }

    if (!existing.mEntries.empty())
        owner.getNetworkHandler().send(player.getNetworkIdentifier(), existing, owner.getCodecContext());
}

void LoginHandler::removeFromPlayerList(ServerNetworkHandler &owner, ServerPlayer &player) {
    PlayerListPacket::Entry removalEntry(listUuidFor(player));
    removalEntry.mAction = PlayerListPacket::Action::Remove;

    PlayerListPacket removal;
    removal.mEntries.push_back(removalEntry);

    for (auto &entry: owner.getPlayers()) {
        if (entry.second.isSpawned() && &entry.second != &player)
            owner.getNetworkHandler().send(entry.second.getNetworkIdentifier(), removal, owner.getCodecContext());
    }
}

void LoginHandler::handleSetLocalPlayerAsInitialized(ServerNetworkHandler &owner, ServerPlayer &player) {
    player.setLoginState(ServerPlayer::LoginState::Spawned);
    LOG_INFO(LogAreaID::Server, "Player %s spawned", player.getName().c_str());

    player.setEffectsNetworkReady(true);
    player.syncEffects();

    owner._sendInventory(player);
    owner._sendHealth(player);
    ItemActorHandler::sendItemActorsTo(owner, player);

    owner.broadcastTranslation("multiplayer.player.joined", {player.getName()});
}

void LoginHandler::checkTerrainReady(ServerNetworkHandler &owner, ServerPlayer &player) {
    if (player.hasSpawnChunksReady())
        return;

    if (player.getSentChunkCount() < SPAWN_CHUNK_THRESHOLD)
        return;

    player.setSpawnChunksReady(true);

    PlayStatusPacket spawn;
    spawn.mStatus = PlayStatusPacket::Status::PlayerSpawn;
    owner.getNetworkHandler().send(player.getNetworkIdentifier(), spawn, owner.getCodecContext());

    LOG_INFO(LogAreaID::Server, "Terrain ready for %s after %u chunks", player.getName().c_str(),
             (unsigned) player.getSentChunkCount());
}

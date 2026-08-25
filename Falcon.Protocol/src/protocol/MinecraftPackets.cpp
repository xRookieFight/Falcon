#include "protocol/MinecraftPackets.h"

#include "protocol/packets/AddBehaviorTreePacket.h"
#include "protocol/packets/AddActorPacket.h"
#include "protocol/packets/AddItemActorPacket.h"
#include "protocol/packets/AddPaintingPacket.h"
#include "protocol/packets/AddPlayerPacket.h"
#include "protocol/packets/AddVolumeEntityPacket.h"
#include "protocol/packets/AdventureSettingsPacket.h"
#include "protocol/packets/AgentActionEventPacket.h"
#include "protocol/packets/AgentAnimationPacket.h"
#include "protocol/packets/AnimateEntityPacket.h"
#include "protocol/packets/AnimatePacket.h"
#include "protocol/packets/AnvilDamagePacket.h"
#include "protocol/packets/AutomationClientConnectPacket.h"
#include "protocol/packets/AwardAchievementPacket.h"
#include "protocol/packets/AvailableCommandsPacket.h"
#include "protocol/packets/AvailableActorIdentifiersPacket.h"
#include "protocol/packets/BiomeDefinitionListPacket.h"
#include "protocol/packets/BlockActorDataPacket.h"
#include "protocol/packets/BlockEventPacket.h"
#include "protocol/packets/BlockPickRequestPacket.h"
#include "protocol/packets/BookEditPacket.h"
#include "protocol/packets/BossEventPacket.h"
#include "protocol/packets/CameraPacket.h"
#include "protocol/packets/CameraAimAssistPacket.h"
#include "protocol/packets/CameraAimAssistActorPriorityPacket.h"
#include "protocol/packets/ClientCameraAimAssistPacket.h"
#include "protocol/packets/CameraAimAssistPresetsPacket.h"
#include "protocol/packets/CameraInstructionPacket.h"
#include "protocol/packets/CameraPresetsPacket.h"
#include "protocol/packets/CameraShakePacket.h"
#include "protocol/packets/CameraSplinePacket.h"
#include "protocol/packets/ChangeDimensionPacket.h"
#include "protocol/packets/ChangeMobPropertyPacket.h"
#include "protocol/packets/ChunkRadiusUpdatedPacket.h"
#include "protocol/packets/ClientCacheBlobStatusPacket.h"
#include "protocol/packets/ClientCacheMissResponsePacket.h"
#include "protocol/packets/ClientCacheStatusPacket.h"
#include "protocol/packets/ClientCheatAbilityPacket.h"
#include "protocol/packets/ClientToServerHandshakePacket.h"
#include "protocol/packets/ClientboundAttributeLayerSyncPacket.h"
#include "protocol/packets/ClientboundCloseFormPacket.h"
#include "protocol/packets/ClientboundControlSchemeSetPacket.h"
#include "protocol/packets/ClientboundDataDrivenUICloseScreenPacket.h"
#include "protocol/packets/ClientboundDataDrivenUIReloadPacket.h"
#include "protocol/packets/ClientboundDataDrivenUIShowScreenPacket.h"
#include "protocol/packets/ClientboundDataStorePacket.h"
#include "protocol/packets/ClientboundDebugRendererPacket.h"
#include "protocol/packets/ClientboundMapItemDataPacket.h"
#include "protocol/packets/ClientboundTextureShiftPacket.h"
#include "protocol/packets/ClientboundUpdateSoundDataPacket.h"
#include "protocol/packets/CodeBuilderPacket.h"
#include "protocol/packets/CommandBlockUpdatePacket.h"
#include "protocol/packets/CommandOutputPacket.h"
#include "protocol/packets/CommandRequestPacket.h"
#include "protocol/packets/CodeBuilderSourcePacket.h"
#include "protocol/packets/CompletedUsingItemPacket.h"
#include "protocol/packets/ContainerClosePacket.h"
#include "protocol/packets/ContainerOpenPacket.h"
#include "protocol/packets/ContainerRegistryCleanupPacket.h"
#include "protocol/packets/ContainerSetDataPacket.h"
#include "protocol/packets/CorrectPlayerMovePredictionPacket.h"
#include "protocol/packets/CraftingDataPacket.h"
#include "protocol/packets/CraftingEventPacket.h"
#include "protocol/packets/CreatePhotoPacket.h"
#include "protocol/packets/CreativeContentPacket.h"
#include "protocol/packets/CurrentStructureFeaturePacket.h"
#include "protocol/packets/DeathInfoPacket.h"
#include "protocol/packets/PrimitiveShapesPacket.h"
#include "protocol/packets/DebugInfoPacket.h"
#include "protocol/packets/DimensionDataPacket.h"
#include "protocol/packets/DisconnectPacket.h"
#include "protocol/packets/EditorNetworkPacket.h"
#include "protocol/packets/EducationSettingsPacket.h"
#include "protocol/packets/EduUriResourcePacket.h"
#include "protocol/packets/EmoteListPacket.h"
#include "protocol/packets/EmotePacket.h"
#include "protocol/packets/ActorEventPacket.h"
#include "protocol/packets/EntityFallPacket.h"
#include "protocol/packets/ActorPickRequestPacket.h"
#include "protocol/packets/LegacyTelemetryEventPacket.h"
#include "protocol/packets/FeatureRegistryPacket.h"
#include "protocol/packets/FilterTextPacket.h"
#include "protocol/packets/GameRulesChangedPacket.h"
#include "protocol/packets/GameTestRequestPacket.h"
#include "protocol/packets/GameTestResultsPacket.h"
#include "protocol/packets/GraphicsOverrideParameterPacket.h"
#include "protocol/packets/GuiDataPickItemPacket.h"
#include "protocol/packets/HurtArmorPacket.h"
#include "protocol/packets/InteractPacket.h"
#include "protocol/packets/InventoryContentPacket.h"
#include "protocol/packets/InventorySlotPacket.h"
#include "protocol/packets/InventoryTransactionPacket.h"
#include "protocol/packets/ItemRegistryPacket.h"
#include "protocol/packets/ItemStackRequestPacket.h"
#include "protocol/packets/ItemStackResponsePacket.h"
#include "protocol/packets/JigsawStructureDataPacket.h"
#include "protocol/packets/LabTablePacket.h"
#include "protocol/packets/LessonProgressPacket.h"
#include "protocol/packets/LevelChunkPacket.h"
#include "protocol/packets/LevelEventPacket.h"
#include "protocol/packets/LevelSoundEventPacket.h"
#include "protocol/packets/LocatorBarPacket.h"
#include "protocol/packets/LoginPacket.h"
#include "protocol/packets/MapCreateLockedCopyPacket.h"
#include "protocol/packets/MapInfoRequestPacket.h"
#include "protocol/packets/MobArmorEquipmentPacket.h"
#include "protocol/packets/MobEffectPacket.h"
#include "protocol/packets/MobEquipmentPacket.h"
#include "protocol/packets/ModalFormRequestPacket.h"
#include "protocol/packets/ModalFormResponsePacket.h"
#include "protocol/packets/MotionPredictionHintsPacket.h"
#include "protocol/packets/MoveActorAbsolutePacket.h"
#include "protocol/packets/MoveActorDeltaPacket.h"
#include "protocol/packets/MovePlayerPacket.h"
#include "protocol/packets/MovementEffectPacket.h"
#include "protocol/packets/ClientMovementPredictionSyncPacket.h"
#include "protocol/packets/MultiplayerSettingsPacket.h"
#include "protocol/packets/NetworkChunkPublisherUpdatePacket.h"
#include "protocol/packets/NetworkSettingsPacket.h"
#include "protocol/packets/NetworkStackLatencyPacket.h"
#include "protocol/packets/NpcDialoguePacket.h"
#include "protocol/packets/NpcRequestPacket.h"
#include "protocol/packets/OnScreenTextureAnimationPacket.h"
#include "protocol/packets/OpenSignPacket.h"
#include "protocol/packets/PacketViolationWarningPacket.h"
#include "protocol/packets/PartyChangedPacket.h"
#include "protocol/packets/PartyDestinationCookieResponsePacket.h"
#include "protocol/packets/PhotoInfoRequestPacket.h"
#include "protocol/packets/PhotoTransferPacket.h"
#include "protocol/packets/PlayStatusPacket.h"
#include "protocol/packets/PlaySoundPacket.h"
#include "protocol/packets/PlayerActionPacket.h"
#include "protocol/packets/PlayerListPacket.h"
#include "protocol/packets/PlayerArmorDamagePacket.h"
#include "protocol/packets/PlayerAuthInputPacket.h"
#include "protocol/packets/PlayerEnchantOptionsPacket.h"
#include "protocol/packets/PlayerFogPacket.h"
#include "protocol/packets/PlayerHotbarPacket.h"
#include "protocol/packets/PlayerLocationPacket.h"
#include "protocol/packets/PlayerSkinPacket.h"
#include "protocol/packets/PlayerStartItemCooldownPacket.h"
#include "protocol/packets/PlayerUpdateEntityOverridesPacket.h"
#include "protocol/packets/PlayerVideoCapturePacket.h"
#include "protocol/packets/PositionTrackingDBClientRequestPacket.h"
#include "protocol/packets/PositionTrackingDBServerBroadcastPacket.h"
#include "protocol/packets/PurchaseReceiptPacket.h"
#include "protocol/packets/RefreshEntitlementsPacket.h"
#include "protocol/packets/RemoveActorPacket.h"
#include "protocol/packets/RemoveObjectivePacket.h"
#include "protocol/packets/RemoveVolumeEntityPacket.h"
#include "protocol/packets/RequestAbilityPacket.h"
#include "protocol/packets/RequestChunkRadiusPacket.h"
#include "protocol/packets/RequestNetworkSettingsPacket.h"
#include "protocol/packets/RequestPermissionsPacket.h"
#include "protocol/packets/ResourcePackChunkDataPacket.h"
#include "protocol/packets/ResourcePackChunkRequestPacket.h"
#include "protocol/packets/ResourcePackClientResponsePacket.h"
#include "protocol/packets/ResourcePackDataInfoPacket.h"
#include "protocol/packets/ResourcePackStackPacket.h"
#include "protocol/packets/ResourcePacksInfoPacket.h"
#include "protocol/packets/ResourcePacksReadyForValidationPacket.h"
#include "protocol/packets/RespawnPacket.h"
#include "protocol/packets/ScriptCustomEventPacket.h"
#include "protocol/packets/ScriptMessagePacket.h"
#include "protocol/packets/SendPartyDestinationCookiePacket.h"
#include "protocol/packets/ServerboundDataDrivenScreenClosedPacket.h"
#include "protocol/packets/ServerboundDataStorePacket.h"
#include "protocol/packets/ServerboundDiagnosticsPacket.h"
#include "protocol/packets/ServerboundLoadingScreenPacket.h"
#include "protocol/packets/ServerboundPackSettingChangePacket.h"
#include "protocol/packets/ServerPlayerPostMovePositionPacket.h"
#include "protocol/packets/ServerPresenceInfoPacket.h"
#include "protocol/packets/ServerSettingsRequestPacket.h"
#include "protocol/packets/ServerSettingsResponsePacket.h"
#include "protocol/packets/ServerStatsPacket.h"
#include "protocol/packets/ServerStoreInfoPacket.h"
#include "protocol/packets/ServerToClientHandshakePacket.h"
#include "protocol/packets/SetCommandsEnabledPacket.h"
#include "protocol/packets/SetDefaultGameTypePacket.h"
#include "protocol/packets/SetDifficultyPacket.h"
#include "protocol/packets/SetDisplayObjectivePacket.h"
#include "protocol/packets/SetActorDataPacket.h"
#include "protocol/packets/SetActorLinkPacket.h"
#include "protocol/packets/SetActorMotionPacket.h"
#include "protocol/packets/SetHealthPacket.h"
#include "protocol/packets/SetHudPacket.h"
#include "protocol/packets/SetLastHurtByPacket.h"
#include "protocol/packets/SetLocalPlayerAsInitializedPacket.h"
#include "protocol/packets/SetMovementAuthorityPacket.h"
#include "protocol/packets/SetPlayerGameTypePacket.h"
#include "protocol/packets/SetPlayerInventoryOptionsPacket.h"
#include "protocol/packets/SetScorePacket.h"
#include "protocol/packets/SetScoreboardIdentityPacket.h"
#include "protocol/packets/SetSpawnPositionPacket.h"
#include "protocol/packets/SetTimePacket.h"
#include "protocol/packets/SetTitlePacket.h"
#include "protocol/packets/SettingsCommandPacket.h"
#include "protocol/packets/ShowCreditsPacket.h"
#include "protocol/packets/ShowProfilePacket.h"
#include "protocol/packets/ShowStoreOfferPacket.h"
#include "protocol/packets/SimpleEventPacket.h"
#include "protocol/packets/SimulationTypePacket.h"
#include "protocol/packets/SpawnExperienceOrbPacket.h"
#include "protocol/packets/SpawnParticleEffectPacket.h"
#include "protocol/packets/StartGamePacket.h"
#include "protocol/packets/StopSoundPacket.h"
#include "protocol/packets/StructureBlockUpdatePacket.h"
#include "protocol/packets/StructureTemplateDataRequestPacket.h"
#include "protocol/packets/StructureTemplateDataResponsePacket.h"
#include "protocol/packets/SubChunkPacket.h"
#include "protocol/packets/SubChunkRequestPacket.h"
#include "protocol/packets/SubClientLoginPacket.h"
#include "protocol/packets/SyncActorPropertyPacket.h"
#include "protocol/packets/SyncWorldClocksPacket.h"
#include "protocol/packets/TakeItemActorPacket.h"
#include "protocol/packets/TextPacket.h"
#include "protocol/packets/TickSyncPacket.h"
#include "protocol/packets/TickingAreasLoadStatusPacket.h"
#include "protocol/packets/ToastRequestPacket.h"
#include "protocol/packets/PlayerToggleCrafterSlotRequestPacket.h"
#include "protocol/packets/TransferPacket.h"
#include "protocol/packets/TrimDataPacket.h"
#include "protocol/packets/UpdateAbilitiesPacket.h"
#include "protocol/packets/UpdateAdventureSettingsPacket.h"
#include "protocol/packets/UpdateAttributesPacket.h"
#include "protocol/packets/UpdateBlockPacket.h"
#include "protocol/packets/UpdateBlockPropertiesPacket.h"
#include "protocol/packets/UpdateBlockSyncedPacket.h"
#include "protocol/packets/UpdateClientInputLocksPacket.h"
#include "protocol/packets/UpdateClientOptionsPacket.h"
#include "protocol/packets/UpdateEquipPacket.h"
#include "protocol/packets/UpdatePlayerGameTypePacket.h"
#include "protocol/packets/UpdateSoftEnumPacket.h"
#include "protocol/packets/UpdateSubChunkBlocksPacket.h"
#include "protocol/packets/UpdateTradePacket.h"
#include "protocol/packets/UnlockedRecipesPacket.h"
#include "protocol/packets/VideoStreamConnectPacket.h"
#include "protocol/packets/VoxelShapesPacket.h"

#include <mutex>
#include <unordered_map>

namespace {

    std::mutex &getFactoryMutex() {
        static std::mutex mutex;
        return mutex;
    }

    std::unordered_map<int, MinecraftPackets::PacketFactory> &getFactories() {
        static std::unordered_map<int, MinecraftPackets::PacketFactory> factories;
        return factories;
    }

    std::once_flag &getRegistrationFlag() {
        static std::once_flag flag;
        return flag;
    }

}

void MinecraftPackets::registerPacket(MinecraftPacketIds id, const PacketFactory &factory) {
    if (!factory)
        return;

    std::lock_guard<std::mutex> guard(getFactoryMutex());
    getFactories()[(int) id] = factory;
}

void MinecraftPackets::_registerDefaultPackets() {
    registerPacket<AddBehaviorTreePacket>();
    registerPacket<AddActorPacket>();
    registerPacket<AddItemActorPacket>();
    registerPacket<AddPaintingPacket>();
    registerPacket<AddPlayerPacket>();
    registerPacket<AddVolumeEntityPacket>();
    registerPacket<AdventureSettingsPacket>();
    registerPacket<AgentActionEventPacket>();
    registerPacket<AgentAnimationPacket>();
    registerPacket<AnimateEntityPacket>();
    registerPacket<AnimatePacket>();
    registerPacket<AnvilDamagePacket>();
    registerPacket<AutomationClientConnectPacket>();
    registerPacket<AwardAchievementPacket>();
    registerPacket<AvailableCommandsPacket>();
    registerPacket<AvailableActorIdentifiersPacket>();
    registerPacket<BiomeDefinitionListPacket>();
    registerPacket<BlockActorDataPacket>();
    registerPacket<BlockEventPacket>();
    registerPacket<BlockPickRequestPacket>();
    registerPacket<BookEditPacket>();
    registerPacket<BossEventPacket>();
    registerPacket<CameraPacket>();
    registerPacket<CameraAimAssistPacket>();
    registerPacket<CameraAimAssistActorPriorityPacket>();
    registerPacket<ClientCameraAimAssistPacket>();
    registerPacket<CameraAimAssistPresetsPacket>();
    registerPacket<CameraInstructionPacket>();
    registerPacket<CameraPresetsPacket>();
    registerPacket<CameraShakePacket>();
    registerPacket<CameraSplinePacket>();
    registerPacket<ChangeDimensionPacket>();
    registerPacket<ChangeMobPropertyPacket>();
    registerPacket<ChunkRadiusUpdatedPacket>();
    registerPacket<ClientCacheBlobStatusPacket>();
    registerPacket<ClientCacheMissResponsePacket>();
    registerPacket<ClientCacheStatusPacket>();
    registerPacket<ClientCheatAbilityPacket>();
    registerPacket<ClientToServerHandshakePacket>();
    registerPacket<ClientboundAttributeLayerSyncPacket>();
    registerPacket<ClientboundCloseFormPacket>();
    registerPacket<ClientboundControlSchemeSetPacket>();
    registerPacket<ClientboundDataDrivenUICloseScreenPacket>();
    registerPacket<ClientboundDataDrivenUIReloadPacket>();
    registerPacket<ClientboundDataDrivenUIShowScreenPacket>();
    registerPacket<ClientboundDataStorePacket>();
    registerPacket<ClientboundDebugRendererPacket>();
    registerPacket<ClientboundMapItemDataPacket>();
    registerPacket<ClientboundTextureShiftPacket>();
    registerPacket<ClientboundUpdateSoundDataPacket>();
    registerPacket<CodeBuilderPacket>();
    registerPacket<CommandBlockUpdatePacket>();
    registerPacket<CommandOutputPacket>();
    registerPacket<CommandRequestPacket>();
    registerPacket<CodeBuilderSourcePacket>();
    registerPacket<CompletedUsingItemPacket>();
    registerPacket<ContainerClosePacket>();
    registerPacket<ContainerOpenPacket>();
    registerPacket<ContainerRegistryCleanupPacket>();
    registerPacket<ContainerSetDataPacket>();
    registerPacket<CorrectPlayerMovePredictionPacket>();
    registerPacket<CraftingDataPacket>();
    registerPacket<CraftingEventPacket>();
    registerPacket<CreatePhotoPacket>();
    registerPacket<CreativeContentPacket>();
    registerPacket<CurrentStructureFeaturePacket>();
    registerPacket<DeathInfoPacket>();
    registerPacket<PrimitiveShapesPacket>();
    registerPacket<DebugInfoPacket>();
    registerPacket<DimensionDataPacket>();
    registerPacket<DisconnectPacket>();
    registerPacket<EditorNetworkPacket>();
    registerPacket<EducationSettingsPacket>();
    registerPacket<EduUriResourcePacket>();
    registerPacket<EmoteListPacket>();
    registerPacket<EmotePacket>();
    registerPacket<ActorEventPacket>();
    registerPacket<EntityFallPacket>();
    registerPacket<ActorPickRequestPacket>();
    registerPacket<LegacyTelemetryEventPacket>();
    registerPacket<FeatureRegistryPacket>();
    registerPacket<FilterTextPacket>();
    registerPacket<GameRulesChangedPacket>();
    registerPacket<GameTestRequestPacket>();
    registerPacket<GameTestResultsPacket>();
    registerPacket<GraphicsOverrideParameterPacket>();
    registerPacket<GuiDataPickItemPacket>();
    registerPacket<HurtArmorPacket>();
    registerPacket<InteractPacket>();
    registerPacket<InventoryContentPacket>();
    registerPacket<InventorySlotPacket>();
    registerPacket<InventoryTransactionPacket>();
    registerPacket<ItemRegistryPacket>();
    registerPacket<ItemStackRequestPacket>();
    registerPacket<ItemStackResponsePacket>();
    registerPacket<JigsawStructureDataPacket>();
    registerPacket<LabTablePacket>();
    registerPacket<LessonProgressPacket>();
    registerPacket<LevelChunkPacket>();
    registerPacket<LevelEventPacket>();
    registerPacket<LevelSoundEventPacket>();
    registerPacket<LocatorBarPacket>();
    registerPacket<LoginPacket>();
    registerPacket<MapCreateLockedCopyPacket>();
    registerPacket<MapInfoRequestPacket>();
    registerPacket<MobArmorEquipmentPacket>();
    registerPacket<MobEffectPacket>();
    registerPacket<MobEquipmentPacket>();
    registerPacket<ModalFormRequestPacket>();
    registerPacket<ModalFormResponsePacket>();
    registerPacket<MotionPredictionHintsPacket>();
    registerPacket<MoveActorAbsolutePacket>();
    registerPacket<MoveActorDeltaPacket>();
    registerPacket<MovePlayerPacket>();
    registerPacket<MovementEffectPacket>();
    registerPacket<ClientMovementPredictionSyncPacket>();
    registerPacket<MultiplayerSettingsPacket>();
    registerPacket<NetworkChunkPublisherUpdatePacket>();
    registerPacket<NetworkSettingsPacket>();
    registerPacket<NetworkStackLatencyPacket>();
    registerPacket<NpcDialoguePacket>();
    registerPacket<NpcRequestPacket>();
    registerPacket<OnScreenTextureAnimationPacket>();
    registerPacket<OpenSignPacket>();
    registerPacket<PacketViolationWarningPacket>();
    registerPacket<PartyChangedPacket>();
    registerPacket<PartyDestinationCookieResponsePacket>();
    registerPacket<PhotoInfoRequestPacket>();
    registerPacket<PhotoTransferPacket>();
    registerPacket<PlayStatusPacket>();
    registerPacket<PlaySoundPacket>();
    registerPacket<PlayerActionPacket>();
    registerPacket<PlayerListPacket>();
    registerPacket<PlayerArmorDamagePacket>();
    registerPacket<PlayerAuthInputPacket>();
    registerPacket<PlayerEnchantOptionsPacket>();
    registerPacket<PlayerFogPacket>();
    registerPacket<PlayerHotbarPacket>();
    registerPacket<PlayerLocationPacket>();
    registerPacket<PlayerSkinPacket>();
    registerPacket<PlayerStartItemCooldownPacket>();
    registerPacket<PlayerUpdateEntityOverridesPacket>();
    registerPacket<PlayerVideoCapturePacket>();
    registerPacket<PositionTrackingDBClientRequestPacket>();
    registerPacket<PositionTrackingDBServerBroadcastPacket>();
    registerPacket<PurchaseReceiptPacket>();
    registerPacket<RefreshEntitlementsPacket>();
    registerPacket<RemoveActorPacket>();
    registerPacket<RemoveObjectivePacket>();
    registerPacket<RemoveVolumeEntityPacket>();
    registerPacket<RequestAbilityPacket>();
    registerPacket<RequestChunkRadiusPacket>();
    registerPacket<RequestNetworkSettingsPacket>();
    registerPacket<RequestPermissionsPacket>();
    registerPacket<ResourcePackChunkDataPacket>();
    registerPacket<ResourcePackChunkRequestPacket>();
    registerPacket<ResourcePackClientResponsePacket>();
    registerPacket<ResourcePackDataInfoPacket>();
    registerPacket<ResourcePackStackPacket>();
    registerPacket<ResourcePacksInfoPacket>();
    registerPacket<ResourcePacksReadyForValidationPacket>();
    registerPacket<RespawnPacket>();
    registerPacket<ScriptCustomEventPacket>();
    registerPacket<ScriptMessagePacket>();
    registerPacket<SendPartyDestinationCookiePacket>();
    registerPacket<ServerboundDataDrivenScreenClosedPacket>();
    registerPacket<ServerboundDataStorePacket>();
    registerPacket<ServerboundDiagnosticsPacket>();
    registerPacket<ServerboundLoadingScreenPacket>();
    registerPacket<ServerboundPackSettingChangePacket>();
    registerPacket<ServerPlayerPostMovePositionPacket>();
    registerPacket<ServerPresenceInfoPacket>();
    registerPacket<ServerSettingsRequestPacket>();
    registerPacket<ServerSettingsResponsePacket>();
    registerPacket<ServerStatsPacket>();
    registerPacket<ServerStoreInfoPacket>();
    registerPacket<ServerToClientHandshakePacket>();
    registerPacket<SetCommandsEnabledPacket>();
    registerPacket<SetDefaultGameTypePacket>();
    registerPacket<SetDifficultyPacket>();
    registerPacket<SetDisplayObjectivePacket>();
    registerPacket<SetActorDataPacket>();
    registerPacket<SetActorLinkPacket>();
    registerPacket<SetActorMotionPacket>();
    registerPacket<SetHealthPacket>();
    registerPacket<SetHudPacket>();
    registerPacket<SetLastHurtByPacket>();
    registerPacket<SetLocalPlayerAsInitializedPacket>();
    registerPacket<SetMovementAuthorityPacket>();
    registerPacket<SetPlayerGameTypePacket>();
    registerPacket<SetPlayerInventoryOptionsPacket>();
    registerPacket<SetScorePacket>();
    registerPacket<SetScoreboardIdentityPacket>();
    registerPacket<SetSpawnPositionPacket>();
    registerPacket<SetTimePacket>();
    registerPacket<SetTitlePacket>();
    registerPacket<SettingsCommandPacket>();
    registerPacket<ShowCreditsPacket>();
    registerPacket<ShowProfilePacket>();
    registerPacket<ShowStoreOfferPacket>();
    registerPacket<SimpleEventPacket>();
    registerPacket<SimulationTypePacket>();
    registerPacket<SpawnExperienceOrbPacket>();
    registerPacket<SpawnParticleEffectPacket>();
    registerPacket<StartGamePacket>();
    registerPacket<StopSoundPacket>();
    registerPacket<StructureBlockUpdatePacket>();
    registerPacket<StructureTemplateDataRequestPacket>();
    registerPacket<StructureTemplateDataResponsePacket>();
    registerPacket<SubChunkPacket>();
    registerPacket<SubChunkRequestPacket>();
    registerPacket<SubClientLoginPacket>();
    registerPacket<SyncActorPropertyPacket>();
    registerPacket<SyncWorldClocksPacket>();
    registerPacket<TakeItemActorPacket>();
    registerPacket<TextPacket>();
    registerPacket<TickSyncPacket>();
    registerPacket<TickingAreasLoadStatusPacket>();
    registerPacket<ToastRequestPacket>();
    registerPacket<PlayerToggleCrafterSlotRequestPacket>();
    registerPacket<TransferPacket>();
    registerPacket<TrimDataPacket>();
    registerPacket<UpdateAbilitiesPacket>();
    registerPacket<UpdateAdventureSettingsPacket>();
    registerPacket<UpdateAttributesPacket>();
    registerPacket<UpdateBlockPacket>();
    registerPacket<UpdateBlockPropertiesPacket>();
    registerPacket<UpdateBlockSyncedPacket>();
    registerPacket<UpdateClientInputLocksPacket>();
    registerPacket<UpdateClientOptionsPacket>();
    registerPacket<UpdateEquipPacket>();
    registerPacket<UpdatePlayerGameTypePacket>();
    registerPacket<UpdateSoftEnumPacket>();
    registerPacket<UpdateSubChunkBlocksPacket>();
    registerPacket<UpdateTradePacket>();
    registerPacket<UnlockedRecipesPacket>();
    registerPacket<VideoStreamConnectPacket>();
    registerPacket<VoxelShapesPacket>();
}

std::shared_ptr<Packet> MinecraftPackets::createPacket(MinecraftPacketIds id) {
    std::call_once(getRegistrationFlag(), &MinecraftPackets::_registerDefaultPackets);

    std::lock_guard<std::mutex> guard(getFactoryMutex());

    auto it = getFactories().find((int) id);
    if (it == getFactories().end())
        return nullptr;

    return it->second();
}

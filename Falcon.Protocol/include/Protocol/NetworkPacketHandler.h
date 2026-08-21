#pragma once

class NetworkIdentifier;

class AddBehaviorTreePacket;
class AnimatePacket;
class AnvilDamagePacket;
class AwardAchievementPacket;
class BlockEntityDataPacket;
class BlockPickRequestPacket;
class BossEventPacket;
class CameraShakePacket;
class ChangeDimensionPacket;
class ChunkRadiusUpdatedPacket;
class ClientCacheBlobStatusPacket;
class ClientCacheMissResponsePacket;
class ClientCacheStatusPacket;
class ClientToServerHandshakePacket;
class ClientboundCloseFormPacket;
class CodeBuilderPacket;
class DebugInfoPacket;
class DisconnectPacket;
class EmoteListPacket;
class EntityPickRequestPacket;
class HurtArmorPacket;
class InteractPacket;
class LevelChunkPacket;
class LoginPacket;
class MapCreateLockedCopyPacket;
class MobEffectPacket;
class ModalFormRequestPacket;
class ModalFormResponsePacket;
class MultiplayerSettingsPacket;
class NetworkChunkPublisherUpdatePacket;
class NetworkSettingsPacket;
class NetworkStackLatencyPacket;
class NpcRequestPacket;
class OnScreenTextureAnimationPacket;
class OpenSignPacket;
class PlayStatusPacket;
class PlayerFogPacket;
class RefreshEntitlementsPacket;
class RemoveEntityPacket;
class RemoveObjectivePacket;
class RequestChunkRadiusPacket;
class RequestNetworkSettingsPacket;
class ResourcePackClientResponsePacket;
class ResourcePackStackPacket;
class ResourcePacksInfoPacket;
class RespawnPacket;
class ScriptMessagePacket;
class ServerPlayerPostMovePositionPacket;
class ServerSettingsRequestPacket;
class ServerSettingsResponsePacket;
class ServerStatsPacket;
class ServerToClientHandshakePacket;
class SetCommandsEnabledPacket;
class SetDefaultGameTypePacket;
class SetDifficultyPacket;
class SetEntityMotionPacket;
class SetHealthPacket;
class SetLastHurtByPacket;
class SetLocalPlayerAsInitializedPacket;
class SetPlayerGameTypePacket;
class SetSpawnPositionPacket;
class SetTimePacket;
class SetTitlePacket;
class SettingsCommandPacket;
class ShowCreditsPacket;
class ShowProfilePacket;
class SimpleEventPacket;
class SimulationTypePacket;
class SpawnExperienceOrbPacket;
class SpawnParticleEffectPacket;
class StartGamePacket;
class StopSoundPacket;
class StructureTemplateDataRequestPacket;
class StructureTemplateDataResponsePacket;
class SubChunkPacket;
class SubChunkRequestPacket;
class TakeItemEntityPacket;
class TextPacket;
class TickingAreasLoadStatusPacket;
class ToastRequestPacket;
class UpdateBlockPacket;

class NetworkPacketHandler {
public:
    virtual ~NetworkPacketHandler() = default;

    virtual void handle(const NetworkIdentifier &, const AddBehaviorTreePacket &) {}

    virtual void handle(const NetworkIdentifier &, const AnimatePacket &) {}

    virtual void handle(const NetworkIdentifier &, const AnvilDamagePacket &) {}

    virtual void handle(const NetworkIdentifier &, const AwardAchievementPacket &) {}

    virtual void handle(const NetworkIdentifier &, const BlockEntityDataPacket &) {}

    virtual void handle(const NetworkIdentifier &, const BlockPickRequestPacket &) {}

    virtual void handle(const NetworkIdentifier &, const BossEventPacket &) {}

    virtual void handle(const NetworkIdentifier &, const CameraShakePacket &) {}

    virtual void handle(const NetworkIdentifier &, const ChangeDimensionPacket &) {}

    virtual void handle(const NetworkIdentifier &, const ChunkRadiusUpdatedPacket &) {}

    virtual void handle(const NetworkIdentifier &, const ClientCacheBlobStatusPacket &) {}

    virtual void handle(const NetworkIdentifier &, const ClientCacheMissResponsePacket &) {}

    virtual void handle(const NetworkIdentifier &, const ClientCacheStatusPacket &) {}

    virtual void handle(const NetworkIdentifier &, const ClientToServerHandshakePacket &) {}

    virtual void handle(const NetworkIdentifier &, const ClientboundCloseFormPacket &) {}

    virtual void handle(const NetworkIdentifier &, const CodeBuilderPacket &) {}

    virtual void handle(const NetworkIdentifier &, const DebugInfoPacket &) {}

    virtual void handle(const NetworkIdentifier &, const DisconnectPacket &) {}

    virtual void handle(const NetworkIdentifier &, const EmoteListPacket &) {}

    virtual void handle(const NetworkIdentifier &, const EntityPickRequestPacket &) {}

    virtual void handle(const NetworkIdentifier &, const HurtArmorPacket &) {}

    virtual void handle(const NetworkIdentifier &, const InteractPacket &) {}

    virtual void handle(const NetworkIdentifier &, const LevelChunkPacket &) {}

    virtual void handle(const NetworkIdentifier &, const LoginPacket &) {}

    virtual void handle(const NetworkIdentifier &, const MapCreateLockedCopyPacket &) {}

    virtual void handle(const NetworkIdentifier &, const MobEffectPacket &) {}

    virtual void handle(const NetworkIdentifier &, const ModalFormRequestPacket &) {}

    virtual void handle(const NetworkIdentifier &, const ModalFormResponsePacket &) {}

    virtual void handle(const NetworkIdentifier &, const MultiplayerSettingsPacket &) {}

    virtual void handle(const NetworkIdentifier &, const NetworkChunkPublisherUpdatePacket &) {}

    virtual void handle(const NetworkIdentifier &, const NetworkSettingsPacket &) {}

    virtual void handle(const NetworkIdentifier &, const NetworkStackLatencyPacket &) {}

    virtual void handle(const NetworkIdentifier &, const NpcRequestPacket &) {}

    virtual void handle(const NetworkIdentifier &, const OnScreenTextureAnimationPacket &) {}

    virtual void handle(const NetworkIdentifier &, const OpenSignPacket &) {}

    virtual void handle(const NetworkIdentifier &, const PlayStatusPacket &) {}

    virtual void handle(const NetworkIdentifier &, const PlayerFogPacket &) {}

    virtual void handle(const NetworkIdentifier &, const RefreshEntitlementsPacket &) {}

    virtual void handle(const NetworkIdentifier &, const RemoveEntityPacket &) {}

    virtual void handle(const NetworkIdentifier &, const RemoveObjectivePacket &) {}

    virtual void handle(const NetworkIdentifier &, const RequestChunkRadiusPacket &) {}

    virtual void handle(const NetworkIdentifier &, const RequestNetworkSettingsPacket &) {}

    virtual void handle(const NetworkIdentifier &, const ResourcePackClientResponsePacket &) {}

    virtual void handle(const NetworkIdentifier &, const ResourcePackStackPacket &) {}

    virtual void handle(const NetworkIdentifier &, const ResourcePacksInfoPacket &) {}

    virtual void handle(const NetworkIdentifier &, const RespawnPacket &) {}

    virtual void handle(const NetworkIdentifier &, const ScriptMessagePacket &) {}

    virtual void handle(const NetworkIdentifier &, const ServerPlayerPostMovePositionPacket &) {}

    virtual void handle(const NetworkIdentifier &, const ServerSettingsRequestPacket &) {}

    virtual void handle(const NetworkIdentifier &, const ServerSettingsResponsePacket &) {}

    virtual void handle(const NetworkIdentifier &, const ServerStatsPacket &) {}

    virtual void handle(const NetworkIdentifier &, const ServerToClientHandshakePacket &) {}

    virtual void handle(const NetworkIdentifier &, const SetCommandsEnabledPacket &) {}

    virtual void handle(const NetworkIdentifier &, const SetDefaultGameTypePacket &) {}

    virtual void handle(const NetworkIdentifier &, const SetDifficultyPacket &) {}

    virtual void handle(const NetworkIdentifier &, const SetEntityMotionPacket &) {}

    virtual void handle(const NetworkIdentifier &, const SetHealthPacket &) {}

    virtual void handle(const NetworkIdentifier &, const SetLastHurtByPacket &) {}

    virtual void handle(const NetworkIdentifier &, const SetLocalPlayerAsInitializedPacket &) {}

    virtual void handle(const NetworkIdentifier &, const SetPlayerGameTypePacket &) {}

    virtual void handle(const NetworkIdentifier &, const SetSpawnPositionPacket &) {}

    virtual void handle(const NetworkIdentifier &, const SetTimePacket &) {}

    virtual void handle(const NetworkIdentifier &, const SetTitlePacket &) {}

    virtual void handle(const NetworkIdentifier &, const SettingsCommandPacket &) {}

    virtual void handle(const NetworkIdentifier &, const ShowCreditsPacket &) {}

    virtual void handle(const NetworkIdentifier &, const ShowProfilePacket &) {}

    virtual void handle(const NetworkIdentifier &, const SimpleEventPacket &) {}

    virtual void handle(const NetworkIdentifier &, const SimulationTypePacket &) {}

    virtual void handle(const NetworkIdentifier &, const SpawnExperienceOrbPacket &) {}

    virtual void handle(const NetworkIdentifier &, const SpawnParticleEffectPacket &) {}

    virtual void handle(const NetworkIdentifier &, const StartGamePacket &) {}

    virtual void handle(const NetworkIdentifier &, const StopSoundPacket &) {}

    virtual void handle(const NetworkIdentifier &, const StructureTemplateDataRequestPacket &) {}

    virtual void handle(const NetworkIdentifier &, const StructureTemplateDataResponsePacket &) {}

    virtual void handle(const NetworkIdentifier &, const SubChunkPacket &) {}

    virtual void handle(const NetworkIdentifier &, const SubChunkRequestPacket &) {}

    virtual void handle(const NetworkIdentifier &, const TakeItemEntityPacket &) {}

    virtual void handle(const NetworkIdentifier &, const TextPacket &) {}

    virtual void handle(const NetworkIdentifier &, const TickingAreasLoadStatusPacket &) {}

    virtual void handle(const NetworkIdentifier &, const ToastRequestPacket &) {}

    virtual void handle(const NetworkIdentifier &, const UpdateBlockPacket &) {}

};

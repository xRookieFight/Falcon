#include "Protocol/MinecraftPackets.h"

#include "Protocol/Packets/AddBehaviorTreePacket.h"
#include "Protocol/Packets/AnimatePacket.h"
#include "Protocol/Packets/AnvilDamagePacket.h"
#include "Protocol/Packets/AwardAchievementPacket.h"
#include "Protocol/Packets/BlockEntityDataPacket.h"
#include "Protocol/Packets/BlockPickRequestPacket.h"
#include "Protocol/Packets/BossEventPacket.h"
#include "Protocol/Packets/CameraShakePacket.h"
#include "Protocol/Packets/ChangeDimensionPacket.h"
#include "Protocol/Packets/ChunkRadiusUpdatedPacket.h"
#include "Protocol/Packets/ClientCacheBlobStatusPacket.h"
#include "Protocol/Packets/ClientCacheMissResponsePacket.h"
#include "Protocol/Packets/ClientCacheStatusPacket.h"
#include "Protocol/Packets/ClientToServerHandshakePacket.h"
#include "Protocol/Packets/ClientboundCloseFormPacket.h"
#include "Protocol/Packets/CodeBuilderPacket.h"
#include "Protocol/Packets/DebugInfoPacket.h"
#include "Protocol/Packets/DisconnectPacket.h"
#include "Protocol/Packets/EmoteListPacket.h"
#include "Protocol/Packets/EntityPickRequestPacket.h"
#include "Protocol/Packets/HurtArmorPacket.h"
#include "Protocol/Packets/InteractPacket.h"
#include "Protocol/Packets/LevelChunkPacket.h"
#include "Protocol/Packets/LoginPacket.h"
#include "Protocol/Packets/MapCreateLockedCopyPacket.h"
#include "Protocol/Packets/MobEffectPacket.h"
#include "Protocol/Packets/ModalFormRequestPacket.h"
#include "Protocol/Packets/ModalFormResponsePacket.h"
#include "Protocol/Packets/MultiplayerSettingsPacket.h"
#include "Protocol/Packets/NetworkChunkPublisherUpdatePacket.h"
#include "Protocol/Packets/NetworkSettingsPacket.h"
#include "Protocol/Packets/NetworkStackLatencyPacket.h"
#include "Protocol/Packets/NpcRequestPacket.h"
#include "Protocol/Packets/OnScreenTextureAnimationPacket.h"
#include "Protocol/Packets/OpenSignPacket.h"
#include "Protocol/Packets/PlayStatusPacket.h"
#include "Protocol/Packets/PlayerFogPacket.h"
#include "Protocol/Packets/RefreshEntitlementsPacket.h"
#include "Protocol/Packets/RemoveEntityPacket.h"
#include "Protocol/Packets/RemoveObjectivePacket.h"
#include "Protocol/Packets/RequestChunkRadiusPacket.h"
#include "Protocol/Packets/RequestNetworkSettingsPacket.h"
#include "Protocol/Packets/ResourcePackClientResponsePacket.h"
#include "Protocol/Packets/ResourcePackStackPacket.h"
#include "Protocol/Packets/ResourcePacksInfoPacket.h"
#include "Protocol/Packets/RespawnPacket.h"
#include "Protocol/Packets/ScriptMessagePacket.h"
#include "Protocol/Packets/ServerPlayerPostMovePositionPacket.h"
#include "Protocol/Packets/ServerSettingsRequestPacket.h"
#include "Protocol/Packets/ServerSettingsResponsePacket.h"
#include "Protocol/Packets/ServerStatsPacket.h"
#include "Protocol/Packets/ServerToClientHandshakePacket.h"
#include "Protocol/Packets/SetCommandsEnabledPacket.h"
#include "Protocol/Packets/SetDefaultGameTypePacket.h"
#include "Protocol/Packets/SetDifficultyPacket.h"
#include "Protocol/Packets/SetEntityMotionPacket.h"
#include "Protocol/Packets/SetHealthPacket.h"
#include "Protocol/Packets/SetLastHurtByPacket.h"
#include "Protocol/Packets/SetLocalPlayerAsInitializedPacket.h"
#include "Protocol/Packets/SetPlayerGameTypePacket.h"
#include "Protocol/Packets/SetSpawnPositionPacket.h"
#include "Protocol/Packets/SetTimePacket.h"
#include "Protocol/Packets/SetTitlePacket.h"
#include "Protocol/Packets/SettingsCommandPacket.h"
#include "Protocol/Packets/ShowCreditsPacket.h"
#include "Protocol/Packets/ShowProfilePacket.h"
#include "Protocol/Packets/SimpleEventPacket.h"
#include "Protocol/Packets/SimulationTypePacket.h"
#include "Protocol/Packets/SpawnExperienceOrbPacket.h"
#include "Protocol/Packets/SpawnParticleEffectPacket.h"
#include "Protocol/Packets/StartGamePacket.h"
#include "Protocol/Packets/StopSoundPacket.h"
#include "Protocol/Packets/StructureTemplateDataRequestPacket.h"
#include "Protocol/Packets/StructureTemplateDataResponsePacket.h"
#include "Protocol/Packets/SubChunkPacket.h"
#include "Protocol/Packets/SubChunkRequestPacket.h"
#include "Protocol/Packets/TakeItemEntityPacket.h"
#include "Protocol/Packets/TextPacket.h"
#include "Protocol/Packets/TickingAreasLoadStatusPacket.h"
#include "Protocol/Packets/ToastRequestPacket.h"
#include "Protocol/Packets/UpdateBlockPacket.h"

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
    registerPacket<AnimatePacket>();
    registerPacket<AnvilDamagePacket>();
    registerPacket<AwardAchievementPacket>();
    registerPacket<BlockEntityDataPacket>();
    registerPacket<BlockPickRequestPacket>();
    registerPacket<BossEventPacket>();
    registerPacket<CameraShakePacket>();
    registerPacket<ChangeDimensionPacket>();
    registerPacket<ChunkRadiusUpdatedPacket>();
    registerPacket<ClientCacheBlobStatusPacket>();
    registerPacket<ClientCacheMissResponsePacket>();
    registerPacket<ClientCacheStatusPacket>();
    registerPacket<ClientToServerHandshakePacket>();
    registerPacket<ClientboundCloseFormPacket>();
    registerPacket<CodeBuilderPacket>();
    registerPacket<DebugInfoPacket>();
    registerPacket<DisconnectPacket>();
    registerPacket<EmoteListPacket>();
    registerPacket<EntityPickRequestPacket>();
    registerPacket<HurtArmorPacket>();
    registerPacket<InteractPacket>();
    registerPacket<LevelChunkPacket>();
    registerPacket<LoginPacket>();
    registerPacket<MapCreateLockedCopyPacket>();
    registerPacket<MobEffectPacket>();
    registerPacket<ModalFormRequestPacket>();
    registerPacket<ModalFormResponsePacket>();
    registerPacket<MultiplayerSettingsPacket>();
    registerPacket<NetworkChunkPublisherUpdatePacket>();
    registerPacket<NetworkSettingsPacket>();
    registerPacket<NetworkStackLatencyPacket>();
    registerPacket<NpcRequestPacket>();
    registerPacket<OnScreenTextureAnimationPacket>();
    registerPacket<OpenSignPacket>();
    registerPacket<PlayStatusPacket>();
    registerPacket<PlayerFogPacket>();
    registerPacket<RefreshEntitlementsPacket>();
    registerPacket<RemoveEntityPacket>();
    registerPacket<RemoveObjectivePacket>();
    registerPacket<RequestChunkRadiusPacket>();
    registerPacket<RequestNetworkSettingsPacket>();
    registerPacket<ResourcePackClientResponsePacket>();
    registerPacket<ResourcePackStackPacket>();
    registerPacket<ResourcePacksInfoPacket>();
    registerPacket<RespawnPacket>();
    registerPacket<ScriptMessagePacket>();
    registerPacket<ServerPlayerPostMovePositionPacket>();
    registerPacket<ServerSettingsRequestPacket>();
    registerPacket<ServerSettingsResponsePacket>();
    registerPacket<ServerStatsPacket>();
    registerPacket<ServerToClientHandshakePacket>();
    registerPacket<SetCommandsEnabledPacket>();
    registerPacket<SetDefaultGameTypePacket>();
    registerPacket<SetDifficultyPacket>();
    registerPacket<SetEntityMotionPacket>();
    registerPacket<SetHealthPacket>();
    registerPacket<SetLastHurtByPacket>();
    registerPacket<SetLocalPlayerAsInitializedPacket>();
    registerPacket<SetPlayerGameTypePacket>();
    registerPacket<SetSpawnPositionPacket>();
    registerPacket<SetTimePacket>();
    registerPacket<SetTitlePacket>();
    registerPacket<SettingsCommandPacket>();
    registerPacket<ShowCreditsPacket>();
    registerPacket<ShowProfilePacket>();
    registerPacket<SimpleEventPacket>();
    registerPacket<SimulationTypePacket>();
    registerPacket<SpawnExperienceOrbPacket>();
    registerPacket<SpawnParticleEffectPacket>();
    registerPacket<StartGamePacket>();
    registerPacket<StopSoundPacket>();
    registerPacket<StructureTemplateDataRequestPacket>();
    registerPacket<StructureTemplateDataResponsePacket>();
    registerPacket<SubChunkPacket>();
    registerPacket<SubChunkRequestPacket>();
    registerPacket<TakeItemEntityPacket>();
    registerPacket<TextPacket>();
    registerPacket<TickingAreasLoadStatusPacket>();
    registerPacket<ToastRequestPacket>();
    registerPacket<UpdateBlockPacket>();
}

std::shared_ptr<Packet> MinecraftPackets::createPacket(MinecraftPacketIds id) {
    std::call_once(getRegistrationFlag(), &MinecraftPackets::_registerDefaultPackets);

    std::lock_guard<std::mutex> guard(getFactoryMutex());

    auto it = getFactories().find((int) id);
    if (it == getFactories().end())
        return nullptr;

    return it->second();
}

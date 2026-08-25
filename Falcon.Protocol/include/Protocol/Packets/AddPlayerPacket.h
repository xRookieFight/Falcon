#pragma once

#include "core/math/Vector3f.h"
#include "core/utility/UUID.h"
#include "protocol/Packet.h"
#include "protocol/types/EntityDataMap.h"
#include "protocol/types/EntityLinkData.h"
#include "protocol/types/EntityProperties.h"
#include "protocol/types/ItemStack.h"
#include "protocol/types/PlayerAbilityData.h"

#include <cstdint>
#include <string>
#include <vector>

class AddPlayerPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::AddPlayer;

    AddPlayerPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "AddPlayerPacket"; }

    void write(BinaryStream &stream, const PacketCodecContext &context) const override;

    void read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    Uuid mUuid;
    std::string mUsername;
    int64_t mRuntimeActorId = 0;
    std::string mPlatformChatId;
    Vector3f mPosition;
    Vector3f mMotion;
    Vector3f mRotation;
    ItemStack mHand;
    int32_t mGameType = 0;
    EntityDataMap mMetadata;
    EntityProperties mProperties;
    PlayerAbilityData mAbilities;
    std::vector<EntityLinkData> mActorLinks;
    std::string mDeviceId;
    int32_t mBuildPlatform = 0;
};

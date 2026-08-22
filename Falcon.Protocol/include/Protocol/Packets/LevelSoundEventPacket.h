#pragma once

#include "Core/Math/Vector3f.h"
#include "Protocol/Packet.h"

#include <string>

class LevelSoundEventPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::LevelSoundEvent;

    LevelSoundEventPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "LevelSoundEventPacket"; }

    void write(BinaryStream &stream, const PacketCodecContext &context) const override;

    void read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    std::string mSound;
    Vector3f mPosition;
    int32_t mExtraData;
    std::string mEntityType;
    bool mIsBabyMob;
    bool mDisableRelativeVolume;
    int64_t mActorUniqueId;
    bool mHasFirePosition;
    Vector3f mFirePosition;
};

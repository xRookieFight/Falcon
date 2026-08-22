#pragma once

#include "Core/Math/Vector3f.h"
#include "Protocol/Packet.h"

#include <cstdint>

enum class EntityEventType : int {
    Jump = 1,
    HurtAnimation = 2,
    DeathAnimation = 3,
    ArmSwing = 4,
    StopAttack = 5,
    Respawn = 18,
};

class EntityEventPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::EntityEvent;

    EntityEventPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "EntityEventPacket"; }

    void write(BinaryStream &stream, const PacketCodecContext &context) const override;

    void read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    uint64_t mRuntimeEntityId = 0;
    uint8_t mEventId = 0;
    int32_t mEventData = 0;
    bool mHasFirePosition = false;
    Vector3f mFirePosition;
};

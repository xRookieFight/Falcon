#pragma once

#include "protocol/Packet.h"
#include "protocol/types/EntityDataMap.h"
#include "protocol/types/EntityProperties.h"

#include <cstdint>

class SetActorDataPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::SetEntityData;

    SetActorDataPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "SetActorDataPacket"; }

    void write(BinaryStream &stream, const PacketCodecContext &context) const override;

    void read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    int64_t mRuntimeActorId = 0;
    EntityDataMap mMetadata;
    EntityProperties mProperties;
    int64_t mTick = 0;
};

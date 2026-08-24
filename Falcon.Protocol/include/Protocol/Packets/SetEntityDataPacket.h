#pragma once

#include "Protocol/Packet.h"
#include "Protocol/Types/EntityDataMap.h"
#include "Protocol/Types/EntityProperties.h"

#include <cstdint>

class SetEntityDataPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::SetEntityData;

    SetEntityDataPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "SetEntityDataPacket"; }

    void write(BinaryStream &stream, const PacketCodecContext &context) const override;

    void read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    int64_t mRuntimeActorId = 0;
    EntityDataMap mMetadata;
    EntityProperties mProperties;
    int64_t mTick = 0;
};

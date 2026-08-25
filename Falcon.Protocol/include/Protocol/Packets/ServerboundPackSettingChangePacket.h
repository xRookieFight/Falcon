#pragma once

#include "core/utility/UUID.h"
#include "protocol/Packet.h"
#include "protocol/types/DataStoreTypes.h"

#include <string>

class ServerboundPackSettingChangePacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::ServerboundPackSettingChange;

    ServerboundPackSettingChangePacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "ServerboundPackSettingChangePacket"; }

    void write(BinaryStream &stream, const PacketCodecContext &context) const override;

    void read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    Uuid mPackId;
    std::string mPackSettingName;
    DataStoreScalar mPackSettingValue;
};

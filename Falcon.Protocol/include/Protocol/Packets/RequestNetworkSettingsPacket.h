#pragma once

#include "Protocol/Packet.h"

class RequestNetworkSettingsPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::RequestNetworkSettings;

    RequestNetworkSettingsPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "RequestNetworkSettingsPacket"; }

    void write(BinaryStream &stream) const override;

    void read(ReadOnlyBinaryStream &stream) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    int32_t mProtocolVersion;
};

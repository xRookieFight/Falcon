#pragma once

#include "Protocol/Packet.h"

class ServerSettingsResponsePacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::ServerSettingsResponse;

    ServerSettingsResponsePacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "ServerSettingsResponsePacket"; }

    void write(BinaryStream &stream) const override;

    void read(ReadOnlyBinaryStream &stream) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    uint32_t mFormId;
    std::string mFormData;
};

#pragma once

#include "Protocol/Packet.h"

class DisconnectPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::Disconnect;

    DisconnectPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "DisconnectPacket"; }

    void write(BinaryStream &stream) const override;

    void read(ReadOnlyBinaryStream &stream) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    int32_t mReason;
    bool mMessageSkipped;
    std::string mKickMessage;
    std::string mFilteredMessage;
};

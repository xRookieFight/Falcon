#pragma once

#include "Protocol/Packet.h"

class ToastRequestPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::ToastRequest;

    ToastRequestPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "ToastRequestPacket"; }

    void write(BinaryStream &stream) const override;

    void read(ReadOnlyBinaryStream &stream) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    std::string mTitle;
    std::string mContent;
};

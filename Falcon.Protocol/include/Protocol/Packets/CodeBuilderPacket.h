#pragma once

#include "Protocol/Packet.h"

class CodeBuilderPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::CodeBuilder;

    CodeBuilderPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "CodeBuilderPacket"; }

    void write(BinaryStream &stream) const override;

    void read(ReadOnlyBinaryStream &stream) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    std::string mUrl;
    bool mOpening;
};

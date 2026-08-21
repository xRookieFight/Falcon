#pragma once

#include "Protocol/Packet.h"

class SettingsCommandPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::SettingsCommand;

    SettingsCommandPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "SettingsCommandPacket"; }

    void write(BinaryStream &stream) const override;

    void read(ReadOnlyBinaryStream &stream) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    std::string mCommand;
    bool mSuppressingOutput;
};

#pragma once

#include "Protocol/Packet.h"

class StopSoundPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::StopSound;

    StopSoundPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "StopSoundPacket"; }

    void write(BinaryStream &stream) const override;

    void read(ReadOnlyBinaryStream &stream) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    std::string mSoundName;
    bool mStoppingAllSound;
    bool mStopMusicLegacy;
};

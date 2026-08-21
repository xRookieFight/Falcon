#pragma once

#include "Protocol/Packet.h"

class SimulationTypePacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::SimulationType;

    enum class Type : int {
        Game,
        Editor,
        Test
    };

    SimulationTypePacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "SimulationTypePacket"; }

    void write(BinaryStream &stream) const override;

    void read(ReadOnlyBinaryStream &stream) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    Type mType;
};

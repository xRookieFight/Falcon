#pragma once

#include "Protocol/Packet.h"

class AnimatePacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::Animate;

    enum class Action : int {
        NoAction,
        SwingArm,
        WakeUp,
        CriticalHit,
        MagicCriticalHit,
        RowRight,
        RowLeft
    };

    AnimatePacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "AnimatePacket"; }

    void write(BinaryStream &stream) const override;

    void read(ReadOnlyBinaryStream &stream) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    Action mAction;
    uint64_t mRuntimeEntityId;
    float mData;
    std::string mSwingSource;
};

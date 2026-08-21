#pragma once

#include "Protocol/Packet.h"

class InteractPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::Interact;

    enum class Action : int {
        None,
        Interact,
        Damage,
        LeaveVehicle,
        MouseOver,
        NpcOpen,
        OpenInventory
    };

    InteractPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "InteractPacket"; }

    void write(BinaryStream &stream) const override;

    void read(ReadOnlyBinaryStream &stream) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    Action mAction;
    uint64_t mRuntimeEntityId;
    bool mHasMousePosition;
    Vector3f mMousePosition;
};

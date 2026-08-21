#pragma once

#include "Protocol/Packet.h"

class AwardAchievementPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::AwardAchievement;

    AwardAchievementPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "AwardAchievementPacket"; }

    void write(BinaryStream &stream) const override;

    void read(ReadOnlyBinaryStream &stream) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    int32_t mAchievementId;
};

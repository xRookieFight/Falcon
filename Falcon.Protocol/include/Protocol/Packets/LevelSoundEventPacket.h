#pragma once

#include "Core/Math/Vector3f.h"
#include "Protocol/Packet.h"

#include <string>

namespace LevelSoundEvent {
    inline constexpr const char *BURP = "burp";
    inline constexpr const char *THROW = "throw";
    inline constexpr const char *TELEPORT = "teleport";
    inline constexpr const char *BREAK = "break";
    inline constexpr const char *HIT = "hit";
    inline constexpr const char *PLACE = "place";
    inline constexpr const char *ITEM_USE_ON = "item.use.on";
    inline constexpr const char *BUCKET_FILL_WATER = "bucket.fill_water";
    inline constexpr const char *BUCKET_EMPTY_WATER = "bucket.empty_water";
    inline constexpr const char *BUCKET_FILL_LAVA = "bucket.fill_lava";
    inline constexpr const char *BUCKET_EMPTY_LAVA = "bucket.empty_lava";
    inline constexpr const char *WIND_CHARGE_BURST = "wind_charge.burst";
    inline constexpr const char *DRINK = "drink";
}

class LevelSoundEventPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::LevelSoundEvent;

    LevelSoundEventPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "LevelSoundEventPacket"; }

    void write(BinaryStream &stream, const PacketCodecContext &context) const override;

    void read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    std::string mSound;
    Vector3f mPosition;
    int32_t mExtraData;
    std::string mActorType;
    bool mIsBabyMob;
    bool mDisableRelativeVolume;
    int64_t mActorUniqueId;
    bool mHasFirePosition;
    Vector3f mFirePosition;
};

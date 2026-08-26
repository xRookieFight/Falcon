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
    inline constexpr const char *MACE_SMASH_GROUND = "mace.smash_ground";
    inline constexpr const char *MACE_HEAVY_SMASH_GROUND = "mace.heavy_smash_ground";
    inline constexpr const char *MACE_SMASH_AIR = "mace.smash_air";
    inline constexpr const char *SPEAR_LUNGE = "spear.lunge";
    inline constexpr const char *BOW = "random.bow";
    inline constexpr const char *BOW_HIT = "random.bowhit";
    inline constexpr const char *CROSSBOW_LOADING_START = "crossbow.loading.start";
    inline constexpr const char *CROSSBOW_LOADING_MIDDLE = "crossbow.loading.middle";
    inline constexpr const char *CROSSBOW_LOADING_END = "crossbow.loading.end";
    inline constexpr const char *CROSSBOW_QUICK_CHARGE_START = "crossbow.quick_charge.start";
    inline constexpr const char *CROSSBOW_QUICK_CHARGE_MIDDLE = "crossbow.quick_charge.middle";
    inline constexpr const char *CROSSBOW_QUICK_CHARGE_END = "crossbow.quick_charge.end";
    inline constexpr const char *CROSSBOW_SHOOT = "crossbow.shoot";
    inline constexpr const char *TRIDENT_THROW = "item.trident.throw";
    inline constexpr const char *TRIDENT_HIT = "item.trident.hit";
    inline constexpr const char *TRIDENT_HIT_GROUND = "item.trident.hit_ground";
    inline constexpr const char *TRIDENT_RETURN = "item.trident.return";
    inline constexpr const char *TRIDENT_THUNDER = "item.trident.thunder";
    inline constexpr const char *TRIDENT_RIPTIDE_1 = "item.trident.riptide_1";
    inline constexpr const char *TRIDENT_RIPTIDE_2 = "item.trident.riptide_2";
    inline constexpr const char *TRIDENT_RIPTIDE_3 = "item.trident.riptide_3";
    inline constexpr const char *ARMOR_EQUIP_GENERIC = "armor.equip_generic";
    inline constexpr const char *ARMOR_EQUIP_ELYTRA = "armor.equip_elytra";
    inline constexpr const char *PISTON_IN = "tile.piston.in";
    inline constexpr const char *PISTON_OUT = "tile.piston.out";
    inline constexpr const char *CHEST_OPEN = "chest.open";
    inline constexpr const char *CHEST_CLOSED = "chest.closed";
    inline constexpr const char *LAUNCH = "launch";
    inline constexpr const char *LARGE_BLAST = "large.blast";
    inline constexpr const char *THUNDER = "ambient.weather.thunder";
    inline constexpr const char *EXPLODE = "random.explode";
    inline constexpr const char *BUNDLE_INSERT = "bundle.insert";
    inline constexpr const char *BUNDLE_INSERT_FAIL = "bundle.insert_fail";
    inline constexpr const char *BUNDLE_REMOVE_ONE = "bundle.remove_one";
    inline constexpr const char *BUNDLE_DROP_CONTENTS = "bundle.drop_contents";
    inline constexpr const char *FIRE_IGNITE = "fire.ignite";
    inline constexpr const char *GHAST_FIREBALL = "mob.ghast.fireball";
    inline constexpr const char *POWER_ON = "power.on";
    inline constexpr const char *POWER_OFF = "power.off";
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

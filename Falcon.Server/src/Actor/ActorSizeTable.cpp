#include "Actor/ActorSizeTable.h"

#include <unordered_map>

namespace ActorSizeTable {

namespace {

constexpr float kDefaultWidth = 0.6f;
constexpr float kDefaultHeight = 1.8f;

struct SizeEntry {
    const char *mIdentifier;
    float mWidth;
    float mHeight;
};

const SizeEntry kSizes[] = {
    {"minecraft:chicken", 0.6f, 0.8f},
    {"minecraft:cow", 0.9f, 1.3f},
    {"minecraft:pig", 0.9f, 0.9f},
    {"minecraft:sheep", 0.9f, 1.3f},
    {"minecraft:wolf", 0.6f, 0.8f},
    {"minecraft:villager", 0.6f, 1.9f},
    {"minecraft:villager_v2", 0.6f, 1.9f},
    {"minecraft:mooshroom", 0.9f, 1.3f},
    {"minecraft:squid", 0.95f, 0.95f},
    {"minecraft:glow_squid", 0.475f, 0.95f},
    {"minecraft:rabbit", 0.402f, 0.402f},
    {"minecraft:bat", 0.5f, 0.9f},
    {"minecraft:iron_golem", 1.4f, 2.9f},
    {"minecraft:snow_golem", 0.4f, 1.8f},
    {"minecraft:copper_golem", 0.49f, 0.98f},
    {"minecraft:ocelot", 0.6f, 0.7f},
    {"minecraft:cat", 0.48f, 0.56f},
    {"minecraft:horse", 1.4f, 1.6f},
    {"minecraft:donkey", 1.4f, 1.6f},
    {"minecraft:mule", 1.4f, 1.6f},
    {"minecraft:skeleton_horse", 1.4f, 1.6f},
    {"minecraft:zombie_horse", 1.4f, 1.6f},
    {"minecraft:polar_bear", 1.3f, 1.4f},
    {"minecraft:llama", 0.6f, 1.9f},
    {"minecraft:trader_llama", 0.6f, 1.9f},
    {"minecraft:parrot", 0.5f, 1.0f},
    {"minecraft:dolphin", 0.9f, 0.6f},
    {"minecraft:turtle", 1.2f, 0.4f},
    {"minecraft:panda", 1.7f, 1.5f},
    {"minecraft:fox", 0.6f, 0.7f},
    {"minecraft:bee", 0.55f, 0.5f},
    {"minecraft:goat", 0.9f, 1.3f},
    {"minecraft:axolotl", 0.75f, 0.42f},
    {"minecraft:frog", 0.5f, 0.55f},
    {"minecraft:tadpole", 0.6f, 0.8f},
    {"minecraft:allay", 0.6f, 0.6f},
    {"minecraft:camel", 1.7f, 2.375f},
    {"minecraft:camel_husk", 1.7f, 2.375f},
    {"minecraft:sniffer", 1.9f, 1.75f},
    {"minecraft:armadillo", 0.7f, 0.65f},
    {"minecraft:strider", 0.9f, 1.7f},
    {"minecraft:hoglin", 1.4f, 1.4f},
    {"minecraft:zoglin", 1.4f, 1.4f},
    {"minecraft:nautilus", 0.875f, 0.95f},
    {"minecraft:zombie_nautilus", 0.875f, 0.95f},
    {"minecraft:happy_ghast", 4.0f, 4.0f},
    {"minecraft:cod", 0.6f, 0.3f},
    {"minecraft:salmon", 0.5f, 0.5f},
    {"minecraft:pufferfish", 0.8f, 0.8f},
    {"minecraft:tropicalfish", 0.5f, 0.4f},
    {"minecraft:zombie", 0.6f, 1.9f},
    {"minecraft:zombie_villager", 0.6f, 1.9f},
    {"minecraft:zombie_villager_v2", 0.6f, 1.9f},
    {"minecraft:drowned", 0.6f, 1.9f},
    {"minecraft:husk", 0.6f, 1.9f},
    {"minecraft:zombie_pigman", 0.6f, 1.9f},
    {"minecraft:creeper", 0.6f, 1.8f},
    {"minecraft:skeleton", 0.6f, 1.9f},
    {"minecraft:stray", 0.6f, 1.9f},
    {"minecraft:bogged", 0.6f, 1.9f},
    {"minecraft:parched", 0.6f, 1.9f},
    {"minecraft:wither_skeleton", 0.7f, 2.4f},
    {"minecraft:spider", 1.4f, 0.9f},
    {"minecraft:cave_spider", 0.7f, 0.5f},
    {"minecraft:slime", 2.04f, 2.04f},
    {"minecraft:magma_cube", 2.04f, 2.04f},
    {"minecraft:sulfur_cube", 0.98f, 0.98f},
    {"minecraft:enderman", 0.6f, 2.9f},
    {"minecraft:endermite", 0.4f, 0.3f},
    {"minecraft:silverfish", 0.4f, 0.3f},
    {"minecraft:ghast", 4.0f, 4.0f},
    {"minecraft:blaze", 0.5f, 1.8f},
    {"minecraft:witch", 0.6f, 1.9f},
    {"minecraft:guardian", 0.85f, 0.85f},
    {"minecraft:elder_guardian", 1.99f, 1.99f},
    {"minecraft:npc", 0.6f, 2.1f},
    {"minecraft:wither", 1.0f, 3.0f},
    {"minecraft:ender_dragon", 13.0f, 4.0f},
    {"minecraft:shulker", 0.99f, 0.99f},
    {"minecraft:vindicator", 0.6f, 1.9f},
    {"minecraft:evocation_illager", 0.6f, 1.9f},
    {"minecraft:pillager", 0.6f, 1.9f},
    {"minecraft:ravager", 1.2f, 1.9f},
    {"minecraft:vex", 0.4f, 0.8f},
    {"minecraft:phantom", 0.9f, 0.5f},
    {"minecraft:piglin", 0.6f, 1.9f},
    {"minecraft:piglin_brute", 0.6f, 1.9f},
    {"minecraft:warden", 0.9f, 2.9f},
    {"minecraft:breeze", 0.6f, 1.77f},
    {"minecraft:creaking", 1.0f, 2.5f},
    {"minecraft:wandering_trader", 0.6f, 1.9f},
    {"minecraft:armor_stand", 0.5f, 1.975f},
    {"minecraft:item", 0.25f, 0.25f},
    {"minecraft:xp_orb", 0.25f, 0.25f},
    {"minecraft:xp_bottle", 0.25f, 0.25f},
    {"minecraft:tnt", 0.98f, 0.98f},
    {"minecraft:falling_block", 0.98f, 0.98f},
    {"minecraft:ender_crystal", 0.98f, 0.98f},
    {"minecraft:fireworks_rocket", 0.25f, 0.25f},
    {"minecraft:eye_of_ender_signal", 0.25f, 0.25f},
    {"minecraft:area_effect_cloud", 3.0f, 0.5f},
    {"minecraft:arrow", 0.1f, 0.1f},
    {"minecraft:thrown_trident", 0.1f, 0.1f},
    {"minecraft:fishing_hook", 0.1f, 0.1f},
    {"minecraft:snowball", 0.25f, 0.25f},
    {"minecraft:egg", 0.25f, 0.25f},
    {"minecraft:ender_pearl", 0.25f, 0.25f},
    {"minecraft:splash_potion", 0.25f, 0.25f},
    {"minecraft:lingering_potion", 0.25f, 0.25f},
    {"minecraft:wither_skull", 0.25f, 0.25f},
    {"minecraft:wither_skull_dangerous", 0.25f, 0.25f},
    {"minecraft:fireball", 0.31f, 0.31f},
    {"minecraft:small_fireball", 0.3125f, 0.3125f},
    {"minecraft:dragon_fireball", 0.3125f, 0.3125f},
    {"minecraft:shulker_bullet", 0.3125f, 0.3125f},
    {"minecraft:wind_charge_projectile", 0.3125f, 0.3125f},
    {"minecraft:breeze_wind_charge_projectile", 0.3125f, 0.3125f},
    {"minecraft:llama_spit", 0.9f, 1.87f},
    {"minecraft:evocation_fang", 1.0f, 0.8f},
    {"minecraft:boat", 1.3f, 0.5f},
    {"minecraft:chest_boat", 1.3f, 0.5f},
    {"minecraft:minecart", 0.98f, 0.7f},
    {"minecraft:hopper_minecart", 0.98f, 0.7f},
    {"minecraft:tnt_minecart", 0.98f, 0.7f},
    {"minecraft:chest_minecart", 0.98f, 0.7f},
    {"minecraft:command_block_minecart", 0.98f, 0.7f},
};

const std::unordered_map<std::string, ActorSize> &getTable() {
    static const std::unordered_map<std::string, ActorSize> kTable = [] {
        std::unordered_map<std::string, ActorSize> table;
        table.reserve(sizeof(kSizes) / sizeof(kSizes[0]));
        for (const SizeEntry &entry : kSizes) {
            table.emplace(entry.mIdentifier, ActorSize{entry.mWidth, entry.mHeight});
        }
        return table;
    }();

    return kTable;
}

}

ActorSize getSize(const std::string &identifier) {
    const std::unordered_map<std::string, ActorSize> &table = getTable();
    auto found = table.find(identifier);
    if (found == table.end()) {
        return ActorSize{kDefaultWidth, kDefaultHeight};
    }

    return found->second;
}

}

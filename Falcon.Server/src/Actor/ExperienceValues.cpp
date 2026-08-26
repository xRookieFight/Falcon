#include "Actor/ExperienceValues.h"

#include <random>
#include <unordered_map>
#include <unordered_set>

namespace {

    const int ORB_SPLIT_SIZES[] = {2477, 1237, 617, 307, 149, 73, 37, 17, 7, 3, 1};

    std::mt19937 &experienceRandom() {
        static std::mt19937 generator(0x9E3779B9u);
        return generator;
    }

    int randomRange(int minInclusive, int maxInclusive) {
        std::uniform_int_distribution<int> distribution(minInclusive, maxInclusive);
        return distribution(experienceRandom());
    }

    int getMaxOrbSize(int amount) {
        for (int split: ORB_SPLIT_SIZES) {
            if (amount >= split)
                return split;
        }

        return 1;
    }

    const std::unordered_set<std::string> &breedingAnimals() {
        static const std::unordered_set<std::string> animals = {
                "minecraft:chicken",
                "minecraft:cow",
                "minecraft:pig",
                "minecraft:sheep",
                "minecraft:wolf",
                "minecraft:mooshroom",
                "minecraft:rabbit",
                "minecraft:ocelot",
                "minecraft:horse",
                "minecraft:donkey",
                "minecraft:mule",
                "minecraft:skeleton_horse",
                "minecraft:zombie_horse",
                "minecraft:polar_bear",
                "minecraft:llama",
                "minecraft:trader_llama",
                "minecraft:parrot",
                "minecraft:turtle",
                "minecraft:cat",
                "minecraft:panda",
                "minecraft:fox",
                "minecraft:bee",
                "minecraft:goat",
                "minecraft:camel",
                "minecraft:sniffer",
                "minecraft:armadillo",
                "minecraft:frog",
                "minecraft:tadpole",
                "minecraft:squid",
                "minecraft:glow_squid",
                "minecraft:nautilus",
                "minecraft:pufferfish",
                "minecraft:salmon",
                "minecraft:cod",
                "minecraft:tropicalfish"
        };

        return animals;
    }

    const std::unordered_set<std::string> &hostileMobs() {
        static const std::unordered_set<std::string> mobs = {
                "minecraft:zombie",
                "minecraft:husk",
                "minecraft:zombie_villager",
                "minecraft:zombie_villager_v2",
                "minecraft:drowned",
                "minecraft:zombie_pigman",
                "minecraft:skeleton",
                "minecraft:stray",
                "minecraft:bogged",
                "minecraft:wither_skeleton",
                "minecraft:parched",
                "minecraft:creeper",
                "minecraft:spider",
                "minecraft:cave_spider",
                "minecraft:silverfish",
                "minecraft:enderman",
                "minecraft:witch",
                "minecraft:ghast",
                "minecraft:phantom",
                "minecraft:vex",
                "minecraft:shulker",
                "minecraft:warden",
                "minecraft:pillager",
                "minecraft:vindicator",
                "minecraft:piglin",
                "minecraft:piglin_brute"
        };

        return mobs;
    }

    const std::unordered_map<std::string, int> &oreExperienceRanges() {
        static const std::unordered_map<std::string, int> ores = {
                {"minecraft:coal_ore", 0},
                {"minecraft:deepslate_coal_ore", 0},
                {"minecraft:diamond_ore", 1},
                {"minecraft:deepslate_diamond_ore", 1},
                {"minecraft:emerald_ore", 2},
                {"minecraft:deepslate_emerald_ore", 2},
                {"minecraft:lapis_ore", 3},
                {"minecraft:deepslate_lapis_ore", 3},
                {"minecraft:redstone_ore", 4},
                {"minecraft:lit_redstone_ore", 4},
                {"minecraft:deepslate_redstone_ore", 4},
                {"minecraft:lit_deepslate_redstone_ore", 4},
                {"minecraft:quartz_ore", 5}
        };

        return ores;
    }

}

namespace ExperienceValues {

    std::vector<int> splitIntoOrbSizes(int amount) {
        std::vector<int> result;

        while (amount > 0) {
            const int size = getMaxOrbSize(amount);
            result.push_back(size);
            amount -= size;
        }

        return result;
    }

    int getMobDropExperience(const std::string &identifier) {
        if (identifier == "minecraft:blaze" || identifier == "minecraft:breeze" ||
            identifier == "minecraft:guardian" || identifier == "minecraft:elder_guardian" ||
            identifier == "minecraft:evocation_illager")
            return 10;

        if (identifier == "minecraft:endermite")
            return 3;

        if (identifier == "minecraft:ravager")
            return 20;

        if (identifier == "minecraft:wither")
            return 50;

        if (identifier == "minecraft:ender_dragon")
            return 12000;

        if (identifier == "minecraft:axolotl")
            return 1;

        if (identifier == "minecraft:strider")
            return randomRange(1, 2);

        if (identifier == "minecraft:sulfur_cube")
            return randomRange(1, 2);

        if (identifier == "minecraft:hoglin" || identifier == "minecraft:zoglin" ||
            identifier == "minecraft:happy_ghast")
            return randomRange(1, 3);

        if (hostileMobs().count(identifier) != 0)
            return 5;

        if (breedingAnimals().count(identifier) != 0)
            return randomRange(1, 3);

        return 0;
    }

    int getOreDropExperience(const std::string &blockName) {
        const auto it = oreExperienceRanges().find(blockName);
        if (it == oreExperienceRanges().end())
            return 0;

        switch (it->second) {
            case 0:
                return randomRange(0, 2);
            case 1:
                return randomRange(3, 7);
            case 2:
                return randomRange(3, 7);
            case 3:
                return randomRange(2, 5);
            case 4:
                return randomRange(1, 5);
            case 5:
                return randomRange(1, 5);
            default:
                return 0;
        }
    }

}

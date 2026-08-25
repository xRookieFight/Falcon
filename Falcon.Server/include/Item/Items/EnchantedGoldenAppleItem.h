#pragma once

#include <cstdint>
#include <string>

class FoodItemComponent;
class ItemStack;

class EnchantedGoldenAppleItem {
public:
    static constexpr const char *IDENTIFIER = "minecraft:enchanted_golden_apple";

    static constexpr int32_t NUTRITION = 4;
    static constexpr float SATURATION = 9.6f;

    static constexpr int32_t REGENERATION_DURATION_TICKS = 600;
    static constexpr int32_t REGENERATION_AMPLIFIER = 1;
    static constexpr int32_t ABSORPTION_DURATION_TICKS = 2400;
    static constexpr int32_t ABSORPTION_AMPLIFIER = 3;
    static constexpr int32_t RESISTANCE_DURATION_TICKS = 6000;
    static constexpr int32_t FIRE_RESISTANCE_DURATION_TICKS = 6000;

    static bool matches(const std::string &identifier);

    static bool matches(const ItemStack &item);

    static void applyFoodComponent(FoodItemComponent &food);
};

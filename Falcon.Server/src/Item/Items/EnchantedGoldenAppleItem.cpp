#include "item/items/EnchantedGoldenAppleItem.h"

#include "actor/MobEffect.h"
#include "item/components/ItemComponentTypes.h"
#include "protocol/types/ItemStack.h"

bool EnchantedGoldenAppleItem::matches(const std::string &identifier) {
    return identifier == IDENTIFIER;
}

bool EnchantedGoldenAppleItem::matches(const ItemStack &item) {
    return item.mDefinition != nullptr && matches(item.mDefinition->getIdentifier());
}

void EnchantedGoldenAppleItem::applyFoodComponent(FoodItemComponent &food) {
    food.mNutrition = NUTRITION;
    food.mSaturation = SATURATION;
    food.mCanAlwaysEat = true;

    food.addEffect({(int32_t) MobEffectId::Regeneration, REGENERATION_AMPLIFIER, REGENERATION_DURATION_TICKS, 1.0f});
    food.addEffect({(int32_t) MobEffectId::Absorption, ABSORPTION_AMPLIFIER, ABSORPTION_DURATION_TICKS, 1.0f});
    food.addEffect({(int32_t) MobEffectId::Resistance, 0, RESISTANCE_DURATION_TICKS, 1.0f});
    food.addEffect({(int32_t) MobEffectId::FireResistance, 0, FIRE_RESISTANCE_DURATION_TICKS, 1.0f});
}

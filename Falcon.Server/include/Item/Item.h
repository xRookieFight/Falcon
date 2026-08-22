#pragma once

#include "Block/Block.h"
#include "Item/ItemData.h"
#include "Item/ItemTypes.h"

#include <cstdint>
#include <string>

class Item {
public:
    Item() : mTypeId(0), mIdentifier("minecraft:air"), mName("Air"), mMaxStackSize(64), mIsBlock(false) {}

    Item(int32_t typeId, const std::string &identifier, const std::string &name, int32_t maxStackSize = 64)
            : mTypeId(typeId), mIdentifier(identifier), mName(name), mMaxStackSize(maxStackSize), mIsBlock(false) {}

    Item(const Block &block, int32_t maxStackSize = 64)
            : mTypeId(block.getTypeId()), mIdentifier(block.getIdentifier()), mName(block.getName()),
              mMaxStackSize(maxStackSize), mIsBlock(true), mBlock(block) {}

    explicit Item(const ItemData &data)
            : mTypeId(data.mTypeId), mIdentifier(data.mIdentifier), mName(data.mName),
              mMaxStackSize(data.mMaxStackSize), mIsBlock(false), mMaxDurability(data.mMaxDurability),
              mToolType(data.mToolType), mToolTier(data.mToolTier), mHarvestLevel(data.mHarvestLevel),
              mAttackDamage(data.mAttackDamage), mArmorSlot(data.mArmorSlot), mArmorTier(data.mArmorTier),
              mArmorPoints(data.mArmorPoints), mArmorToughness(data.mArmorToughness),
              mNutrition(data.mNutrition), mSaturation(data.mSaturation) {}

    int32_t getTypeId() const { return mTypeId; }

    const std::string &getIdentifier() const { return mIdentifier; }

    const std::string &getName() const { return mName; }

    int32_t getMaxStackSize() const { return mMaxStackSize; }

    bool isBlock() const { return mIsBlock; }

    const Block &getBlock() const { return mBlock; }

    int32_t getMaxDurability() const { return mMaxDurability; }

    bool isDurable() const { return mMaxDurability > 0; }

    ToolType getToolType() const { return mToolType; }

    bool isTool() const { return mToolType != ToolType::None; }

    int32_t getToolTier() const { return mToolTier; }

    int32_t getHarvestLevel() const { return mHarvestLevel; }

    int32_t getAttackDamage() const { return mAttackDamage; }

    ArmorSlot getArmorSlot() const { return mArmorSlot; }

    bool isArmor() const { return mArmorSlot != ArmorSlot::None; }

    int32_t getArmorTier() const { return mArmorTier; }

    int32_t getArmorPoints() const { return mArmorPoints; }

    int32_t getArmorToughness() const { return mArmorToughness; }

    int32_t getNutrition() const { return mNutrition; }

    float getSaturation() const { return mSaturation; }

    bool isEdible() const { return mNutrition > 0; }

private:
    int32_t mTypeId;
    std::string mIdentifier;
    std::string mName;
    int32_t mMaxStackSize;
    bool mIsBlock;
    Block mBlock;
    int32_t mMaxDurability = 0;
    ToolType mToolType = ToolType::None;
    int32_t mToolTier = 0;
    int32_t mHarvestLevel = 0;
    int32_t mAttackDamage = 0;
    ArmorSlot mArmorSlot = ArmorSlot::None;
    int32_t mArmorTier = 0;
    int32_t mArmorPoints = 0;
    int32_t mArmorToughness = 0;
    int32_t mNutrition = 0;
    float mSaturation = 0.0f;
};

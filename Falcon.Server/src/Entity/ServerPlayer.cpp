#include "Entity/ServerPlayer.h"

#include "Inventory/ItemStackNbt.h"
#include "Protocol/Packets/SetTitlePacket.h"
#include "Protocol/Packets/TextPacket.h"

#include <chrono>
#include <utility>

namespace {
    const char *TAG_POS = "Pos";
    const char *TAG_MOTION = "Motion";
    const char *TAG_ROTATION = "Rotation";
    const char *TAG_HEALTH = "Health";
    const char *TAG_LEVEL = "Level";
    const char *TAG_GAME_MODE = "playerGameType";
    const char *TAG_FIRST_PLAYED = "firstPlayed";
    const char *TAG_LAST_PLAYED = "lastPlayed";
    const char *TAG_LAST_KNOWN_XUID = "LastKnownXUID";
    const char *TAG_NAME = "NameTag";
    const char *TAG_INVENTORY = "Inventory";
    const char *TAG_OFF_INVENTORY = "OffInventory";
    const char *TAG_SELECTED_SLOT = "SelectedInventorySlot";
    const char *TAG_FOOD_LEVEL = "foodLevel";
    const char *TAG_FOOD_EXHAUSTION_LEVEL = "foodExhaustionLevel";
    const char *TAG_FOOD_SATURATION_LEVEL = "foodSaturationLevel";
    const char *TAG_FOOD_TICK_TIMER = "foodTickTimer";
    const char *TAG_XP_LEVEL = "XpLevel";
    const char *TAG_XP_PROGRESS = "XpP";
    const char *TAG_LIFETIME_XP_TOTAL = "XpTotal";

    int64_t currentTimeMillis() {
        using namespace std::chrono;
        return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    }

    Tag floatList(float first, float second, float third) {
        return Tag::ofList(Tag::Type::Float, {Tag::ofFloat(first), Tag::ofFloat(second), Tag::ofFloat(third)});
    }

    float listValue(const Tag &data, const char *key, size_t index, float fallback) {
        const Tag *list = data.get(key);
        if (list == nullptr || list->getType() != Tag::Type::List)
            return fallback;

        const std::vector<Tag> &values = list->getList();
        if (index >= values.size())
            return fallback;

        return values[index].asFloat();
    }
}

ServerPlayer::ServerPlayer(const NetworkIdentifier &id, uint64_t runtimeId, PacketSender *sender)
        : LivingEntity(runtimeId), mId(id), mLoginState(LoginState::Connecting), mSender(sender) {}

void ServerPlayer::sendMessage(const std::string &message) {
    if (mSender == nullptr)
        return;

    TextPacket text;
    text.mType = TextPacket::Type::Raw;
    text.mMessage = message;
    mSender->sendPacketTo(mId, text);
}

void ServerPlayer::sendTip(const std::string &message) {
    if (mSender == nullptr)
        return;

    TextPacket text;
    text.mType = TextPacket::Type::Tip;
    text.mMessage = message;
    mSender->sendPacketTo(mId, text);
}

void ServerPlayer::sendPopup(const std::string &message) {
    if (mSender == nullptr)
        return;

    TextPacket text;
    text.mType = TextPacket::Type::Popup;
    text.mMessage = message;
    mSender->sendPacketTo(mId, text);
}

void ServerPlayer::sendTitle(const std::string &title, const std::string &subtitle, int32_t fadeInTime,
                             int32_t stayTime, int32_t fadeOutTime) {
    if (mSender == nullptr)
        return;

    SetTitlePacket times;
    times.mType = SetTitlePacket::Type::Times;
    times.mFadeInTime = fadeInTime;
    times.mStayTime = stayTime;
    times.mFadeOutTime = fadeOutTime;
    mSender->sendPacketTo(mId, times);

    if (!subtitle.empty()) {
        SetTitlePacket subtitlePacket;
        subtitlePacket.mType = SetTitlePacket::Type::Subtitle;
        subtitlePacket.mText = subtitle;
        mSender->sendPacketTo(mId, subtitlePacket);
    }

    SetTitlePacket titlePacket;
    titlePacket.mType = SetTitlePacket::Type::Title;
    titlePacket.mText = title;
    mSender->sendPacketTo(mId, titlePacket);
}

Tag ServerPlayer::saveNbt(const std::string &levelName) const {
    Tag data = Tag::ofCompound();

    data.put(TAG_POS, floatList(mPosition.x, mPosition.y, mPosition.z));
    data.put(TAG_MOTION, floatList(mMotion.x, mMotion.y, mMotion.z));
    data.put(TAG_ROTATION, floatList(mRotation.x, mRotation.y, mRotation.z));

    data.putFloat(TAG_HEALTH, mAttributes.get("minecraft:health"));
    data.putString(TAG_LEVEL, levelName);
    data.putInt(TAG_GAME_MODE, mGameType);
    data.putLong(TAG_FIRST_PLAYED, mFirstPlayed == 0 ? currentTimeMillis() : mFirstPlayed);
    data.putLong(TAG_LAST_PLAYED, currentTimeMillis());
    data.putString(TAG_LAST_KNOWN_XUID, mXuid);
    data.putString(TAG_NAME, mName);

    data.putInt(TAG_FOOD_LEVEL, (int32_t) getFood());
    data.putFloat(TAG_FOOD_EXHAUSTION_LEVEL, getExhaustion());
    data.putFloat(TAG_FOOD_SATURATION_LEVEL, getSaturation());
    data.putInt(TAG_FOOD_TICK_TIMER, mFoodTickTimer);

    data.putInt(TAG_XP_LEVEL, mExperience.getXpLevel());
    data.putFloat(TAG_XP_PROGRESS, mExperience.getXpProgress());
    data.putInt(TAG_LIFETIME_XP_TOTAL, mExperience.getLifetimeTotalXp());

    Tag inventory = Tag::ofList(Tag::Type::Compound);

    for (int slot = 0; slot < PlayerInventory::CONTAINER_SIZE; slot++) {
        const ItemStack &item = mInventory.getItem(slot);
        if (!item.isAir())
            inventory.addToList(ItemStackNbt::write(item, slot));
    }

    for (int slot = 0; slot < PlayerInventory::ARMOR_SIZE; slot++) {
        const ItemStack &item = mInventory.getArmor(slot);
        if (!item.isAir())
            inventory.addToList(ItemStackNbt::write(item, PlayerInventory::CONTAINER_SIZE + slot));
    }

    data.put(TAG_INVENTORY, inventory);
    data.put(TAG_OFF_INVENTORY, ItemStackNbt::write(mInventory.getOffhand(), 0));
    data.putInt(TAG_SELECTED_SLOT, mInventory.getSelectedSlot());

    return data;
}

void ServerPlayer::loadNbt(const Tag &data, const PacketCodecContext &context) {
    if (data.getType() != Tag::Type::Compound)
        return;

    mPosition = Vector3f(listValue(data, TAG_POS, 0, mPosition.x),
                         listValue(data, TAG_POS, 1, mPosition.y),
                         listValue(data, TAG_POS, 2, mPosition.z));

    mMotion = Vector3f(listValue(data, TAG_MOTION, 0, 0.0f),
                       listValue(data, TAG_MOTION, 1, 0.0f),
                       listValue(data, TAG_MOTION, 2, 0.0f));

    mRotation = Vector3f(listValue(data, TAG_ROTATION, 0, 0.0f),
                         listValue(data, TAG_ROTATION, 1, 0.0f),
                         listValue(data, TAG_ROTATION, 2, 0.0f));

    mAttributes.set("minecraft:health", data.getFloat(TAG_HEALTH, 20.0f));
    mGameType = data.getInt(TAG_GAME_MODE, mGameType);
    mFirstPlayed = data.getLong(TAG_FIRST_PLAYED, 0);

    setFood((float) data.getInt(TAG_FOOD_LEVEL, (int32_t) getFood()));
    setExhaustion(data.getFloat(TAG_FOOD_EXHAUSTION_LEVEL, getExhaustion()));
    setSaturation(data.getFloat(TAG_FOOD_SATURATION_LEVEL, getSaturation()));
    setFoodTickTimer(data.getInt(TAG_FOOD_TICK_TIMER, mFoodTickTimer));

    mExperience.setXpAndProgress(data.getInt(TAG_XP_LEVEL, 0), data.getFloat(TAG_XP_PROGRESS, 0.0f));
    mExperience.setLifetimeTotalXp(data.getInt(TAG_LIFETIME_XP_TOTAL, 0));
    syncExperience();

    mInventory.clear();

    const Tag *inventory = data.get(TAG_INVENTORY);
    if (inventory != nullptr && inventory->getType() == Tag::Type::List) {
        for (const Tag &entry: inventory->getList()) {
            const int slot = ItemStackNbt::readSlot(entry);
            if (slot < 0)
                continue;

            ItemStack item = ItemStackNbt::read(entry, context);
            if (item.isAir())
                continue;

            if (slot < PlayerInventory::CONTAINER_SIZE)
                mInventory.setItem(slot, std::move(item));
            else if (slot < PlayerInventory::SAVED_SIZE)
                mInventory.setArmor(slot - PlayerInventory::CONTAINER_SIZE, std::move(item));
        }
    }

    const Tag *offhand = data.get(TAG_OFF_INVENTORY);
    if (offhand != nullptr && offhand->getType() == Tag::Type::Compound)
        mInventory.setOffhand(ItemStackNbt::read(*offhand, context));

    mInventory.setSelectedSlot(data.getInt(TAG_SELECTED_SLOT, 0));
}

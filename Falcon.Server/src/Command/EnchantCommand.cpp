#include "Command/EnchantCommand.h"

#include "Entity/ServerPlayer.h"
#include "Item/ItemEnchantments.h"
#include "Network/ServerNetworkHandler.h"

#include <cstdlib>

EnchantCommand::EnchantCommand(ServerNetworkHandler &handler)
        : Command("enchant", "Adds an enchantment to a player's held item",
                  "/enchant <player> <enchantment> [level]"),
          mHandler(handler) {}

bool EnchantCommand::_parseLevel(const std::string &value, int32_t &out) {
    if (value.empty()) {
        return false;
    }

    for (char character: value) {
        if (character < '0' || character > '9') {
            return false;
        }
    }

    out = (int32_t) strtol(value.c_str(), nullptr, 10);
    return true;
}

bool EnchantCommand::execute(CommandSender &sender, const std::vector<std::string> &arguments) {
    if (arguments.size() < 2) {
        sender.sendMessage("Usage: " + getUsage());
        return false;
    }

    ServerPlayer *target = mHandler.getPlayerByName(arguments[0]);
    if (target == nullptr) {
        sender.sendMessage("§cNo targets matched selector");
        return false;
    }

    const EnchantmentData *enchantment = EnchantmentTable::find(arguments[1]);
    if (enchantment == nullptr) {
        sender.sendMessage("§cUnknown enchantment " + arguments[1]);
        return false;
    }

    int32_t level = 1;
    if (arguments.size() > 2 && !_parseLevel(arguments[2], level)) {
        sender.sendMessage("§cLevel must be a number");
        return false;
    }

    if (level < 1 || level > enchantment->mMaxLevel) {
        sender.sendMessage("§cLevel for " + std::string(enchantment->mDisplayName) + " must be between 1 and " +
                           std::to_string(enchantment->mMaxLevel));
        return false;
    }

    PlayerInventory &inventory = target->getInventory();
    ItemStack item = inventory.getItemInHand();

    if (item.isAir() || item.mCount <= 0) {
        sender.sendMessage("§c" + target->getName() + " is not holding any item");
        return false;
    }

    if (item.mDefinition->getIdentifier() == "minecraft:book") {
        std::shared_ptr<ItemDefinition> enchantedBook =
                mHandler.getItemDefinitions().getDefinition("minecraft:enchanted_book");

        if (enchantedBook != nullptr) {
            item.mDefinition = enchantedBook;
        }
    }

    const EnchantmentData *conflict = nullptr;
    const EnchantmentApplyResult result = ItemEnchantments::apply(item, *enchantment, level, &conflict);

    if (result == EnchantmentApplyResult::IncompatibleItem) {
        sender.sendMessage("§c" + std::string(enchantment->mDisplayName) + " cannot be applied to " +
                           item.mDefinition->getIdentifier());
        return false;
    }

    if (result == EnchantmentApplyResult::IncompatibleEnchantment) {
        const std::string conflictName = conflict != nullptr ? conflict->mDisplayName : "another enchantment";
        sender.sendMessage("§c" + std::string(enchantment->mDisplayName) + " cannot be combined with " + conflictName);
        return false;
    }

    if (result != EnchantmentApplyResult::Success) {
        sender.sendMessage("§cLevel for " + std::string(enchantment->mDisplayName) + " must be between 1 and " +
                           std::to_string(enchantment->mMaxLevel));
        return false;
    }

    inventory.setItemInHand(item);
    target->getInventoryManager().syncSlot(InventoryManager::InventoryId::Inventory, inventory.getSelectedSlot());

    const std::string applied = std::string(enchantment->mDisplayName) + " " + std::to_string(level);

    sender.sendMessage("Applied " + applied + " to " + target->getName() + "'s held item");

    if (target != sender.asPlayer()) {
        target->sendMessage("§eYour held item was enchanted with " + applied);
    }

    return true;
}

std::vector<CommandOverloadData> EnchantCommand::getOverloads() const {
    CommandParamData enchantmentParameter;
    enchantmentParameter.mName = "enchantmentName";
    enchantmentParameter.mHasEnumData = true;
    enchantmentParameter.mEnumData.mName = "Enchant";

    const EnchantmentData *entries = EnchantmentTable::getAll();
    for (size_t index = 0; index < EnchantmentTable::getCount(); index++)
        enchantmentParameter.mEnumData.mValues.push_back(entries[index].mName);

    CommandParamData levelParameter;
    levelParameter.mName = "level";
    levelParameter.mOptional = true;
    levelParameter.mHasType = true;
    levelParameter.mType = CommandParamType::Int;

    CommandOverloadData overload;
    overload.mParameters.push_back(makePlayerParameter("player", mHandler.getPlayerNames()));
    overload.mParameters.push_back(enchantmentParameter);
    overload.mParameters.push_back(levelParameter);

    return {overload};
}

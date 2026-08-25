#include "inventory/InventoryManager.h"
#include "actor/ItemActor.h"
#include "block/actor/FurnaceBlockActor.h"
#include "inventory/InventoryManager.h"
#include "actor/ServerPlayer.h"
#include "block/Block.h"
#include "block/blocks/FurnaceBlock.h"
#include "block/inventory/FurnaceInventory.h"
#include "item/CraftingRecipeTable.h"
#include "inventory/CraftingManager.h"
#include "block/inventory/CraftingTableInventory.h"
#include "network/PacketSender.h"
#include "network/handler/ServerNetworkHandler.h"
#include "network/handler/BlockActionHandler.h"
#include "protocol/BlockStateHasher.h"
#include "protocol/packets/ContainerClosePacket.h"
#include "protocol/packets/ContainerOpenPacket.h"
#include "protocol/packets/ContainerSetDataPacket.h"
#include "protocol/packets/UpdateBlockPacket.h"
#include "protocol/packets/InventoryContentPacket.h"
#include "protocol/packets/InventorySlotPacket.h"
#include "protocol/packets/MobEquipmentPacket.h"
#include "protocol/types/ContainerSlotType.h"

#include <algorithm>
#include <array>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {
    struct FurnaceKey {
        int32_t x;
        int32_t y;
        int32_t z;

        bool operator==(const FurnaceKey &other) const {
            return x == other.x && y == other.y && z == other.z;
        }
    };

    struct FurnaceKeyHash {
        size_t operator()(const FurnaceKey &key) const {
            size_t value = (size_t) (uint32_t) key.x;
            value = value * 31u + (size_t) (uint32_t) key.y;
            value = value * 31u + (size_t) (uint32_t) key.z;
            return value;
        }
    };

    using FurnaceStoredState = FurnaceBlockActor;

    std::unordered_map<FurnaceKey, FurnaceStoredState, FurnaceKeyHash> furnaceStates;
    std::unordered_map<FurnaceKey, int64_t, FurnaceKeyHash> furnaceLastTick;

    const ItemStack &itemAt(const PlayerInventory &inventory, InventoryManager::InventoryId id, int slot) {
        switch (id) {
            case InventoryManager::InventoryId::Inventory:
                return inventory.getItem(slot);
            case InventoryManager::InventoryId::Offhand:
                return inventory.getOffhand();
            case InventoryManager::InventoryId::Armor:
                return inventory.getArmor(slot);
            case InventoryManager::InventoryId::Cursor:
                return inventory.getCursor();
            case InventoryManager::InventoryId::CraftingInput:
                return inventory.getCraftingItem(slot);
            case InventoryManager::InventoryId::CraftingTableInput:
                return inventory.getCraftingTableItem(slot);
            case InventoryManager::InventoryId::FurnaceInput:
                return inventory.getFurnaceItem(slot);
        }
        return PlayerInventory::getEmptyItem();
    }

    bool furnaceItemsEqual(const ItemStack &left, const ItemStack &right) {
        return left.mDefinition == right.mDefinition && left.mBlockDefinition == right.mBlockDefinition
               && left.mCount == right.mCount && left.mDamage == right.mDamage && left.mTag == right.mTag;
    }
}

InventoryManager::InventoryManager()
        : mPlayer(nullptr), mSender(nullptr), mLastInventoryNetworkId(CONTAINER_ID_FIRST),
          mCurrentWindowType(ContainerType::Container), mMainInventoryWindowId(CONTAINER_ID_NONE),
          mCraftingTableWindowId(CONTAINER_ID_NONE), mCraftingTablePosition(0, 0, 0),
          mFurnaceWindowId(CONTAINER_ID_NONE), mFurnacePosition(0, 0, 0),
          mFurnaceKind(FurnaceKind::Furnace), mFurnaceContainerType(ContainerType::Furnace),
          mFurnaceBurnTime(0), mFurnaceMaxBurnTime(0), mFurnaceCookTime(0),
          mClientSelectedHotbarSlot(-1), mHasPendingCloseWindow(false), mPendingCloseWindowId(CONTAINER_ID_NONE),
          mHasPendingOpenMainInventory(false) {}

void InventoryManager::attach(ServerPlayer *player, PacketSender *sender) {
    mPlayer = player;
    mSender = sender;
    mLastInventoryNetworkId = CONTAINER_ID_FIRST;
    mCurrentWindowType = ContainerType::Container;
    mMainInventoryWindowId = CONTAINER_ID_NONE;
    mCraftingTableWindowId = CONTAINER_ID_NONE;
    mCraftingTablePosition = Vector3i(0, 0, 0);
    mFurnaceWindowId = CONTAINER_ID_NONE;
    mFurnacePosition = Vector3i(0, 0, 0);
    mFurnaceKind = FurnaceKind::Furnace;
    mFurnaceContainerType = ContainerType::Furnace;
    mFurnaceBurnTime = 0;
    mFurnaceMaxBurnTime = 0;
    mFurnaceCookTime = 0;
    for (ItemStack &item: mFurnaceObservedItems)
        item = ItemStack::air();
    mClientSelectedHotbarSlot = -1;
    mHasPendingCloseWindow = false;
    mPendingCloseWindowId = CONTAINER_ID_NONE;
    mHasPendingOpenMainInventory = false;
}

int InventoryManager::_getNewWindowId() {
    int next = (mLastInventoryNetworkId + 1) % CONTAINER_ID_LAST;
    if (next < CONTAINER_ID_FIRST) {
        next = CONTAINER_ID_FIRST;
    }

    mLastInventoryNetworkId = next;
    return mLastInventoryNetworkId;
}

int InventoryManager::_getWindowId(InventoryId inventory) const {
    switch (inventory) {
        case InventoryId::Inventory:
            return CONTAINER_ID_INVENTORY;
        case InventoryId::Offhand:
            return CONTAINER_ID_OFFHAND;
        case InventoryId::Armor:
            return CONTAINER_ID_ARMOR;
        case InventoryId::Cursor:
            return CONTAINER_ID_UI;
        case InventoryId::CraftingInput:
            return CONTAINER_ID_UI;
        case InventoryId::CraftingTableInput:
            return CONTAINER_ID_UI;
        case InventoryId::FurnaceInput:
            return mFurnaceWindowId;
    }
    return CONTAINER_ID_NONE;
}

int InventoryManager::_getSize(InventoryId inventory) const {
    switch (inventory) {
        case InventoryId::Inventory:
            return PlayerInventory::CONTAINER_SIZE;
        case InventoryId::Armor:
            return PlayerInventory::ARMOR_SIZE;
        case InventoryId::Offhand:
        case InventoryId::Cursor:
            return 1;
        case InventoryId::CraftingInput:
            return PlayerInventory::CRAFTING_SIZE;
        case InventoryId::CraftingTableInput:
            return CraftingTableInventory::SIZE;
        case InventoryId::FurnaceInput:
            return FurnaceInventory::SIZE;
    }
    return 0;
}

void InventoryManager::_sendContentPackets(int containerId, InventoryId inventory) {
    const PlayerInventory &contents = mPlayer->getInventory();
    const int size = _getSize(inventory);

    InventoryContentPacket content;
    content.mContainerId = containerId;
    content.mContents.reserve((size_t) size);
    for (int slot = 0; slot < size; slot++) {
        content.mContents.push_back(itemAt(contents, inventory, slot));
    }
    content.mContainerNameData.mContainer = ContainerSlotType::AnvilInput;
    content.mStorageItem = ItemStack::air();
    mSender->sendPacketTo(mPlayer->getNetworkIdentifier(), content);
}

void InventoryManager::_sendSlotPackets(int containerId, int netSlot, InventoryId inventory, int slot) {
    InventorySlotPacket packet;
    packet.mContainerId = containerId;
    packet.mSlot = netSlot;
    packet.mItem = itemAt(mPlayer->getInventory(), inventory, slot);
    mSender->sendPacketTo(mPlayer->getNetworkIdentifier(), packet);
}

void InventoryManager::syncContents(InventoryId inventory) {
    if (mPlayer == nullptr || mSender == nullptr) {
        return;
    }

    if (inventory == InventoryId::Cursor) {
        _sendSlotPackets(CONTAINER_ID_UI, UI_SLOT_CURSOR, inventory, 0);
        return;
    }

    if (inventory == InventoryId::CraftingInput) {
        for (int slot = 0; slot < PlayerInventory::CRAFTING_SIZE; slot++) {
            _sendSlotPackets(CONTAINER_ID_UI, PlayerInventory::CRAFTING_NETWORK_SLOT_FIRST + slot, inventory, slot);
        }
        return;
    }

    if (inventory == InventoryId::CraftingTableInput) {
        if (!isCraftingTableOpen()) {
            return;
        }
        for (int slot = 0; slot < CraftingTableInventory::SIZE; slot++) {
            _sendSlotPackets(CONTAINER_ID_UI, CraftingTableInventory::NETWORK_SLOT_FIRST + slot, inventory, slot);
        }
        return;
    }

    if (inventory == InventoryId::FurnaceInput) {
        if (!isFurnaceOpen()) {
            return;
        }
        _sendContentPackets(mFurnaceWindowId, inventory);
        return;
    }

    _sendContentPackets(_getWindowId(inventory), inventory);
}

void InventoryManager::syncSlot(InventoryId inventory, int slot) {
    if (mPlayer == nullptr || mSender == nullptr) {
        return;
    }

    if (inventory == InventoryId::Cursor) {
        _sendSlotPackets(CONTAINER_ID_UI, UI_SLOT_CURSOR, inventory, 0);
        return;
    }

    if (inventory == InventoryId::CraftingInput) {
        if (slot < 0 || slot >= PlayerInventory::CRAFTING_SIZE) {
            return;
        }
        _sendSlotPackets(CONTAINER_ID_UI, PlayerInventory::CRAFTING_NETWORK_SLOT_FIRST + slot, inventory, slot);
        return;
    }

    if (inventory == InventoryId::CraftingTableInput) {
        if (!isCraftingTableOpen() || slot < 0 || slot >= CraftingTableInventory::SIZE) {
            return;
        }
        _sendSlotPackets(CONTAINER_ID_UI, CraftingTableInventory::NETWORK_SLOT_FIRST + slot, inventory, slot);
        return;
    }

    if (inventory == InventoryId::FurnaceInput) {
        if (!isFurnaceOpen() || slot < 0 || slot >= FurnaceInventory::SIZE) {
            return;
        }
        _sendSlotPackets(mFurnaceWindowId, slot, inventory, slot);
        return;
    }

    if (inventory == InventoryId::Offhand) {
        _sendContentPackets(CONTAINER_ID_OFFHAND, inventory);
        return;
    }

    _sendSlotPackets(_getWindowId(inventory), slot, inventory, slot);
}

void InventoryManager::syncAll() {
    if (mPlayer == nullptr || mSender == nullptr) {
        return;
    }

    syncContents(InventoryId::Inventory);
    syncContents(InventoryId::Offhand);
    syncContents(InventoryId::Armor);
    syncContents(InventoryId::CraftingInput);
    syncContents(InventoryId::Cursor);
    if (isFurnaceOpen()) {
        syncContents(InventoryId::FurnaceInput);
    }
}

void InventoryManager::_storeFurnaceState(bool clearLocal) {
    if (mPlayer == nullptr || !isFurnaceOpen()) {
        return;
    }

    const FurnaceKey key{mFurnacePosition.x, mFurnacePosition.y, mFurnacePosition.z};
    FurnaceStoredState &state = furnaceStates[key];
    state.mKind = mFurnaceKind;
    for (int slot = 0; slot < FurnaceInventory::SIZE; ++slot) {
        const ItemStack &local = mPlayer->getInventory().getFurnaceItem(slot);
        if (!furnaceItemsEqual(local, mFurnaceObservedItems[(size_t) slot]))
            state.mItems[(size_t) slot] = local;
        mFurnaceObservedItems[(size_t) slot] = local;
        if (clearLocal) {
            mPlayer->getInventory().setFurnaceItem(slot, ItemStack::air());
            mFurnaceObservedItems[(size_t) slot] = ItemStack::air();
        }
    }
}

namespace {
    bool isWoodLike(const std::string &id) {
        return id == "minecraft:wood" || id == "minecraft:crimson_stem" || id == "minecraft:warped_stem"
               || id == "minecraft:crimson_hyphae" || id == "minecraft:warped_hyphae"
               || id.find("oak_") != std::string::npos || id.find("spruce_") != std::string::npos
               || id.find("birch_") != std::string::npos || id.find("jungle_") != std::string::npos
               || id.find("acacia_") != std::string::npos || id.find("dark_oak_") != std::string::npos
               || id.find("mangrove_") != std::string::npos || id.find("cherry_") != std::string::npos
               || id.find("bamboo_") != std::string::npos || id.find("crimson_") != std::string::npos
               || id.find("warped_") != std::string::npos || id.find("pale_oak_") != std::string::npos
               || id.find("poplar_") != std::string::npos;
    }

    int fuelTime(const ItemStack &item) {
        if (item.isAir() || item.mDefinition == nullptr) {
            return 0;
        }
        const std::string id = item.mDefinition->getIdentifier();
        if (id == "minecraft:lava_bucket") return 20000;
        if (id == "minecraft:coal_block") return 16000;
        if (id == "minecraft:coal" || id == "minecraft:charcoal") return 1600;
        if (id == "minecraft:blaze_rod") return 2400;
        if (id == "minecraft:dried_kelp_block") return 4000;
        if (id == "minecraft:bamboo") return 50;
        if (id == "minecraft:stick") return 100;
        if (id == "minecraft:bowl" || id == "minecraft:bow") return 200;
        if (id.find("_boat") != std::string::npos) return 1200;
        if (id == "minecraft:wooden_sword" || id == "minecraft:wooden_pickaxe"
            || id == "minecraft:wooden_axe" || id == "minecraft:wooden_shovel"
            || id == "minecraft:wooden_hoe") return 200;
        if (isWoodLike(id) && id.find("_slab") != std::string::npos) return 300;
        if (isWoodLike(id) && (id.find("_planks") != std::string::npos || id.find("_log") != std::string::npos
            || id.find("_wood") != std::string::npos || id.find("_stem") != std::string::npos
            || id.find("_hyphae") != std::string::npos || id.find("_mosaic") != std::string::npos
            || id == "minecraft:wood")) return 300;
        if (isWoodLike(id) && id.find("_sign") != std::string::npos) return 200;
        if (isWoodLike(id) && id.find("_door") != std::string::npos) return 200;
        if (isWoodLike(id) && id.find("_button") != std::string::npos) return 100;
        if (isWoodLike(id) && (id.find("_trapdoor") != std::string::npos
            || id.find("_fence") != std::string::npos || id.find("_pressure_plate") != std::string::npos
            || id.find("_stairs") != std::string::npos)) return 300;
        if (id.find("wooden_") != std::string::npos) return 200;
        if (id.find("_sapling") != std::string::npos) return 100;
        if (id == "minecraft:bookshelf" || id == "minecraft:crafting_table" || id == "minecraft:cartography_table"
            || id == "minecraft:fletching_table" || id == "minecraft:smithing_table" || id == "minecraft:chest"
            || id == "minecraft:trapped_chest" || id == "minecraft:barrel" || id == "minecraft:jukebox"
            || id == "minecraft:note_block" || id == "minecraft:daylight_detector"
            || id == "minecraft:banner" || id.find("_banner") != std::string::npos
            || id.find("_hanging_sign") != std::string::npos) return 300;
        return 0;
    }

    bool stackMatches(const ItemStack &left, const ItemStack &right) {
        return !left.isAir() && !right.isAir() && PlayerInventory::canStack(left, right);
    }

    std::string litIdentifier(const std::string &identifier, bool lit) {
        if (identifier == "minecraft:furnace" || identifier == "minecraft:lit_furnace") {
            return lit ? "minecraft:lit_furnace" : "minecraft:furnace";
        }
        if (identifier == "minecraft:blast_furnace" || identifier == "minecraft:lit_blast_furnace") {
            return lit ? "minecraft:lit_blast_furnace" : "minecraft:blast_furnace";
        }
        if (identifier == "minecraft:smoker" || identifier == "minecraft:lit_smoker") {
            return lit ? "minecraft:lit_smoker" : "minecraft:smoker";
        }
        return identifier;
    }

    void updateLitState(ServerNetworkHandler &owner, const Vector3i &position, bool lit) {
        const BlockState state = owner.getLevel().getBlockState(position.x, position.y, position.z);
        const std::string identifier = litIdentifier(state.mName, lit);
        if (identifier == state.mName) {
            return;
        }
        const BlockState updated(identifier, state.mStates);
        owner.getLevel().setBlockState(position.x, position.y, position.z, updated);
        UpdateBlockPacket packet;
        packet.mBlockPosition = position;
        packet.mRuntimeId = (uint32_t) BlockStateHasher::hash(updated.mName, updated.mStates);
        packet.mFlags = UpdateBlockPacket::Flag::All;
        packet.mDataLayer = 0;
        BlockActionHandler::broadcastToViewers(owner,
                                               Vector3f((float) position.x + 0.5f,
                                                        (float) position.y + 0.5f,
                                                        (float) position.z + 0.5f),
                                               packet);
    }

    const FurnaceRecipeData *findFurnaceRecipe(const ItemStack &input) {
        if (input.isAir() || input.mDefinition == nullptr) {
            return nullptr;
        }

        const FurnaceRecipeData *recipes = CraftingRecipeTable::getFurnaceRecipes();
        const size_t recipeCount = CraftingRecipeTable::getFurnaceRecipeCount();
        for (size_t index = 0; index < recipeCount; ++index) {
            const FurnaceRecipeData &candidate = recipes[index];
            if (candidate.mInputItemId == nullptr
                || std::string(input.mDefinition->getIdentifier()) != candidate.mInputItemId
                || (candidate.mInputAuxValue >= 0 && input.mDamage != candidate.mInputAuxValue)
                || input.mCount < candidate.mInputCount) {
                continue;
            }
            return &candidate;
        }
        return nullptr;
    }

    bool sameFurnaceItem(const ItemStack &left, const ItemStack &right) {
        return furnaceItemsEqual(left, right);
    }

    bool tickFurnaceState(ServerNetworkHandler &owner, const Vector3i &position, FurnaceStoredState &state) {
        if (!FurnaceBlock::matches(owner.getLevel().getBlockState(position.x, position.y, position.z))) {
            state.mBurnTime = 0;
            state.mCookTime = 0;
            state.mMaxBurnTime = 0;
            return false;
        }

        const FurnaceRecipeData *recipe = findFurnaceRecipe(state.mItems[FurnaceInventory::SLOT_INPUT]);
        ItemStack result = ItemStack::air();
        if (recipe != nullptr) {
            result.mDefinition = owner.getItemDefinitions().getDefinition(recipe->mOutputItemId);
            result.mBlockDefinition = owner.getBlockDefinitions().getDefinition(recipe->mOutputItemId);
            result.mDamage = 0;
            result.mCount = recipe->mOutputCount;
        }

        const ItemStack &input = state.mItems[FurnaceInventory::SLOT_INPUT];
        const ItemStack &currentOutput = state.mItems[FurnaceInventory::SLOT_OUTPUT];
        const bool canSmelt = recipe != nullptr && !result.isAir() && (currentOutput.isAir()
                             || (stackMatches(currentOutput, result)
                                 && currentOutput.mCount + result.mCount
                                    <= PlayerInventory::getMaxStackSize(currentOutput)));
        const int previousBurn = state.mBurnTime;
        const int previousCook = state.mCookTime;
        const int previousMaxBurn = state.mMaxBurnTime;
        const std::array<ItemStack, FurnaceInventory::SIZE> previousItems = state.mItems;

        if (state.mBurnTime <= 0 && canSmelt) {
            ItemStack fuel = state.mItems[FurnaceInventory::SLOT_FUEL];
            const int duration = fuelTime(fuel);
            if (duration > 0 && fuel.mCount > 0) {
                state.mBurnTime = duration;
                state.mMaxBurnTime = duration;
                --fuel.mCount;
                if (fuel.mCount <= 0) {
                    if (fuel.mDefinition != nullptr
                        && fuel.mDefinition->getIdentifier() == "minecraft:lava_bucket") {
                        fuel = ItemStack::air();
                        fuel.mDefinition = owner.getItemDefinitions().getDefinition("minecraft:bucket");
                        fuel.mBlockDefinition = owner.getBlockDefinitions().getDefinition("minecraft:bucket");
                        fuel.mCount = fuel.mDefinition == nullptr ? 0 : 1;
                    } else {
                        fuel = ItemStack::air();
                    }
                }
                state.mItems[FurnaceInventory::SLOT_FUEL] = std::move(fuel);
            }
        }

        if (state.mBurnTime > 0) {
            --state.mBurnTime;
            if (canSmelt) {
                ++state.mCookTime;
                const int duration = FurnaceInventory::cookDuration(state.mKind);
                if (state.mCookTime >= duration) {
                    ItemStack produced = result;
                    if (!currentOutput.isAir())
                        produced.mCount += currentOutput.mCount;
                    state.mItems[FurnaceInventory::SLOT_OUTPUT] = std::move(produced);

                    ItemStack consumed = input;
                    consumed.mCount -= recipe->mInputCount;
                    if (consumed.mCount <= 0)
                        consumed = ItemStack::air();
                    state.mItems[FurnaceInventory::SLOT_INPUT] = std::move(consumed);
                    state.mCookTime -= duration;
                }
            } else if (state.mBurnTime <= 0) {
                state.mBurnTime = 0;
                state.mCookTime = 0;
                state.mMaxBurnTime = 0;
            } else {
                state.mCookTime = 0;
            }
        } else {
            state.mBurnTime = 0;
            state.mCookTime = 0;
            state.mMaxBurnTime = 0;
        }

        const std::string currentIdentifier = owner.getLevel().getBlockState(position.x, position.y,
                                                                              position.z).mName;
        const bool isLit = currentIdentifier == "minecraft:lit_furnace"
                           || currentIdentifier == "minecraft:lit_blast_furnace"
                           || currentIdentifier == "minecraft:lit_smoker";
        if (state.mBurnTime > 0 && (previousBurn == 0 || !isLit))
            updateLitState(owner, position, true);
        else if (state.mBurnTime == 0 && isLit)
            updateLitState(owner, position, false);

        if (previousBurn == state.mBurnTime && previousCook == state.mCookTime
            && previousMaxBurn == state.mMaxBurnTime) {
            for (int slot = 0; slot < FurnaceInventory::SIZE; ++slot) {
                if (!sameFurnaceItem(previousItems[(size_t) slot], state.mItems[(size_t) slot]))
                    return true;
            }
            return false;
        }
        return true;
    }
}

void InventoryManager::tickFurnace(ServerNetworkHandler &owner) {
    if (mPlayer == nullptr || !isFurnaceOpen()) {
        return;
    }

    if (!FurnaceBlock::matches(owner.getLevel().getBlockState(mFurnacePosition.x,
                                                               mFurnacePosition.y,
                                                               mFurnacePosition.z))) {
        onClientRemoveWindow(mFurnaceWindowId);
        return;
    }

    _storeFurnaceState(false);
    const FurnaceKey key{mFurnacePosition.x, mFurnacePosition.y, mFurnacePosition.z};
    FurnaceStoredState &state = furnaceStates[key];
    PlayerInventory &inventory = mPlayer->getInventory();
    const int previousBurn = mFurnaceBurnTime;
    const int previousMaxBurn = mFurnaceMaxBurnTime;
    const int previousCook = mFurnaceCookTime;
    const std::array<ItemStack, FurnaceInventory::SIZE> previousItems = {
        inventory.getFurnaceItem(FurnaceInventory::SLOT_INPUT),
        inventory.getFurnaceItem(FurnaceInventory::SLOT_FUEL),
        inventory.getFurnaceItem(FurnaceInventory::SLOT_OUTPUT)
    };
    const auto lastTick = furnaceLastTick.find(key);
    const bool processTick = lastTick == furnaceLastTick.end() || lastTick->second != owner.getCurrentTick();
    if (processTick) {
        tickFurnaceState(owner, mFurnacePosition, state);
        furnaceLastTick[key] = owner.getCurrentTick();
    }
    mFurnaceBurnTime = state.mBurnTime;
    mFurnaceMaxBurnTime = state.mMaxBurnTime;
    mFurnaceCookTime = state.mCookTime;
    for (int slot = 0; slot < FurnaceInventory::SIZE; ++slot) {
        if (!sameFurnaceItem(previousItems[(size_t) slot], state.mItems[(size_t) slot]))
            inventory.setFurnaceItem(slot, state.mItems[(size_t) slot]);
        mFurnaceObservedItems[(size_t) slot] = inventory.getFurnaceItem(slot);
    }
    if (!sameFurnaceItem(previousItems[FurnaceInventory::SLOT_INPUT],
                         inventory.getFurnaceItem(FurnaceInventory::SLOT_INPUT))) {
        syncSlot(InventoryId::FurnaceInput, FurnaceInventory::SLOT_INPUT);
    }
    if (!sameFurnaceItem(previousItems[FurnaceInventory::SLOT_FUEL],
                         inventory.getFurnaceItem(FurnaceInventory::SLOT_FUEL))) {
        syncSlot(InventoryId::FurnaceInput, FurnaceInventory::SLOT_FUEL);
    }
    if (!sameFurnaceItem(previousItems[FurnaceInventory::SLOT_OUTPUT],
                         inventory.getFurnaceItem(FurnaceInventory::SLOT_OUTPUT))) {
        syncSlot(InventoryId::FurnaceInput, FurnaceInventory::SLOT_OUTPUT);
    }
    if (previousCook != mFurnaceCookTime) {
        ContainerSetDataPacket packet;
        packet.mWindowId = (int8_t) mFurnaceWindowId;
        packet.mProperty = 0;
        packet.mValue = mFurnaceCookTime;
        mSender->sendPacketTo(mPlayer->getNetworkIdentifier(), packet);
    }
    if (previousBurn != mFurnaceBurnTime) {
        ContainerSetDataPacket packet;
        packet.mWindowId = (int8_t) mFurnaceWindowId;
        packet.mProperty = 1;
        packet.mValue = mFurnaceBurnTime;
        mSender->sendPacketTo(mPlayer->getNetworkIdentifier(), packet);
    }
    if (previousMaxBurn != mFurnaceMaxBurnTime) {
        ContainerSetDataPacket packet;
        packet.mWindowId = (int8_t) mFurnaceWindowId;
        packet.mProperty = 2;
        packet.mValue = mFurnaceMaxBurnTime;
        mSender->sendPacketTo(mPlayer->getNetworkIdentifier(), packet);
    }

    _storeFurnaceState(false);
}

void InventoryManager::tickStoredFurnaces(ServerNetworkHandler &owner) {
    std::unordered_map<FurnaceKey, bool, FurnaceKeyHash> openPositions;
    for (auto &entry: owner.getPlayers()) {
        ServerPlayer &player = entry.second;
        if (!player.getInventoryManager().isFurnaceOpen())
            continue;
        const Vector3i &position = player.getInventoryManager().getFurnacePosition();
        openPositions[FurnaceKey{position.x, position.y, position.z}] = true;
    }

    for (auto &entry: furnaceStates) {
        if (openPositions.find(entry.first) != openPositions.end())
            continue;
        const FurnaceKey &key = entry.first;
        tickFurnaceState(owner, Vector3i(key.x, key.y, key.z), entry.second);
    }
}

void InventoryManager::onFurnaceBroken(ServerNetworkHandler &owner, const Vector3i &position) {
    for (auto &entry: owner.getPlayers()) {
        InventoryManager &manager = entry.second.getInventoryManager();
        if (manager.isFurnaceOpen() && manager.getFurnacePosition() == position)
            manager.onClientRemoveWindow(manager.getFurnaceWindowId());
    }

    const FurnaceKey key{position.x, position.y, position.z};
    const auto it = furnaceStates.find(key);
    if (it == furnaceStates.end())
        return;

    const Vector3f dropPosition((float) position.x + 0.5f, (float) position.y + 0.5f,
                                (float) position.z + 0.5f);
    for (const ItemStack &item: it->second.mItems) {
        if (!item.isAir() && item.mCount > 0)
            owner.dropItem(dropPosition, item, Vector3f(0.0f, 0.2f, 0.0f), ItemActor::DEFAULT_PICKUP_DELAY);
    }
    furnaceStates.erase(it);
    furnaceLastTick.erase(key);
}

void InventoryManager::_sendOutputPacket(const ItemStack &item) {
    InventorySlotPacket packet;
    packet.mContainerId = CONTAINER_ID_UI;
    packet.mSlot = CraftingTableInventory::CREATED_OUTPUT_NETWORK_SLOT;
    packet.mItem = item;
    mSender->sendPacketTo(mPlayer->getNetworkIdentifier(), packet);
}

void InventoryManager::syncCraftingTableState(const std::vector<ItemStack> &recipeOutputs,
                                               const std::vector<uint32_t> &recipeSourceIndices) {
    if (mPlayer == nullptr || mSender == nullptr || !isCraftingTableOpen()) {
        return;
    }

    syncContents(InventoryId::CraftingTableInput);
    const ItemStack result = CraftingManager::findResult(mPlayer->getInventory().getCraftingTableContents(), 3,
                                                         recipeOutputs, recipeSourceIndices);
    _sendOutputPacket(result);
}

void InventoryManager::syncCraftingPreview(const std::vector<ItemStack> &recipeOutputs,
                                            const std::vector<uint32_t> &recipeSourceIndices) {
    if (mPlayer == nullptr || mSender == nullptr) {
        return;
    }

    const std::vector<ItemStack> &contents = isCraftingTableOpen()
                                             ? mPlayer->getInventory().getCraftingTableContents()
                                             : mPlayer->getInventory().getCraftingContents();
    const int gridWidth = isCraftingTableOpen() ? 3 : 2;
    const ItemStack result = CraftingManager::findResult(contents, gridWidth,
                                                         recipeOutputs, recipeSourceIndices);
    _sendOutputPacket(result);
}

void InventoryManager::onClientSelectHotbarSlot(int slot) {
    mClientSelectedHotbarSlot = slot;
}

void InventoryManager::syncSelectedHotbarSlot() {
    if (mPlayer == nullptr || mSender == nullptr) {
        return;
    }

    const PlayerInventory &inventory = mPlayer->getInventory();
    const int selected = inventory.getSelectedSlot();
    if (selected == mClientSelectedHotbarSlot) {
        return;
    }

    MobEquipmentPacket equipment;
    equipment.mRuntimeActorId = (int64_t) mPlayer->getRuntimeId();
    equipment.mItem = inventory.getItemInHand();
    equipment.mInventorySlot = selected;
    equipment.mHotbarSlot = selected;
    equipment.mContainerId = CONTAINER_ID_INVENTORY;
    mSender->sendPacketTo(mPlayer->getNetworkIdentifier(), equipment);

    mClientSelectedHotbarSlot = selected;
}

void InventoryManager::onCurrentWindowRemove() {
    if (mPlayer == nullptr || mSender == nullptr) {
        return;
    }

    const int currentWindowId = mFurnaceWindowId != CONTAINER_ID_NONE
                                ? mFurnaceWindowId
                                : (mCraftingTableWindowId != CONTAINER_ID_NONE ? mCraftingTableWindowId : mMainInventoryWindowId);
    if (currentWindowId == CONTAINER_ID_NONE) {
        return;
    }

    ContainerClosePacket close;
    close.mWindowId = (int8_t) currentWindowId;
    close.mType = mCurrentWindowType;
    close.mServerInitiated = true;
    mSender->sendPacketTo(mPlayer->getNetworkIdentifier(), close);

    mPendingCloseWindowId = currentWindowId;
    mHasPendingCloseWindow = true;
    if (mCraftingTableWindowId == currentWindowId) {
        mCraftingTableWindowId = CONTAINER_ID_NONE;
    } else if (mFurnaceWindowId == currentWindowId) {
        _storeFurnaceState(true);
        mFurnaceWindowId = CONTAINER_ID_NONE;
        mFurnaceBurnTime = 0;
        mFurnaceMaxBurnTime = 0;
        mFurnaceCookTime = 0;
        for (ItemStack &item: mFurnaceObservedItems)
            item = ItemStack::air();
    } else {
        mMainInventoryWindowId = CONTAINER_ID_NONE;
    }
}

bool InventoryManager::onClientOpenCraftingTable(const Vector3i &position) {
    if (mPlayer == nullptr || mSender == nullptr) {
        return false;
    }

    if (mCraftingTableWindowId != CONTAINER_ID_NONE) {
        return mCraftingTablePosition == position;
    }

    if (mMainInventoryWindowId != CONTAINER_ID_NONE || mFurnaceWindowId != CONTAINER_ID_NONE || mHasPendingCloseWindow) {
        return false;
    }

    const int windowId = _getNewWindowId();
    mCraftingTableWindowId = windowId;
    mCraftingTablePosition = position;
    mCurrentWindowType = ContainerType::Workbench;

    ContainerOpenPacket open;
    open.mWindowId = (int8_t) windowId;
    open.mType = ContainerType::Workbench;
    open.mBlockPosition = position;
    open.mUniqueActorId = -1;
    mSender->sendPacketTo(mPlayer->getNetworkIdentifier(), open);
    return true;
}

bool InventoryManager::onClientOpenFurnace(const Vector3i &position, FurnaceKind kind, ContainerType type) {
    if (mPlayer == nullptr || mSender == nullptr) {
        return false;
    }
    if (mFurnaceWindowId != CONTAINER_ID_NONE) {
        return mFurnacePosition == position;
    }
    if (mMainInventoryWindowId != CONTAINER_ID_NONE || mCraftingTableWindowId != CONTAINER_ID_NONE
        || mHasPendingCloseWindow) {
        return false;
    }

    const int windowId = _getNewWindowId();
    mFurnaceWindowId = windowId;
    mFurnacePosition = position;
    mFurnaceKind = kind;
    mFurnaceContainerType = type;
    mFurnaceBurnTime = 0;
    mFurnaceMaxBurnTime = 0;
    mFurnaceCookTime = 0;
    const FurnaceKey key{position.x, position.y, position.z};
    FurnaceStoredState &state = furnaceStates[key];
    state.mKind = kind;
    mFurnaceBurnTime = state.mBurnTime;
    mFurnaceMaxBurnTime = state.mMaxBurnTime;
    mFurnaceCookTime = state.mCookTime;
    for (int slot = 0; slot < FurnaceInventory::SIZE; ++slot) {
        mPlayer->getInventory().setFurnaceItem(slot, state.mItems[(size_t) slot]);
        mFurnaceObservedItems[(size_t) slot] = state.mItems[(size_t) slot];
    }
    mCurrentWindowType = type;

    ContainerOpenPacket open;
    open.mWindowId = (int8_t) windowId;
    open.mType = type;
    open.mBlockPosition = position;
    open.mUniqueActorId = -1;
    mSender->sendPacketTo(mPlayer->getNetworkIdentifier(), open);
    syncContents(InventoryId::FurnaceInput);
    return true;
}

void InventoryManager::onClientOpenMainInventory() {
    if (mPlayer == nullptr || mSender == nullptr) {
        return;
    }

    if (mMainInventoryWindowId != CONTAINER_ID_NONE) {
        return;
    }

    if (mCraftingTableWindowId != CONTAINER_ID_NONE || mFurnaceWindowId != CONTAINER_ID_NONE) {
        mHasPendingOpenMainInventory = true;
        onCurrentWindowRemove();
        return;
    }

    if (mHasPendingCloseWindow) {
        mHasPendingOpenMainInventory = true;
        return;
    }

    const int windowId = _getNewWindowId();
    mMainInventoryWindowId = windowId;
    mCurrentWindowType = ContainerType::Inventory;

    ContainerOpenPacket open;
    open.mWindowId = (int8_t) windowId;
    open.mType = mCurrentWindowType;
    open.mBlockPosition = Vector3i(0, 0, 0);
    open.mUniqueActorId = mPlayer->getUniqueId();
    mSender->sendPacketTo(mPlayer->getNetworkIdentifier(), open);
}

void InventoryManager::onClientRemoveWindow(int windowId) {
    if (mPlayer == nullptr || mSender == nullptr) {
        return;
    }

    if (windowId == CONTAINER_ID_NONE) {
        windowId = mLastInventoryNetworkId;
    }

    if (windowId == mMainInventoryWindowId) {
        mMainInventoryWindowId = CONTAINER_ID_NONE;
    }

    if (windowId == mCraftingTableWindowId) {
        mCraftingTableWindowId = CONTAINER_ID_NONE;
    }
    if (windowId == mFurnaceWindowId) {
        _storeFurnaceState(true);
        mFurnaceWindowId = CONTAINER_ID_NONE;
        mFurnaceBurnTime = 0;
        mFurnaceMaxBurnTime = 0;
        mFurnaceCookTime = 0;
        for (ItemStack &item: mFurnaceObservedItems)
            item = ItemStack::air();
    }

    ContainerClosePacket close;
    close.mWindowId = (int8_t) windowId;
    close.mType = mCurrentWindowType;
    close.mServerInitiated = false;
    mSender->sendPacketTo(mPlayer->getNetworkIdentifier(), close);

    if (mHasPendingCloseWindow && mPendingCloseWindowId == windowId) {
        mHasPendingCloseWindow = false;
        mPendingCloseWindowId = CONTAINER_ID_NONE;

        if (mHasPendingOpenMainInventory) {
            mHasPendingOpenMainInventory = false;
            onClientOpenMainInventory();
        }
    }
}

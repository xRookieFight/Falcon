#include "Protocol/ItemCodec.h"

#include "Core/NBT/NbtIo.h"

namespace {
    const std::string BLOCKING_ID = "minecraft:shield";

    Tag networkItemTag(const ItemStack &item) {
        if (item.mDamage <= 0)
            return item.mTag;

        Tag tag = item.mTag.isCompound() ? item.mTag : Tag::ofCompound();
        tag.putInt("Damage", item.mDamage);
        return tag;
    }

    void applyNetworkDamage(ItemStack &item) {
        if (item.mTag.isCompound() && item.mTag.get("Damage") != nullptr)
            item.mDamage = item.mTag.getInt("Damage", item.mDamage);
    }
}

void ItemCodec::writeItemInstance(BinaryStream &stream, const PacketCodecContext &context, const ItemStack &item) {
    if (item.isAir()) {
        stream.putVarInt(0);
        stream.putLShort(0);
        stream.putUnsignedVarInt((uint32_t) item.mDamage);
        stream.putVarInt(0);
        stream.putUnsignedVarInt((uint32_t) item.mDamage);
        return;
    }

    stream.putVarInt(item.mDefinition->getRuntimeId());
    stream.putLShort((uint16_t) item.mCount);
    stream.putUnsignedVarInt((uint32_t) item.mDamage);
    stream.putVarInt(item.mBlockDefinition == nullptr ? 0 : item.mBlockDefinition->getRuntimeId());

    const Tag networkTag = networkItemTag(item);

    BinaryStream userData;
    if (networkTag.getType() != Tag::Type::End) {
        userData.putLShort(0xffff);
        userData.putByte(1);
        NbtIo::writeTag(userData, networkTag, NbtVariant::LittleEndian);
    } else {
        userData.putLShort(0);
    }

    userData.putLInt((uint32_t) item.mCanPlace.size());
    for (const std::string &entry: item.mCanPlace) {
        userData.putLShort((uint16_t) entry.size());
        userData.put(entry);
    }

    userData.putLInt((uint32_t) item.mCanBreak.size());
    for (const std::string &entry: item.mCanBreak) {
        userData.putLShort((uint16_t) entry.size());
        userData.put(entry);
    }

    if (item.mDefinition->getIdentifier() == BLOCKING_ID) {
        userData.putLLong((uint64_t) item.mBlockingTicks);
    }

    stream.putUnsignedVarInt((uint32_t) userData.size());
    stream.put(userData.getBuffer());
}

ItemStack ItemCodec::readItemInstance(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    int runtimeId = stream.getVarInt();
    if (runtimeId == 0) {
        stream.getLShort();
        stream.getUnsignedVarInt();
        stream.getVarInt();
        stream.getUnsignedVarInt();
        return ItemStack::air();
    }

    ItemStack item;
    item.mDefinition = context.getItemDefinitions().getDefinition(runtimeId);
    item.mCount = stream.getLShort();
    item.mDamage = (int) stream.getUnsignedVarInt();

    int blockRuntimeId = stream.getVarInt();
    if (blockRuntimeId != 0) {
        item.mBlockDefinition = context.getBlockDefinitions().getDefinition(blockRuntimeId);
    }

    uint32_t userDataLength = stream.getUnsignedVarInt();
    std::string userDataBytes = stream.get(userDataLength);
    ReadOnlyBinaryStream userData(userDataBytes);

    uint16_t nbtSize = userData.getLShort();
    if (nbtSize == 0xffff) {
        unsigned char tagCount = userData.getByte();
        if (tagCount != 1) {
            throw BinaryDataException("Expected 1 tag but got " + std::to_string((int) tagCount));
        }
        item.mTag = NbtIo::readTag(userData, NbtVariant::LittleEndian);
    } else if (nbtSize > 0) {
        item.mTag = NbtIo::readTag(userData, NbtVariant::LittleEndian);
    }

    uint32_t canPlaceLength = userData.getLInt();
    item.mCanPlace.reserve(canPlaceLength);
    for (uint32_t i = 0; i < canPlaceLength; i++) {
        item.mCanPlace.push_back(userData.get(userData.getLShort()));
    }

    uint32_t canBreakLength = userData.getLInt();
    item.mCanBreak.reserve(canBreakLength);
    for (uint32_t i = 0; i < canBreakLength; i++) {
        item.mCanBreak.push_back(userData.get(userData.getLShort()));
    }

    if (item.mDefinition != nullptr && item.mDefinition->getIdentifier() == BLOCKING_ID
            && userData.getRemainingLength() >= 8) {
        item.mBlockingTicks = userData.getLLong();
    }

    applyNetworkDamage(item);
    return item;
}

void ItemCodec::writeNetworkItemStackDescriptor(BinaryStream &stream, const PacketCodecContext &context, const ItemStack &item) {
    if (item.isAir()) {
        stream.putLShort(0);
        stream.putLShort(0);
        stream.putUnsignedVarInt((uint32_t) item.mDamage);
        stream.putBool(false);
        stream.putUnsignedVarInt((uint32_t) item.mDamage);
        stream.putUnsignedVarInt((uint32_t) item.mDamage);
        return;
    }

    stream.putLShort((uint16_t) item.mDefinition->getRuntimeId());
    stream.putLShort((uint16_t) item.mCount);
    stream.putUnsignedVarInt((uint32_t) item.mDamage);

    stream.putBool(item.mUsingNetId);
    if (item.mUsingNetId) {
        stream.putVarInt(item.mNetId);
    }

    stream.putUnsignedVarInt(item.mBlockDefinition == nullptr ? 0 : (uint32_t) item.mBlockDefinition->getRuntimeId());

    const Tag networkTag = networkItemTag(item);

    BinaryStream userData;
    if (networkTag.getType() != Tag::Type::End) {
        userData.putLShort(0xffff);
        userData.putByte(1);
        NbtIo::writeTag(userData, networkTag, NbtVariant::LittleEndian);
    } else {
        userData.putLShort(0);
    }

    userData.putLInt((uint32_t) item.mCanPlace.size());
    for (const std::string &entry: item.mCanPlace) {
        userData.putLShort((uint16_t) entry.size());
        userData.put(entry);
    }

    userData.putLInt((uint32_t) item.mCanBreak.size());
    for (const std::string &entry: item.mCanBreak) {
        userData.putLShort((uint16_t) entry.size());
        userData.put(entry);
    }

    if (item.mDefinition->getIdentifier() == BLOCKING_ID) {
        userData.putLLong((uint64_t) item.mBlockingTicks);
    }

    stream.putUnsignedVarInt((uint32_t) userData.size());
    stream.put(userData.getBuffer());
}

ItemStack ItemCodec::readNetworkItemStackDescriptor(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    int runtimeId = (int16_t) stream.getLShort();
    if (runtimeId == 0) {
        stream.getLShort();
        stream.getUnsignedVarInt();
        stream.getBool();
        stream.getUnsignedVarInt();
        stream.getUnsignedVarInt();
        return ItemStack::air();
    }

    ItemStack item;
    item.mDefinition = context.getItemDefinitions().getDefinition(runtimeId);
    item.mCount = stream.getLShort();
    item.mDamage = (int) stream.getUnsignedVarInt();

    item.mUsingNetId = stream.getBool();
    if (item.mUsingNetId) {
        item.mNetId = stream.getVarInt();
    }

    int blockRuntimeId = (int) stream.getUnsignedVarInt();
    if (blockRuntimeId != 0) {
        item.mBlockDefinition = context.getBlockDefinitions().getDefinition(blockRuntimeId);
    }

    uint32_t userDataLength = stream.getUnsignedVarInt();
    std::string userDataBytes = stream.get(userDataLength);
    ReadOnlyBinaryStream userData(userDataBytes);

    uint16_t nbtSize = userData.getLShort();
    if (nbtSize == 0xffff) {
        unsigned char tagCount = userData.getByte();
        if (tagCount != 1) {
            throw BinaryDataException("Expected 1 tag but got " + std::to_string((int) tagCount));
        }
        item.mTag = NbtIo::readTag(userData, NbtVariant::LittleEndian);
    } else if (nbtSize > 0) {
        item.mTag = NbtIo::readTag(userData, NbtVariant::LittleEndian);
    }

    uint32_t canPlaceLength = userData.getLInt();
    item.mCanPlace.reserve(canPlaceLength);
    for (uint32_t i = 0; i < canPlaceLength; i++) {
        item.mCanPlace.push_back(userData.get(userData.getLShort()));
    }

    uint32_t canBreakLength = userData.getLInt();
    item.mCanBreak.reserve(canBreakLength);
    for (uint32_t i = 0; i < canBreakLength; i++) {
        item.mCanBreak.push_back(userData.get(userData.getLShort()));
    }

    if (item.mDefinition != nullptr && item.mDefinition->getIdentifier() == BLOCKING_ID
            && userData.getRemainingLength() >= 8) {
        item.mBlockingTicks = userData.getLLong();
    }

    applyNetworkDamage(item);
    return item;
}

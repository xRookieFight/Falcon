#include "Protocol/Packets/ItemStackResponsePacket.h"

#include "Protocol/NetworkPacketHandler.h"
#include "Protocol/Types/ContainerSlotType.h"
#include "Protocol/Types/FullContainerName.h"

namespace {
    void writeFullContainerName(BinaryStream &stream, const FullContainerName &name) {
        stream.putByte((unsigned char) containerSlotTypeToId(name.mContainer));
        stream.putOptionalPresent(name.mHasDynamicId);
        if (name.mHasDynamicId) {
            stream.putLInt((uint32_t) name.mDynamicId);
        }
    }

    FullContainerName readFullContainerName(ReadOnlyBinaryStream &stream) {
        FullContainerName name;
        name.mContainer = containerSlotTypeFromId(stream.getByte());
        name.mHasDynamicId = stream.getOptionalPresent();
        if (name.mHasDynamicId) {
            name.mDynamicId = (int32_t) stream.getLInt();
        }
        return name;
    }

    void writeItemEntry(BinaryStream &stream, const ItemStackResponseSlot &entry) {
        stream.putByte((unsigned char) entry.mSlot);
        stream.putByte((unsigned char) entry.mHotbarSlot);
        stream.putByte((unsigned char) entry.mCount);

        stream.putBool(true);
        bool hasStackNetworkId = entry.mStackNetworkId > 0;
        stream.putBool(hasStackNetworkId);
        if (hasStackNetworkId) {
            stream.putVarInt(entry.mStackNetworkId);
        }

        stream.putString(entry.mCustomName);
        stream.putString(entry.mFilteredCustomName);
        stream.putVarInt(entry.mDurabilityCorrection);
    }

    ItemStackResponseSlot readItemEntry(ReadOnlyBinaryStream &stream) {
        ItemStackResponseSlot entry;
        entry.mSlot = stream.getByte();
        entry.mHotbarSlot = stream.getByte();
        entry.mCount = stream.getByte();

        bool hasStackNetworkId = stream.getBool() && stream.getBool();
        entry.mStackNetworkId = hasStackNetworkId ? stream.getVarInt() : 0;

        entry.mCustomName = stream.getString();
        entry.mFilteredCustomName = stream.getString();
        entry.mDurabilityCorrection = stream.getVarInt();
        return entry;
    }

    void writeItemStackResponseContainer(BinaryStream &stream, const ItemStackResponseContainer &container) {
        writeFullContainerName(stream, container.mContainerName);
        stream.putArrayLength((uint32_t) container.mItems.size());
        for (const ItemStackResponseSlot &item: container.mItems) {
            writeItemEntry(stream, item);
        }
    }

    ItemStackResponseContainer readItemStackResponseContainer(ReadOnlyBinaryStream &stream) {
        ItemStackResponseContainer container;
        container.mContainerName = readFullContainerName(stream);

        uint32_t count = stream.getArrayLength();
        container.mItems.reserve(count);
        for (uint32_t i = 0; i < count; i++) {
            container.mItems.push_back(readItemEntry(stream));
        }

        return container;
    }
}

ItemStackResponsePacket::ItemStackResponsePacket() = default;

void ItemStackResponsePacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    stream.putArrayLength((uint32_t) mEntries.size());
    for (const ItemStackResponseEntry &entry: mEntries) {
        stream.putByte((unsigned char) entry.mResult);
        stream.putVarInt(entry.mRequestId);

        stream.putBool(true);
        if (entry.mContainers.empty()) {
            stream.putBool(false);
            continue;
        }

        stream.putBool(true);
        stream.putArrayLength((uint32_t) entry.mContainers.size());
        for (const ItemStackResponseContainer &container: entry.mContainers) {
            writeItemStackResponseContainer(stream, container);
        }
    }
}

void ItemStackResponsePacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    uint32_t entryCount = stream.getArrayLength();
    mEntries.reserve(entryCount);
    for (uint32_t i = 0; i < entryCount; i++) {
        ItemStackResponseEntry entry;
        entry.mResult = stream.getByte();
        entry.mRequestId = stream.getVarInt();

        bool hasContainers = stream.getBool() && stream.getBool();
        if (hasContainers) {
            uint32_t containerCount = stream.getArrayLength();
            entry.mContainers.reserve(containerCount);
            for (uint32_t j = 0; j < containerCount; j++) {
                entry.mContainers.push_back(readItemStackResponseContainer(stream));
            }
        }

        mEntries.push_back(entry);
    }
}

void ItemStackResponsePacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}

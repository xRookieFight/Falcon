#include "protocol/packets/DimensionDataPacket.h"

#include "protocol/NetworkPacketHandler.h"

DimensionDataPacket::DimensionDataPacket() = default;

void DimensionDataPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    stream.putArrayLength((uint32_t) mDefinitions.size());
    for (const DimensionDefinition &definition: mDefinitions) {
        stream.putString(definition.mId);
        stream.putVarInt(definition.mMaximumHeight);
        stream.putVarInt(definition.mMinimumHeight);
        stream.putVarInt(definition.mGeneratorType);
        stream.putVarInt(definition.mDimensionType);
        stream.putUuid(definition.mPackId);
    }
}

void DimensionDataPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mDefinitions.clear();
    uint32_t count = stream.getArrayLength();
    for (uint32_t i = 0; i < count; i++) {
        DimensionDefinition definition;
        definition.mId = stream.getString();
        definition.mMaximumHeight = stream.getVarInt();
        definition.mMinimumHeight = stream.getVarInt();
        definition.mGeneratorType = stream.getVarInt();
        definition.mDimensionType = stream.getVarInt();
        definition.mPackId = stream.getUuid();
        mDefinitions.push_back(definition);
    }
}

void DimensionDataPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}

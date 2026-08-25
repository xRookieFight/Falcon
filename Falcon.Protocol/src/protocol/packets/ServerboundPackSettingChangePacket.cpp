#include "protocol/packets/ServerboundPackSettingChangePacket.h"

#include "protocol/DataStoreCodec.h"
#include "protocol/NetworkPacketHandler.h"

ServerboundPackSettingChangePacket::ServerboundPackSettingChangePacket() = default;

void ServerboundPackSettingChangePacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    stream.putUuid(mPackId);
    stream.putString(mPackSettingName);
    DataStoreCodec::writeScalar(stream, mPackSettingValue);
}

void ServerboundPackSettingChangePacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mPackId = stream.getUuid();
    mPackSettingName = stream.getString();
    mPackSettingValue = DataStoreCodec::readScalar(stream);
}

void ServerboundPackSettingChangePacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}

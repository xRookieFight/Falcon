#include "Protocol/Packets/HurtArmorPacket.h"

#include "Protocol/NetworkPacketHandler.h"

HurtArmorPacket::HurtArmorPacket()
        : mCause(0), mDamage(0), mArmorSlots(0) {}

void HurtArmorPacket::write(BinaryStream &stream) const {
    stream.putVarInt(mCause);
    stream.putVarInt(mDamage);
    stream.putUnsignedVarLong(mArmorSlots);
}

void HurtArmorPacket::read(ReadOnlyBinaryStream &stream) {
    mCause = stream.getVarInt();
    mDamage = stream.getVarInt();
    mArmorSlots = stream.getUnsignedVarLong();
}

void HurtArmorPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}

#include "Protocol/Packets/ShowStoreOfferPacket.h"

#include "Protocol/NetworkPacketHandler.h"

ShowStoreOfferPacket::ShowStoreOfferPacket() = default;

void ShowStoreOfferPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    stream.putString(mOfferId);
    stream.putByte((unsigned char) mRedirectType);
}

void ShowStoreOfferPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mOfferId = stream.getString();
    mRedirectType = (StoreOfferRedirectType) stream.getByte();
}

void ShowStoreOfferPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}

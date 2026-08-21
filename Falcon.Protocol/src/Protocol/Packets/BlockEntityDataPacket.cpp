#include "Protocol/Packets/BlockEntityDataPacket.h"

#include "Core/NBT/NbtIo.h"
#include "Protocol/NetworkPacketHandler.h"

BlockEntityDataPacket::BlockEntityDataPacket()
        : mData(Tag::ofCompound()) {}

void BlockEntityDataPacket::write(BinaryStream &stream) const {
    stream.putBlockPosition(mBlockPosition);
    NbtIo::writeTag(stream, mData, NbtVariant::Network);
}

void BlockEntityDataPacket::read(ReadOnlyBinaryStream &stream) {
    mBlockPosition = stream.getBlockPosition();
    mData = NbtIo::readTag(stream, NbtVariant::Network);

    if (!mData.isCompound())
        throw BinaryDataException("Expected a compound tag in BlockEntityDataPacket");
}

void BlockEntityDataPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}

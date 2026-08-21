#include "Protocol/Packets/EmoteListPacket.h"

#include "Protocol/NetworkPacketHandler.h"

EmoteListPacket::EmoteListPacket()
        : mRuntimeEntityId(0) {}

void EmoteListPacket::write(BinaryStream &stream) const {
    stream.putUnsignedVarLong(mRuntimeEntityId);

    stream.putArrayLength((uint32_t) mPieceIds.size());
    for (const UUID &pieceId: mPieceIds)
        stream.putUuid(pieceId);
}

void EmoteListPacket::read(ReadOnlyBinaryStream &stream) {
    mRuntimeEntityId = stream.getUnsignedVarLong();

    const uint32_t pieceCount = stream.getArrayLength();
    mPieceIds.clear();
    mPieceIds.reserve(pieceCount);

    for (uint32_t i = 0; i < pieceCount; i++)
        mPieceIds.push_back(stream.getUuid());
}

void EmoteListPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}

#include "Protocol/Packets/AddBehaviorTreePacket.h"

#include "Protocol/NetworkPacketHandler.h"

AddBehaviorTreePacket::AddBehaviorTreePacket() {}

void AddBehaviorTreePacket::write(BinaryStream &stream) const {
    stream.putString(mBehaviorTreeJson);
}

void AddBehaviorTreePacket::read(ReadOnlyBinaryStream &stream) {
    mBehaviorTreeJson = stream.getString();
}

void AddBehaviorTreePacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}

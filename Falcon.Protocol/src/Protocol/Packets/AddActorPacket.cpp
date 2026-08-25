#include "Protocol/Packets/AddActorPacket.h"

#include "Protocol/EntityCodec.h"
#include "Protocol/NetworkPacketHandler.h"

AddActorPacket::AddActorPacket() = default;

void AddActorPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    stream.putVarLong(mUniqueActorId);
    stream.putUnsignedVarLong((uint64_t) mRuntimeActorId);
    stream.putString(mIdentifier);
    stream.putVector3f(mPosition);
    stream.putVector3f(mMotion);
    stream.putVector2f(mRotation);
    stream.putLFloat(mHeadRotation);
    stream.putLFloat(mBodyRotation);

    stream.putArrayLength((uint32_t) mAttributes.size());
    for (const AttributeData &attribute: mAttributes) {
        EntityCodec::writeAttribute(stream, attribute);
    }

    EntityCodec::writeEntityData(stream, mMetadata);
    EntityCodec::writeEntityProperties(stream, mProperties);

    stream.putArrayLength((uint32_t) mActorLinks.size());
    for (const EntityLinkData &link: mActorLinks) {
        EntityCodec::writeEntityLink(stream, link);
    }
}

void AddActorPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mUniqueActorId = stream.getVarLong();
    mRuntimeActorId = (int64_t) stream.getUnsignedVarLong();
    mIdentifier = stream.getString();
    mPosition = stream.getVector3f();
    mMotion = stream.getVector3f();
    mRotation = stream.getVector2f();
    mHeadRotation = stream.getLFloat();
    mBodyRotation = stream.getLFloat();

    uint32_t attributeCount = stream.getArrayLength();
    mAttributes.reserve(attributeCount);
    for (uint32_t i = 0; i < attributeCount; i++) {
        mAttributes.push_back(EntityCodec::readAttribute(stream));
    }

    mMetadata = EntityCodec::readEntityData(stream);
    mProperties = EntityCodec::readEntityProperties(stream);

    uint32_t linkCount = stream.getArrayLength();
    mActorLinks.reserve(linkCount);
    for (uint32_t i = 0; i < linkCount; i++) {
        mActorLinks.push_back(EntityCodec::readEntityLink(stream));
    }
}

void AddActorPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}

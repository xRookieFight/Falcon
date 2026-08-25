#include "Network/NetworkIdentifier.h"

const NetworkIdentifier INVALID_NETWORK_IDENTIFIER;

NetworkIdentifier::NetworkIdentifier()
        : mGuid(RakNet::UNASSIGNED_RAKNET_GUID), mSystemAddress(RakNet::UNASSIGNED_SYSTEM_ADDRESS),
          mType(Type::Generic) {}

NetworkIdentifier::NetworkIdentifier(const RakNet::RakNetGUID &guid, const RakNet::SystemAddress &systemAddress)
        : mGuid(guid), mSystemAddress(systemAddress), mType(Type::RakNet) {}

std::string NetworkIdentifier::getAddress() const {
    return mSystemAddress.ToString(false);
}

size_t NetworkIdentifier::getHash() const {
    if (mType == Type::RakNet)
        return (size_t) mGuid.g;

    return RakNet::SystemAddressHash()(mSystemAddress);
}

std::string NetworkIdentifier::toString() const {
    return mSystemAddress.ToString() + " (" + mGuid.ToString() + ")";
}

bool NetworkIdentifier::operator==(const NetworkIdentifier &right) const {
    if (mType != right.mType)
        return false;

    if (mType == Type::RakNet)
        return mGuid == right.mGuid;

    return mSystemAddress == right.mSystemAddress;
}

bool NetworkIdentifier::operator!=(const NetworkIdentifier &right) const {
    return !(*this == right);
}

size_t NetworkIdentifier::Hasher::operator()(const NetworkIdentifier &identifier) const {
    return identifier.getHash();
}

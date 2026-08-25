#pragma once

#include "raknet/RakNetTypes.h"

#include <cstddef>
#include <string>

class NetworkIdentifier {
public:
    enum class Type : unsigned char {
        RakNet = 0,
        NetherNet = 1,
        Generic = 2
    };

    struct Hasher {
        size_t operator()(const NetworkIdentifier &identifier) const;
    };

    NetworkIdentifier();

    NetworkIdentifier(const RakNet::RakNetGUID &guid, const RakNet::SystemAddress &systemAddress);

    Type getType() const { return mType; }

    const RakNet::RakNetGUID &getGuid() const { return mGuid; }

    const RakNet::SystemAddress &getSystemAddress() const { return mSystemAddress; }

    std::string getAddress() const;

    size_t getHash() const;

    std::string toString() const;

    bool operator==(const NetworkIdentifier &right) const;

    bool operator!=(const NetworkIdentifier &right) const;

private:
    RakNet::RakNetGUID mGuid;
    RakNet::SystemAddress mSystemAddress;
    Type mType;
};

extern const NetworkIdentifier INVALID_NETWORK_IDENTIFIER;

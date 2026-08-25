#pragma once

#include "Network/NetworkIdentifier.h"

#include <cstdint>
#include <unordered_map>

class PlayerHandleRegistry {
public:
    uint32_t acquire(const NetworkIdentifier &id) {
        auto existing = mByIdentifier.find(id);
        if (existing != mByIdentifier.end())
            return existing->second;

        const uint32_t handle = mNextHandle++;
        mByHandle.emplace(handle, id);
        mByIdentifier.emplace(id, handle);
        return handle;
    }

    const NetworkIdentifier *lookup(uint32_t handle) const {
        auto it = mByHandle.find(handle);
        return it == mByHandle.end() ? nullptr : &it->second;
    }

private:
    std::unordered_map<uint32_t, NetworkIdentifier> mByHandle;
    std::unordered_map<NetworkIdentifier, uint32_t, NetworkIdentifier::Hasher> mByIdentifier;
    uint32_t mNextHandle = 1;
};

#include "Actor/ServerActor.h"

ServerActor::ServerActor(uint64_t runtimeId, const std::string &identifier)
        : Actor(runtimeId), mIdentifier(identifier) {}

int32_t ServerActor::getIntProperty(const std::string &name, int32_t fallback) const {
    const auto it = mIntProperties.find(name);
    return it == mIntProperties.end() ? fallback : it->second;
}

float ServerActor::getFloatProperty(const std::string &name, float fallback) const {
    const auto it = mFloatProperties.find(name);
    return it == mFloatProperties.end() ? fallback : it->second;
}

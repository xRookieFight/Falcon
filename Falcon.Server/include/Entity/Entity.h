#pragma once

#include "Core/Math/Vector3f.h"
#include "Entity/EntityAttributes.h"
#include "Entity/EntityFlags.h"

#include <cstdint>
#include <string>

class Entity {
public:
    explicit Entity(uint64_t runtimeId);

    virtual ~Entity() = default;

    virtual const char *getIdentifier() const = 0;

    uint64_t getRuntimeId() const { return mRuntimeId; }

    int64_t getUniqueId() const { return (int64_t) mRuntimeId; }

    const Vector3f &getPosition() const { return mPosition; }

    void setPosition(const Vector3f &position) { mPosition = position; }

    const Vector3f &getRotation() const { return mRotation; }

    void setRotation(const Vector3f &rotation) { mRotation = rotation; }

    const Vector3f &getMotion() const { return mMotion; }

    void setMotion(const Vector3f &motion) { mMotion = motion; }

    EntityFlags &getFlags() { return mFlags; }

    const EntityFlags &getFlags() const { return mFlags; }

    EntityAttributes &getAttributes() { return mAttributes; }

    const EntityAttributes &getAttributes() const { return mAttributes; }

    bool isOnGround() const { return mOnGround; }

    void setOnGround(bool onGround) { mOnGround = onGround; }

protected:
    uint64_t mRuntimeId;
    Vector3f mPosition;
    Vector3f mRotation;
    Vector3f mMotion;
    EntityFlags mFlags;
    EntityAttributes mAttributes;
    bool mOnGround = false;
};

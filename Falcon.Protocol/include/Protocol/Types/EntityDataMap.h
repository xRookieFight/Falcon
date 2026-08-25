#pragma once

#include "core/math/Vector3f.h"
#include "core/math/Vector3i.h"
#include "core/nbt/Tag.h"

#include <cstdint>
#include <string>
#include <vector>

enum class EntityDataFormat {
    Byte,
    Short,
    Int,
    Float,
    String,
    Nbt,
    Vector3i,
    Long,
    Vector3f,
};

class EntityDataEntry {
public:
    int32_t mId = 0;
    EntityDataFormat mFormat = EntityDataFormat::Byte;

    int8_t mByteValue = 0;
    int16_t mShortValue = 0;
    int32_t mIntValue = 0;
    float mFloatValue = 0.0f;
    std::string mStringValue;
    Tag mNbtValue;
    Vector3i mVector3iValue;
    int64_t mLongValue = 0;
    Vector3f mVector3fValue;
};

class EntityDataMap {
public:
    std::vector<EntityDataEntry> mEntries;
};

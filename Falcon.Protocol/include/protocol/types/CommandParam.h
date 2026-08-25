#pragma once

#include <cstdint>

enum class CommandParamType : int32_t {
    Int = 1,
    Float = 2,
    Value = 3,
    WildcardInt = 5,
    Operator = 6,
    CompareOperator = 7,
    Target = 8,
    WildcardTarget = 10,
    Filepath = 17,
    IntegerRange = 22,
    EquipmentSlots = 47,
    String = 56,
    BlockPosition = 64,
    Position = 65,
    Message = 67,
    RawText = 70,
    Json = 74,
    BlockStates = 84,
    Command = 87,
};

const uint32_t ARG_FLAG_VALID = 0x100000;
const uint32_t ARG_FLAG_ENUM = 0x200000;
const uint32_t ARG_FLAG_POSTFIX = 0x1000000;
const uint32_t ARG_FLAG_SOFT_ENUM = 0x4000000;

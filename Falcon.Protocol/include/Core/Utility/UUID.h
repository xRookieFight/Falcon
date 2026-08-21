#pragma once

#include <cstdint>
#include <cstdio>
#include <string>

struct UUID {
    uint64_t mostSignificantBits;
    uint64_t leastSignificantBits;

    UUID() : mostSignificantBits(0), leastSignificantBits(0) {}

    UUID(uint64_t most, uint64_t least) : mostSignificantBits(most), leastSignificantBits(least) {}

    bool operator==(const UUID &right) const {
        return mostSignificantBits == right.mostSignificantBits && leastSignificantBits == right.leastSignificantBits;
    }

    bool operator!=(const UUID &right) const { return !(*this == right); }

    std::string toString() const {
        char buffer[40];
        snprintf(buffer, sizeof(buffer), "%08x-%04x-%04x-%04x-%012llx",
                 (unsigned int) (mostSignificantBits >> 32),
                 (unsigned int) ((mostSignificantBits >> 16) & 0xffff),
                 (unsigned int) (mostSignificantBits & 0xffff),
                 (unsigned int) ((leastSignificantBits >> 48) & 0xffff),
                 (unsigned long long) (leastSignificantBits & 0xffffffffffffULL));
        return std::string(buffer);
    }
};

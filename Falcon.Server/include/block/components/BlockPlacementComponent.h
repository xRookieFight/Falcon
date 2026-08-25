#pragma once

#include "block/BlockState.h"
#include "core/math/Vector3f.h"

class BlockPlacementComponent {
public:
    static BlockState apply(const BlockState &state, float yaw, int face,
                            const Vector3f &clickPosition);
};

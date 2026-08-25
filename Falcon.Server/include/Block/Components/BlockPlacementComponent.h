#pragma once

#include "Block/BlockState.h"
#include "Core/Math/Vector3f.h"
#include "Core/Math/Vector3i.h"

class BlockPlacementComponent {
public:
    static BlockState apply(const BlockState &state, float yaw, int face,
                            const Vector3f &clickPosition, const Vector3f &playerPosition,
                            const Vector3i &blockPosition);
};

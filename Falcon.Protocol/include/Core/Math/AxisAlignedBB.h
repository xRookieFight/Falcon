#pragma once

#include "Core/Math/Vector3f.h"

#include <algorithm>

struct AxisAlignedBB {
    float mMinX = 0.0f;
    float mMinY = 0.0f;
    float mMinZ = 0.0f;
    float mMaxX = 0.0f;
    float mMaxY = 0.0f;
    float mMaxZ = 0.0f;

    AxisAlignedBB() = default;

    AxisAlignedBB(float minX, float minY, float minZ, float maxX, float maxY, float maxZ)
            : mMinX(minX), mMinY(minY), mMinZ(minZ), mMaxX(maxX), mMaxY(maxY), mMaxZ(maxZ) {
    }

    bool isEmpty() const {
        return mMinX >= mMaxX || mMinY >= mMaxY || mMinZ >= mMaxZ;
    }

    AxisAlignedBB offset(float x, float y, float z) const {
        return AxisAlignedBB(mMinX + x, mMinY + y, mMinZ + z, mMaxX + x, mMaxY + y, mMaxZ + z);
    }

    AxisAlignedBB expand(float x, float y, float z) const {
        return AxisAlignedBB(mMinX - x, mMinY - y, mMinZ - z, mMaxX + x, mMaxY + y, mMaxZ + z);
    }

    bool isVectorInside(float x, float y, float z) const {
        return x >= mMinX && x <= mMaxX && y >= mMinY && y <= mMaxY && z >= mMinZ && z <= mMaxZ;
    }

    bool isVectorInside(const Vector3f &position) const {
        return isVectorInside(position.x, position.y, position.z);
    }

    bool intersectsWith(const AxisAlignedBB &other, float epsilon) const {
        if (other.mMaxX - mMinX <= epsilon || mMaxX - other.mMinX <= epsilon)
            return false;

        if (other.mMaxY - mMinY <= epsilon || mMaxY - other.mMinY <= epsilon)
            return false;

        return other.mMaxZ - mMinZ > epsilon && mMaxZ - other.mMinZ > epsilon;
    }

    bool intersectsWith(const AxisAlignedBB &other) const {
        return intersectsWith(other, 1.0E-5f);
    }

    float getAverageEdgeLength() const {
        return ((mMaxX - mMinX) + (mMaxY - mMinY) + (mMaxZ - mMinZ)) / 3.0f;
    }
};

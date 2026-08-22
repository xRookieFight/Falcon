#pragma once

#include "Core/Utility/UUID.h"

#include <cstdint>
#include <string>
#include <vector>

struct RecipeIngredientEntry {
    bool mHasItem = false;
    std::string mItemId;
    int32_t mAuxValue = -1;
    int32_t mCount = 1;
};

struct RecipeOutputEntry {
    int32_t mRuntimeId = 0;
    int32_t mCount = 1;
    int32_t mMeta = 0;
};

struct CraftingRecipeEntry {
    std::string mRecipeId;
    int32_t mWidth = 0;
    int32_t mHeight = 0;
    std::vector<RecipeIngredientEntry> mInputs;
    std::vector<RecipeOutputEntry> mOutputs;
    Uuid mUuid;
    std::string mBlockName;
    int32_t mPriority = 0;
    bool mSymmetric = false;
    int32_t mRecipeNetId = 0;
};

#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum class AimAssistAction {
    Set,
    Clear,
};

enum class CameraAimAssistOperation {
    Set,
    AddToExisting,
};

class CameraAimAssistPriority {
public:
    std::string mName;
    int32_t mPriority = 0;
};

class CameraAimAssistItemSettings {
public:
    std::string mItemId;
    std::string mCategory;
};

class CameraAimAssistCategory {
public:
    std::string mName;
    std::vector<CameraAimAssistPriority> mEntityPriorities;
    std::vector<CameraAimAssistPriority> mBlockPriorities;
    std::vector<CameraAimAssistPriority> mBlockTagPriorities;
    std::vector<CameraAimAssistPriority> mEntityTypeFamiliesPriorities;

    bool mHasEntityDefaultPriorities = false;
    int32_t mEntityDefaultPriorities = 0;

    bool mHasBlockDefaultPriorities = false;
    int32_t mBlockDefaultPriorities = 0;
};

class CameraAimAssistPresetDefinition {
public:
    std::string mIdentifier;
    std::vector<std::string> mBlockExclusionList;
    std::vector<std::string> mEntityExclusionList;
    std::vector<std::string> mBlockTagExclusionList;
    std::vector<std::string> mEntityTypeFamiliesExclusionList;
    std::vector<std::string> mLiquidTargetingList;
    std::vector<CameraAimAssistItemSettings> mItemSettings;

    bool mHasDefaultItemSettings = false;
    std::string mDefaultItemSettings;

    bool mHasHandSettings = false;
    std::string mHandSettings;
};

class AimAssistActorPriorityData {
public:
    int32_t mPresetIndex = 0;
    int32_t mCategoryIndex = 0;
    int32_t mActorIndex = 0;
    int32_t mPriorityValue = 0;
};

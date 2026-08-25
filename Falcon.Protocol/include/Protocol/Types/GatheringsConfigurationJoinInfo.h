#pragma once

#include "core/utility/UUID.h"

#include <string>

class GatheringsConfigurationJoinInfo {
public:
    Uuid mExperienceId;
    std::string mExperienceName;
    bool mHasWorldId = false;
    Uuid mWorldId;
    bool mHasWorldName = false;
    std::string mWorldName;
    std::string mCreatorId;
    bool mHasTargetId = false;
    Uuid mTargetId;
    bool mHasScenarioId = false;
    std::string mScenarioId;
    bool mHasServerId = false;
    std::string mServerId;
};

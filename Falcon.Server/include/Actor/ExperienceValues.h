#pragma once

#include <string>
#include <vector>

namespace ExperienceValues {

    std::vector<int> splitIntoOrbSizes(int amount);

    int getMobDropExperience(const std::string &identifier);

    int getOreDropExperience(const std::string &blockName);

}

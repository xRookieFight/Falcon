#include "Command/Command.h"

Command::Command(const std::string &name, const std::string &description, const std::string &usage,
                 const std::vector<std::string> &aliases)
        : mName(name), mDescription(description), mUsage(usage), mAliases(aliases) {}

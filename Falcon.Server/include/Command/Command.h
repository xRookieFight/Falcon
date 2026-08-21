#pragma once

#include "Command/CommandSender.h"

#include <string>
#include <vector>

class Server;

class Command {
public:
    Command(const std::string &name, const std::string &description, const std::string &usage,
            const std::vector<std::string> &aliases = {});

    virtual ~Command() = default;

    virtual bool execute(CommandSender &sender, const std::vector<std::string> &arguments) = 0;

    const std::string &getName() const { return mName; }

    const std::string &getDescription() const { return mDescription; }

    const std::string &getUsage() const { return mUsage; }

    const std::vector<std::string> &getAliases() const { return mAliases; }

protected:
    std::string mName;
    std::string mDescription;
    std::string mUsage;
    std::vector<std::string> mAliases;
};

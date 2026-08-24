#pragma once

#include "Command/Command.h"

#include <cstdint>

class ServerNetworkHandler;

class TimeCommand : public Command {
public:
    explicit TimeCommand(ServerNetworkHandler &handler);

    bool execute(CommandSender &sender, const std::vector<std::string> &arguments) override;

    std::vector<CommandOverloadData> getOverloads() const override;

private:
    static bool parseInteger(const std::string &value, int64_t &out);
    static bool parseTime(const std::string &value, int64_t &out);

    ServerNetworkHandler &mHandler;
};

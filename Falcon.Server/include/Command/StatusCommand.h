#pragma once

#include "command/Command.h"

#include <cstdint>

class ServerNetworkHandler;

class StatusCommand : public Command {
public:
    explicit StatusCommand(ServerNetworkHandler &handler);

    bool execute(CommandOrigin &sender, const std::vector<std::string> &arguments) override;

    std::vector<CommandOverloadData> getOverloads() const override;

private:
    static std::string formatUptime(int64_t seconds);

    ServerNetworkHandler &mHandler;
};

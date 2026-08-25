#pragma once

#include "command/Command.h"

class ServerNetworkHandler;

class KillCommand : public Command {
public:
    explicit KillCommand(ServerNetworkHandler &handler);

    bool execute(CommandOrigin &sender, const std::vector<std::string> &arguments) override;

    CommandPermission getRequiredPermission() const override;

    std::vector<CommandOverloadData> getOverloads() const override;

private:
    ServerNetworkHandler &mHandler;
};

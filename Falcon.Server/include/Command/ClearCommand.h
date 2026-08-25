#pragma once

#include "Command/Command.h"

class ServerNetworkHandler;

class ClearCommand final : public Command {
public:
    explicit ClearCommand(ServerNetworkHandler &handler);

    CommandPermission getRequiredPermission() const override;

    bool execute(CommandOrigin &sender, const std::vector<std::string> &arguments) override;

    std::vector<CommandOverloadData> getOverloads() const override;

private:
    ServerNetworkHandler &mHandler;
};

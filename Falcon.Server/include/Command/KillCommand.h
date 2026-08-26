#pragma once

#include "Command/Command.h"

class ServerNetworkHandler;

class KillCommand : public Command {
public:
    explicit KillCommand(ServerNetworkHandler &handler);

    bool execute(CommandOrigin &sender, const std::vector<std::string> &arguments) override;


    std::vector<CommandOverloadData> getOverloads() const override;

private:
    ServerNetworkHandler &mHandler;
};

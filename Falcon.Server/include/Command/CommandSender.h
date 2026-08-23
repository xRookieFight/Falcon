#pragma once

#include "Protocol/Types/AdventureSettingData.h"

#include <string>
#include <vector>

class ServerPlayer;

class CommandSender {
public:
    virtual ~CommandSender() = default;

    virtual const std::string &getSenderName() const = 0;

    virtual bool isPlayer() const = 0;

    virtual ServerPlayer *asPlayer() = 0;

    virtual void sendMessage(const std::string &message) = 0;

    virtual void sendTranslation(const std::string &key, const std::vector<std::string> &parameters) = 0;

    virtual CommandPermission getCommandPermission() const = 0;
};

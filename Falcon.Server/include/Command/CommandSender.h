#pragma once

#include <string>

class ServerPlayer;

class CommandSender {
public:
    virtual ~CommandSender() = default;

    virtual const std::string &getSenderName() const = 0;

    virtual bool isPlayer() const = 0;

    virtual ServerPlayer *asPlayer() = 0;

    virtual void sendMessage(const std::string &message) = 0;
};

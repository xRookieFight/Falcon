#pragma once

#include <string>

class ServerNetworkHandler;
class ServerPlayer;

class ChatHandler {
public:
    static void broadcastChat(ServerNetworkHandler &owner, ServerPlayer &player, const std::string &message);
};

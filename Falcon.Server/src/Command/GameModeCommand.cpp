#include "Command/GameModeCommand.h"

#include "Actor/ServerPlayer.h"
#include "Network/ServerNetworkHandler.h"

#include <algorithm>
#include <cctype>

namespace {
    std::string toLowerCase(const std::string &value) {
        std::string lowered = value;
        std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                       [](unsigned char character) { return (char) std::tolower(character); });
        return lowered;
    }
}

GameModeCommand::GameModeCommand(ServerNetworkHandler &handler)
        : Command("gamemode", "commands.gamemode.description", "/gamemode <mode> [player]", {"gm"}),
          mHandler(handler) {}

int GameModeCommand::parseGameMode(const std::string &value) {
    const std::string mode = toLowerCase(value);

    if (mode == "0" || mode == "survival" || mode == "s")
        return 0;
    if (mode == "1" || mode == "creative" || mode == "c")
        return 1;
    if (mode == "2" || mode == "adventure" || mode == "a")
        return 2;
    if (mode == "3" || mode == "spectator" || mode == "spc" || mode == "view" || mode == "v")
        return 3;

    return -1;
}

const char *GameModeCommand::getGameModeName(int gameMode) {
    switch (gameMode) {
        case 0:
            return "Survival";
        case 1:
            return "Creative";
        case 2:
            return "Adventure";
        case 3:
            return "Spectator";
        default:
            return "Unknown";
    }
}

bool GameModeCommand::execute(CommandOrigin &sender, const std::vector<std::string> &arguments) {
    if (arguments.empty()) {
        sender.sendTranslation("commands.generic.usage", {getUsage()});
        return false;
    }

    const int gameMode = parseGameMode(arguments[0]);
    if (gameMode < 0) {
        sender.sendTranslation("commands.gamemode.invalid", {arguments[0]});
        return false;
    }

    ServerPlayer *target = arguments.size() > 1
                           ? mHandler.getPlayerByName(arguments[1])
                           : sender.asPlayer();

    if (target == nullptr) {
        if (arguments.size() > 1)
            sender.sendTranslation("commands.generic.player.notFound", {});
        else
            sender.sendTranslation("commands.generic.playerOnly", {});
        return false;
    }

    mHandler.setPlayerGameMode(*target, gameMode);

    const std::string modeName = getGameModeName(gameMode);
    if (target == sender.asPlayer()) {
        sender.sendTranslation("commands.gamemode.success.self", {modeName});
    } else {
        sender.sendTranslation("commands.gamemode.success.other", {target->getName(), modeName});
    }

    return true;
}

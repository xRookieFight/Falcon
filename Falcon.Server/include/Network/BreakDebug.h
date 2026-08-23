#pragma once

#include <string>

class ServerPlayer;

namespace BreakDebug {
    void log(const std::string &event, const ServerPlayer *player = nullptr,
             const std::string &details = std::string());
}

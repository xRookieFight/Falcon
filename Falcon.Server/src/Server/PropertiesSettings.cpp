#include "Server/PropertiesSettings.h"

#include <cstdlib>
#include <fstream>

PropertiesSettings::PropertiesSettings() : mLoaded(false) {}

PropertiesSettings::PropertiesSettings(const std::string &path) : mLoaded(false) {
    load(path);
}

std::string PropertiesSettings::_trim(const std::string &value) {
    size_t start = 0;
    size_t end = value.size();

    while (start < end && (value[start] == ' ' || value[start] == '\t' || value[start] == '\r'))
        start++;

    while (end > start && (value[end - 1] == ' ' || value[end - 1] == '\t' || value[end - 1] == '\r'))
        end--;

    return value.substr(start, end - start);
}

bool PropertiesSettings::load(const std::string &path) {
    mPath = path;
    mProperties.clear();
    mLoaded = false;

    std::ifstream file(path);
    if (!file.is_open())
        return false;

    std::string line;
    while (std::getline(file, line)) {
        const std::string trimmed = _trim(line);
        if (trimmed.empty() || trimmed[0] == '#')
            continue;

        const size_t separator = trimmed.find('=');
        if (separator == std::string::npos)
            continue;

        const std::string key = _trim(trimmed.substr(0, separator));
        if (key.empty())
            continue;

        mProperties[key] = _trim(trimmed.substr(separator + 1));
    }

    mLoaded = true;
    return true;
}

bool PropertiesSettings::hasProperty(const std::string &key) const {
    return mProperties.find(key) != mProperties.end();
}

std::string PropertiesSettings::getString(const std::string &key, const std::string &defaultValue) const {
    auto it = mProperties.find(key);
    return it == mProperties.end() ? defaultValue : it->second;
}

int PropertiesSettings::getInt(const std::string &key, int defaultValue) const {
    auto it = mProperties.find(key);
    if (it == mProperties.end() || it->second.empty())
        return defaultValue;

    return atoi(it->second.c_str());
}

float PropertiesSettings::getFloat(const std::string &key, float defaultValue) const {
    auto it = mProperties.find(key);
    if (it == mProperties.end() || it->second.empty())
        return defaultValue;

    return (float) atof(it->second.c_str());
}

bool PropertiesSettings::getBool(const std::string &key, bool defaultValue) const {
    auto it = mProperties.find(key);
    if (it == mProperties.end() || it->second.empty())
        return defaultValue;

    return it->second == "true" || it->second == "1";
}

GameType PropertiesSettings::getGameType() const {
    const std::string value = getString("gamemode", "survival");

    if (value == "creative" || value == "1")
        return GameType::Creative;
    if (value == "adventure" || value == "2")
        return GameType::Adventure;
    if (value == "spectator" || value == "6")
        return GameType::Spectator;

    return GameType::Survival;
}

Difficulty PropertiesSettings::getDifficulty() const {
    const std::string value = getString("difficulty", "easy");

    if (value == "peaceful" || value == "0")
        return Difficulty::Peaceful;
    if (value == "normal" || value == "2")
        return Difficulty::Normal;
    if (value == "hard" || value == "3")
        return Difficulty::Hard;

    return Difficulty::Easy;
}

PlayerPermission PropertiesSettings::getDefaultPlayerPermissionLevel() const {
    const std::string value = getString("default-player-permission-level", "member");

    if (value == "visitor")
        return PlayerPermission::Visitor;
    if (value == "operator")
        return PlayerPermission::Operator;

    return PlayerPermission::Member;
}

ContentLogLevel PropertiesSettings::getContentLogLevel() const {
    const std::string value = getString("content-log-level", "info");

    if (value == "error")
        return ContentLogLevel::Error;
    if (value == "warning")
        return ContentLogLevel::Warning;
    if (value == "verbose")
        return ContentLogLevel::Verbose;

    return ContentLogLevel::Info;
}

NetworkSettingsPacket::CompressionAlgorithm PropertiesSettings::getCompressionAlgorithm() const {
    if (getString("compression-algorithm", "zlib") == "snappy")
        return NetworkSettingsPacket::CompressionAlgorithm::Snappy;

    return NetworkSettingsPacket::CompressionAlgorithm::ZLib;
}

ChatRestrictionLevel PropertiesSettings::getChatRestrictionLevel() const {
    const std::string value = getString("chat-restriction", "None");

    if (value == "Dropped")
        return ChatRestrictionLevel::Dropped;
    if (value == "Disabled")
        return ChatRestrictionLevel::Disabled;

    return ChatRestrictionLevel::None;
}

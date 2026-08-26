#include "Level/GameRules.h"

#include <algorithm>
#include <cctype>

namespace {
    std::string toLower(const std::string &value) {
        std::string result = value;
        std::transform(result.begin(), result.end(), result.begin(), [](unsigned char character) {
            return (char) std::tolower(character);
        });
        return result;
    }

    bool parseBool(const std::string &value, bool &result) {
        const std::string lowered = toLower(value);

        if (lowered == "true" || lowered == "1") {
            result = true;
            return true;
        }

        if (lowered == "false" || lowered == "0") {
            result = false;
            return true;
        }

        return false;
    }
}

GameRules::GameRules() {
    Rule fireTick;
    fireTick.mName = "dofiretick";
    fireTick.mType = GameRuleData::Type::Bool;
    fireTick.mBoolValue = true;
    mRules.push_back(fireTick);

    Rule daylightCycle;
    daylightCycle.mName = "dodaylightcycle";
    daylightCycle.mType = GameRuleData::Type::Bool;
    daylightCycle.mBoolValue = true;
    mRules.push_back(daylightCycle);

    Rule showCoordinates;
    showCoordinates.mName = "showcoordinates";
    showCoordinates.mType = GameRuleData::Type::Bool;
    showCoordinates.mBoolValue = true;
    mRules.push_back(showCoordinates);
}

const GameRules::Rule *GameRules::find(const std::string &name) const {
    const std::string lowered = toLower(name);

    for (const Rule &rule: mRules) {
        if (rule.mName == lowered)
            return &rule;
    }

    return nullptr;
}

GameRules::Rule *GameRules::_find(const std::string &name) {
    const std::string lowered = toLower(name);

    for (Rule &rule: mRules) {
        if (rule.mName == lowered)
            return &rule;
    }

    return nullptr;
}

bool GameRules::getBool(const std::string &name) const {
    const Rule *rule = find(name);
    return rule != nullptr && rule->mType == GameRuleData::Type::Bool && rule->mBoolValue;
}

int32_t GameRules::getInt(const std::string &name) const {
    const Rule *rule = find(name);
    return rule == nullptr || rule->mType != GameRuleData::Type::Int ? 0 : rule->mIntValue;
}

bool GameRules::setFromString(const std::string &name, const std::string &value) {
    Rule *rule = _find(name);
    if (rule == nullptr)
        return false;

    if (rule->mType == GameRuleData::Type::Bool)
        return parseBool(value, rule->mBoolValue);

    try {
        rule->mIntValue = (int32_t) std::stol(value);
    } catch (const std::exception &) {
        return false;
    }

    return true;
}

std::vector<std::string> GameRules::getNames() const {
    std::vector<std::string> names;
    names.reserve(mRules.size());

    for (const Rule &rule: mRules)
        names.push_back(rule.mName);

    return names;
}

std::vector<GameRuleData> GameRules::toNetwork() const {
    std::vector<GameRuleData> data;
    data.reserve(mRules.size());

    for (const Rule &rule: mRules) {
        if (rule.mType == GameRuleData::Type::Bool)
            data.push_back(GameRuleData::ofBool(rule.mName, rule.mBoolValue));
        else
            data.push_back(GameRuleData::ofInt(rule.mName, rule.mIntValue));
    }

    return data;
}

ChangedGameRuleData GameRules::toChangedNetwork(const Rule &rule) const {
    ChangedGameRuleData data;
    data.mName = rule.mName;
    data.mEditable = true;

    if (rule.mType == GameRuleData::Type::Bool) {
        data.mType = ChangedGameRuleType::Bool;
        data.mBoolValue = rule.mBoolValue;
    } else {
        data.mType = ChangedGameRuleType::Int;
        data.mIntValue = rule.mIntValue;
    }

    return data;
}

Tag GameRules::save() const {
    Tag data = Tag::ofCompound();

    for (const Rule &rule: mRules) {
        if (rule.mType == GameRuleData::Type::Bool)
            data.putByte(rule.mName, rule.mBoolValue ? 1 : 0);
        else
            data.putInt(rule.mName, rule.mIntValue);
    }

    return data;
}

void GameRules::load(const Tag &data) {
    if (!data.isCompound())
        return;

    for (Rule &rule: mRules) {
        const Tag *stored = data.get(rule.mName);
        if (stored == nullptr)
            continue;

        if (rule.mType == GameRuleData::Type::Bool)
            rule.mBoolValue = stored->asByte() != 0;
        else
            rule.mIntValue = stored->asInt();
    }
}

#include "Item/StringToItemParser.h"

#include "Block/VanillaBlocks.h"
#include "Item/VanillaItems.h"

StringToItemParser::StringToItemParser() {
    _registerDefaults();
}

StringToItemParser &StringToItemParser::getInstance() {
    static StringToItemParser instance;
    return instance;
}

std::string StringToItemParser::_normalize(const std::string &input) {
    std::string out;
    out.reserve(input.size());

    for (char c: input) {
        if (c >= 'A' && c <= 'Z') {
            out.push_back((char) (c - 'A' + 'a'));
            continue;
        }

        if (c == ' ' || c == '-') {
            out.push_back('_');
            continue;
        }

        out.push_back(c);
    }

    const std::string prefix = "minecraft:";
    if (out.size() > prefix.size() && out.compare(0, prefix.size(), prefix) == 0)
        return out.substr(prefix.size());

    return out;
}

void StringToItemParser::registerItem(const std::string &alias, const std::function<Item()> &factory) {
    mFactories[_normalize(alias)] = factory;
}

void StringToItemParser::registerBlock(const std::string &alias, const std::function<Block()> &factory) {
    mFactories[_normalize(alias)] = [factory]() {
        return Item(factory());
    };
}

void StringToItemParser::_registerDefaults() {
    registerBlock("air", VanillaBlocks::AIR);
    registerBlock("bedrock", VanillaBlocks::BEDROCK);
    registerBlock("grass", VanillaBlocks::GRASS);
    registerBlock("grass_block", VanillaBlocks::GRASS);
    registerBlock("stone", VanillaBlocks::STONE);

    registerItem("stick", VanillaItems::STICK);
}

bool StringToItemParser::parse(const std::string &input, Item &out) const {
    auto it = mFactories.find(_normalize(input));
    if (it == mFactories.end())
        return false;

    out = it->second();
    return true;
}

std::vector<std::string> StringToItemParser::getKnownAliases() const {
    std::vector<std::string> aliases;
    aliases.reserve(mFactories.size());

    for (const auto &entry: mFactories)
        aliases.push_back(entry.first);

    return aliases;
}

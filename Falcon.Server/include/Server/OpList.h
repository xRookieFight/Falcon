#pragma once

#include <string>
#include <unordered_set>

class OpList {
public:
    explicit OpList(const std::string &path);

    bool isOp(const std::string &name) const;

    void addOp(const std::string &name);

    void removeOp(const std::string &name);

    void reload();

private:
    static std::string _toLowerCase(const std::string &value);

    void _save() const;

    std::string mPath;
    std::unordered_set<std::string> mNames;
};

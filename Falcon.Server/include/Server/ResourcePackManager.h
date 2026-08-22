#pragma once

#include "Core/Utility/UUID.h"

#include <cstdint>
#include <string>
#include <vector>

class ResourcePack {
public:
    Uuid mUuid;
    std::string mUuidString;
    std::string mVersion;
    std::string mName;
    std::string mContentKey;
    std::string mData;
    std::string mSha256;
    uint64_t mSize = 0;
};

class ResourcePackManager {
public:
    void loadFromDirectory(const std::string &directory);

    const std::vector<ResourcePack> &getPacks() const { return mPacks; }

    const ResourcePack *findById(const std::string &uuid) const;

    size_t count() const { return mPacks.size(); }

private:
    bool _loadPackFile(const std::string &path, const std::string &fileName);

    static bool _readWholeFile(const std::string &path, std::string &out);

    static bool _extractZipEntry(const std::string &zip, const std::string &suffix, std::string &out);

    static bool _readManifest(const std::string &manifest, std::string &uuid, std::string &version,
                              std::string &name);

    static std::string _sha256(const std::string &data);

    std::vector<ResourcePack> mPacks;
};

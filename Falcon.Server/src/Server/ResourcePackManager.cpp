#include "Server/ResourcePackManager.h"

#include "Core/Debug/BedrockLog.h"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>

#include <zlib.h>
#include <openssl/sha.h>

namespace {

    uint16_t readU16(const std::string &data, size_t offset) {
        return (uint16_t) ((unsigned char) data[offset] | ((unsigned char) data[offset + 1] << 8));
    }

    uint32_t readU32(const std::string &data, size_t offset) {
        return (uint32_t) ((unsigned char) data[offset] | ((unsigned char) data[offset + 1] << 8) |
                           ((unsigned char) data[offset + 2] << 16) | ((unsigned char) data[offset + 3] << 24));
    }

    bool inflateRaw(const std::string &input, size_t uncompressedSize, std::string &out) {
        out.resize(uncompressedSize);

        z_stream stream;
        std::memset(&stream, 0, sizeof(stream));

        if (inflateInit2(&stream, -MAX_WBITS) != Z_OK)
            return false;

        stream.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(input.data()));
        stream.avail_in = (uInt) input.size();
        stream.next_out = reinterpret_cast<Bytef *>(&out[0]);
        stream.avail_out = (uInt) uncompressedSize;

        const int result = inflate(&stream, Z_FINISH);
        inflateEnd(&stream);

        return result == Z_STREAM_END;
    }

    std::string findJsonString(const std::string &json, const std::string &key) {
        const std::string pattern = "\"" + key + "\"";

        size_t position = json.find(pattern);
        if (position == std::string::npos)
            return std::string();

        position = json.find(':', position + pattern.size());
        if (position == std::string::npos)
            return std::string();

        position = json.find('"', position);
        if (position == std::string::npos)
            return std::string();

        std::string value;
        for (size_t i = position + 1; i < json.size(); i++) {
            if (json[i] == '"')
                return value;
            value.push_back(json[i]);
        }

        return std::string();
    }

    std::string extractObject(const std::string &json, const std::string &key) {
        const std::string pattern = "\"" + key + "\"";
        size_t position = json.find(pattern);
        if (position == std::string::npos)
            return std::string();

        position = json.find('{', position + pattern.size());
        if (position == std::string::npos)
            return std::string();

        int depth = 0;
        for (size_t i = position; i < json.size(); i++) {
            if (json[i] == '{')
                depth++;
            else if (json[i] == '}') {
                depth--;
                if (depth == 0)
                    return json.substr(position, i - position + 1);
            }
        }

        return std::string();
    }

    std::string extractVersion(const std::string &header) {
        const std::string pattern = "\"version\"";
        size_t position = header.find(pattern);
        if (position == std::string::npos)
            return "1.0.0";

        position = header.find('[', position + pattern.size());
        if (position == std::string::npos)
            return "1.0.0";

        const size_t end = header.find(']', position);
        if (end == std::string::npos)
            return "1.0.0";

        std::string numbers;
        std::string current;
        for (size_t i = position + 1; i < end; i++) {
            const char c = header[i];
            if (c >= '0' && c <= '9') {
                current.push_back(c);
            } else if (!current.empty()) {
                numbers += (numbers.empty() ? "" : ".") + current;
                current.clear();
            }
        }
        if (!current.empty())
            numbers += (numbers.empty() ? "" : ".") + current;

        return numbers.empty() ? "1.0.0" : numbers;
    }

}

bool ResourcePackManager::_readWholeFile(const std::string &path, std::string &out) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
        return false;

    const std::streamsize size = file.tellg();
    if (size <= 0)
        return false;

    out.resize((size_t) size);
    file.seekg(0);
    file.read(&out[0], size);
    return file.good();
}

bool ResourcePackManager::_extractZipEntry(const std::string &zip, const std::string &suffix, std::string &out) {
    if (zip.size() < 22)
        return false;

    size_t eocd = std::string::npos;
    for (size_t i = zip.size() - 22; i + 4 <= zip.size(); i--) {
        if (readU32(zip, i) == 0x06054b50) {
            eocd = i;
            break;
        }
        if (i == 0)
            break;
    }

    if (eocd == std::string::npos)
        return false;

    const uint16_t entryCount = readU16(zip, eocd + 10);
    uint32_t offset = readU32(zip, eocd + 16);

    for (uint16_t entry = 0; entry < entryCount; entry++) {
        if (offset + 46 > zip.size() || readU32(zip, offset) != 0x02014b50)
            return false;

        const uint16_t method = readU16(zip, offset + 10);
        const uint32_t compressedSize = readU32(zip, offset + 20);
        const uint32_t uncompressedSize = readU32(zip, offset + 24);
        const uint16_t nameLength = readU16(zip, offset + 28);
        const uint16_t extraLength = readU16(zip, offset + 30);
        const uint16_t commentLength = readU16(zip, offset + 32);
        const uint32_t localOffset = readU32(zip, offset + 42);

        const std::string name = zip.substr(offset + 46, nameLength);
        offset += 46 + nameLength + extraLength + commentLength;

        const bool matches = name.size() >= suffix.size() &&
                             name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0;
        if (!matches)
            continue;

        if (localOffset + 30 > zip.size() || readU32(zip, localOffset) != 0x04034b50)
            return false;

        const uint16_t localNameLength = readU16(zip, localOffset + 26);
        const uint16_t localExtraLength = readU16(zip, localOffset + 28);
        const size_t dataStart = localOffset + 30 + localNameLength + localExtraLength;

        if (dataStart + compressedSize > zip.size())
            return false;

        const std::string raw = zip.substr(dataStart, compressedSize);

        if (method == 0) {
            out = raw;
            return true;
        }

        if (method == 8)
            return inflateRaw(raw, uncompressedSize, out);

        return false;
    }

    return false;
}

bool ResourcePackManager::_readManifest(const std::string &manifest, std::string &uuid, std::string &version,
                                        std::string &name) {
    const std::string header = extractObject(manifest, "header");
    if (header.empty())
        return false;

    uuid = findJsonString(header, "uuid");
    version = extractVersion(header);
    name = findJsonString(header, "name");
    return !uuid.empty();
}

std::string ResourcePackManager::_sha256(const std::string &data) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char *>(data.data()), data.size(), digest);
    return std::string(reinterpret_cast<const char *>(digest), SHA256_DIGEST_LENGTH);
}

bool ResourcePackManager::_loadPackFile(const std::string &path, const std::string &fileName) {
    std::string data;
    if (!_readWholeFile(path, data)) {
        LOG_WARN(LogAreaID::Server, "Could not read resource pack %s", fileName.c_str());
        return false;
    }

    std::string manifest;
    if (!_extractZipEntry(data, "manifest.json", manifest)) {
        LOG_WARN(LogAreaID::Server, "Resource pack %s has no readable manifest.json", fileName.c_str());
        return false;
    }

    std::string uuid;
    std::string version;
    std::string name;
    if (!_readManifest(manifest, uuid, version, name)) {
        LOG_WARN(LogAreaID::Server, "Resource pack %s manifest is missing a header uuid", fileName.c_str());
        return false;
    }

    ResourcePack pack;
    pack.mUuidString = uuid;
    pack.mUuid = Uuid::fromString(uuid);
    pack.mVersion = version;
    pack.mName = name.empty() ? fileName : name;
    pack.mData = std::move(data);
    pack.mSize = pack.mData.size();
    pack.mSha256 = _sha256(pack.mData);

    std::filesystem::path keyPath = std::filesystem::path(path);
    keyPath.replace_extension(".key");

    std::string key;
    if (_readWholeFile(keyPath.string(), key)) {
        while (!key.empty() && (key.back() == '\n' || key.back() == '\r' || key.back() == ' ' || key.back() == '\t'))
            key.pop_back();

        pack.mContentKey = key;
        LOG_INFO(LogAreaID::Server, "Resource pack %s is encrypted (loaded content key)", pack.mName.c_str());
    }

    LOG_INFO(LogAreaID::Server, "Loaded resource pack %s v%s (%llu bytes)", pack.mName.c_str(), version.c_str(),
             (unsigned long long) pack.mSize);

    mPacks.push_back(std::move(pack));
    return true;
}

void ResourcePackManager::loadFromDirectory(const std::string &directory) {
    mPacks.clear();

    std::error_code error;
    if (!std::filesystem::exists(directory, error)) {
        std::filesystem::create_directories(directory, error);
        return;
    }

    for (const std::filesystem::directory_entry &entry: std::filesystem::directory_iterator(directory, error)) {
        if (!entry.is_regular_file())
            continue;

        const std::string extension = entry.path().extension().string();
        if (extension != ".zip" && extension != ".mcpack")
            continue;

        _loadPackFile(entry.path().string(), entry.path().filename().string());
    }

    LOG_TRACE(LogAreaID::Server, "Resource pack manager loaded %zu pack(s)", mPacks.size());
}

const ResourcePack *ResourcePackManager::findById(const std::string &uuid) const {
    for (const ResourcePack &pack: mPacks) {
        if (pack.mUuidString == uuid)
            return &pack;
    }
    return nullptr;
}

#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct ZipEntry {
    std::string mName;
    std::string mData;
};

class ZipArchive {
public:
    static bool listEntries(const std::string &zip, std::vector<ZipEntry> &out);

    static bool extractToDirectory(const std::string &zip, const std::string &destinationDirectory);

    static bool isZip(const std::string &data);
};

class ZipWriter {
public:
    void addFile(const std::string &name, const std::string &data);

    std::string finish() const;

private:
    struct Record {
        std::string mName;
        std::string mData;
        uint32_t mCrc;
        uint32_t mLocalOffset;
    };

    std::vector<Record> mRecords;
    std::string mBuffer;
};

#include "Network/ConnectionRequest.h"

#include <cstdint>

namespace {

    int decodeBase64Char(char c) {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == '+' || c == '-') return 62;
        if (c == '/' || c == '_') return 63;
        return -1;
    }

}

std::string ConnectionRequest::decodeBase64Url(const std::string &value) {
    std::string out;
    out.reserve(value.size() * 3 / 4);

    uint32_t buffer = 0;
    int bits = 0;

    for (char c: value) {
        const int digit = decodeBase64Char(c);
        if (digit < 0)
            continue;

        buffer = (buffer << 6) | (uint32_t) digit;
        bits += 6;

        if (bits >= 8) {
            bits -= 8;
            out.push_back((char) ((buffer >> bits) & 0xff));
        }
    }

    return out;
}

std::string ConnectionRequest::readJwtPayload(const std::string &jwt) {
    const size_t first = jwt.find('.');
    if (first == std::string::npos)
        return std::string();

    const size_t second = jwt.find('.', first + 1);
    if (second == std::string::npos)
        return std::string();

    return decodeBase64Url(jwt.substr(first + 1, second - first - 1));
}

std::string ConnectionRequest::findJsonString(const std::string &json, const std::string &key) {
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
        if (json[i] == '\\' && i + 1 < json.size()) {
            value.push_back(json[i + 1]);
            i++;
            continue;
        }

        if (json[i] == '"')
            return value;

        value.push_back(json[i]);
    }

    return std::string();
}

bool ConnectionRequest::parse(const std::string &authJwt, const std::string &clientJwt) {
    // the identity claims live in the last certificate of the chain
    size_t search = 0;
    std::string identityPayload;

    for (;;) {
        const size_t start = authJwt.find("ey", search);
        if (start == std::string::npos)
            break;

        size_t end = start;
        while (end < authJwt.size() && authJwt[end] != '"')
            end++;

        const std::string payload = readJwtPayload(authJwt.substr(start, end - start));
        if (payload.find("extraData") != std::string::npos)
            identityPayload = payload;

        search = end + 1;
    }

    if (!identityPayload.empty()) {
        mDisplayName = findJsonString(identityPayload, "displayName");
        mIdentity = findJsonString(identityPayload, "identity");
        mXuid = findJsonString(identityPayload, "XUID");
    }

    const std::string clientPayload = readJwtPayload(clientJwt);
    if (!clientPayload.empty()) {
        mDeviceId = findJsonString(clientPayload, "DeviceId");
        mTitleId = findJsonString(clientPayload, "TitleID");

        if (mDisplayName.empty())
            mDisplayName = findJsonString(clientPayload, "ThirdPartyName");
    }

    return !mDisplayName.empty();
}

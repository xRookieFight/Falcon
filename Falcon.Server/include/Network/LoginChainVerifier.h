#pragma once

#include <string>
#include <vector>

class LoginChainVerifier {
public:
    bool verify(const std::string &authJwt);

    bool isSigned() const { return mSigned; }

    const std::string &getIdentity() const { return mIdentity; }

    const std::string &getXuid() const { return mXuid; }

    const std::string &getDisplayName() const { return mDisplayName; }

    const std::string &getTitleId() const { return mTitleId; }

    const std::string &getIdentityPublicKey() const { return mIdentityPublicKey; }

private:
    static std::vector<std::string> extractChain(const std::string &authJwt);

    static std::string extractToken(const std::string &authJwt);

    static std::string xuidToUuid(const std::string &xuid);

    bool verifyToken(const std::string &token);

    static bool splitJwt(const std::string &jwt, std::string &header, std::string &payload, std::string &signature);

    static std::string extractObject(const std::string &json, const std::string &key);

    bool verifyChain(const std::vector<std::string> &chain);

    void extractIdentity(const std::string &payload);

    bool mSigned = false;
    std::string mIdentity;
    std::string mXuid;
    std::string mDisplayName;
    std::string mTitleId;
    std::string mIdentityPublicKey;
};

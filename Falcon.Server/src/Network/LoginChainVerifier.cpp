#include "Network/LoginChainVerifier.h"

#include "Network/ConnectionRequest.h"
#include "Core/Debug/BedrockLog.h"

#include <cstdint>
#include <cstring>

#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/bn.h>
#include <openssl/x509.h>

namespace {

    const char *MOJANG_PUBLIC_KEY_BASE64 =
            "MHYwEAYHKoZIzj0CAQYFK4EEACIDYgAECRXueJeTDqNRRgJi/vlRufByu/2G0i2Ebt6YMar5QX/R0DIIyrJMcUpruK4QveTfJSTp3Shlq4Gk34cD/4GUWwkv0DVuzeuB+tXija7HBxii03NHDbPAD0AKnLr2wdAp";

    EVP_PKEY *parseKey(const std::string &base64) {
        const std::string der = ConnectionRequest::decodeBase64Url(base64);
        if (der.empty())
            return nullptr;

        const unsigned char *pointer = reinterpret_cast<const unsigned char *>(der.data());
        return d2i_PUBKEY(nullptr, &pointer, (long) der.size());
    }

    bool keyEquals(EVP_PKEY *left, EVP_PKEY *right) {
        if (left == nullptr || right == nullptr)
            return false;

        unsigned char *leftDer = nullptr;
        unsigned char *rightDer = nullptr;

        const int leftLength = i2d_PUBKEY(left, &leftDer);
        const int rightLength = i2d_PUBKEY(right, &rightDer);

        bool equal = false;
        if (leftLength > 0 && leftLength == rightLength)
            equal = std::memcmp(leftDer, rightDer, (size_t) leftLength) == 0;

        OPENSSL_free(leftDer);
        OPENSSL_free(rightDer);
        return equal;
    }

    std::string joseToDer(const std::string &signature) {
        if (signature.size() != 96)
            return std::string();

        const unsigned char *raw = reinterpret_cast<const unsigned char *>(signature.data());

        BIGNUM *r = BN_bin2bn(raw, 48, nullptr);
        BIGNUM *s = BN_bin2bn(raw + 48, 48, nullptr);
        if (r == nullptr || s == nullptr) {
            BN_free(r);
            BN_free(s);
            return std::string();
        }

        ECDSA_SIG *sig = ECDSA_SIG_new();
        if (sig == nullptr) {
            BN_free(r);
            BN_free(s);
            return std::string();
        }

        ECDSA_SIG_set0(sig, r, s);

        unsigned char *der = nullptr;
        const int length = i2d_ECDSA_SIG(sig, &der);

        std::string out;
        if (length > 0)
            out.assign(reinterpret_cast<const char *>(der), (size_t) length);

        OPENSSL_free(der);
        ECDSA_SIG_free(sig);
        return out;
    }

    bool verifyEs384(EVP_PKEY *key, const std::string &message, const std::string &derSignature) {
        if (key == nullptr || derSignature.empty())
            return false;

        EVP_MD_CTX *context = EVP_MD_CTX_new();
        if (context == nullptr)
            return false;

        bool valid = false;
        if (EVP_DigestVerifyInit(context, nullptr, EVP_sha384(), nullptr, key) == 1 &&
            EVP_DigestVerifyUpdate(context, message.data(), message.size()) == 1) {
            const int result = EVP_DigestVerifyFinal(
                    context, reinterpret_cast<const unsigned char *>(derSignature.data()), derSignature.size());
            valid = result == 1;
        }

        EVP_MD_CTX_free(context);
        return valid;
    }

}

std::vector<std::string> LoginChainVerifier::extractChain(const std::string &authJwt) {
    std::vector<std::string> chain;

    const std::string pattern = "\"chain\"";
    size_t position = authJwt.find(pattern);
    if (position == std::string::npos)
        return chain;

    position = authJwt.find('[', position + pattern.size());
    if (position == std::string::npos)
        return chain;

    const size_t end = authJwt.find(']', position);
    if (end == std::string::npos)
        return chain;

    std::string token;
    int dots = 0;

    for (size_t i = position; i <= end; i++) {
        const char c = authJwt[i];
        const bool isTokenChar = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                                 (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.';

        if (isTokenChar) {
            token.push_back(c);
            if (c == '.')
                dots++;
            continue;
        }

        if (!token.empty()) {
            if (dots == 2)
                chain.push_back(token);
            token.clear();
            dots = 0;
        }
    }

    if (!token.empty() && dots == 2)
        chain.push_back(token);

    return chain;
}

bool LoginChainVerifier::splitJwt(const std::string &jwt, std::string &header, std::string &payload,
                                  std::string &signature) {
    const size_t first = jwt.find('.');
    if (first == std::string::npos)
        return false;

    const size_t second = jwt.find('.', first + 1);
    if (second == std::string::npos)
        return false;

    header = jwt.substr(0, first);
    payload = jwt.substr(first + 1, second - first - 1);
    signature = jwt.substr(second + 1);
    return true;
}

std::string LoginChainVerifier::extractObject(const std::string &json, const std::string &key) {
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

void LoginChainVerifier::extractIdentity(const std::string &payload) {
    const std::string extraData = extractObject(payload, "extraData");
    const std::string &source = extraData.empty() ? payload : extraData;

    mDisplayName = ConnectionRequest::findJsonString(source, "displayName");
    mIdentity = ConnectionRequest::findJsonString(source, "identity");
    mXuid = ConnectionRequest::findJsonString(source, "XUID");
    mTitleId = ConnectionRequest::findJsonString(source, "titleId");
}

bool LoginChainVerifier::verifyChain(const std::vector<std::string> &chain) {
    EVP_PKEY *currentKey = nullptr;
    EVP_PKEY *mojangKey = parseKey(MOJANG_PUBLIC_KEY_BASE64);
    if (mojangKey == nullptr) {
        LOG_ERROR(LogAreaID::Network, "Failed to parse the Mojang root public key");
        return false;
    }

    bool ok = true;
    std::string lastPayload;

    for (size_t i = 0; i < chain.size(); i++) {
        std::string header;
        std::string payloadPart;
        std::string signaturePart;

        if (!splitJwt(chain[i], header, payloadPart, signaturePart)) {
            LOG_WARN(LogAreaID::Network, "Login chain entry %u is not a valid JWT", (unsigned) i);
            ok = false;
            break;
        }

        const std::string headerJson = ConnectionRequest::decodeBase64Url(header);
        const std::string x5u = ConnectionRequest::findJsonString(headerJson, "x5u");

        EVP_PKEY *expectedKey = parseKey(x5u);
        if (expectedKey == nullptr) {
            LOG_WARN(LogAreaID::Network, "Login chain entry %u has an invalid x5u key", (unsigned) i);
            ok = false;
            break;
        }

        if (currentKey == nullptr) {
            currentKey = expectedKey;
        } else if (!keyEquals(currentKey, expectedKey)) {
            LOG_WARN(LogAreaID::Network, "Received broken login chain at entry %u", (unsigned) i);
            EVP_PKEY_free(expectedKey);
            ok = false;
            break;
        } else {
            EVP_PKEY_free(expectedKey);
        }

        const std::string message = header + "." + payloadPart;
        const std::string derSignature = joseToDer(ConnectionRequest::decodeBase64Url(signaturePart));

        if (!verifyEs384(currentKey, message, derSignature)) {
            LOG_WARN(LogAreaID::Network, "Login chain signature does not match content at entry %u", (unsigned) i);
            ok = false;
            break;
        }

        if (i == 1 && !keyEquals(currentKey, mojangKey)) {
            LOG_WARN(LogAreaID::Network, "The login chain is not signed by Mojang");
            ok = false;
            break;
        }

        const std::string payloadJson = ConnectionRequest::decodeBase64Url(payloadPart);
        lastPayload = payloadJson;

        const std::string identityPublicKey = ConnectionRequest::findJsonString(payloadJson, "identityPublicKey");
        EVP_PKEY *nextKey = parseKey(identityPublicKey);

        EVP_PKEY_free(currentKey);
        currentKey = nextKey;

        if (currentKey == nullptr && i + 1 < chain.size()) {
            LOG_WARN(LogAreaID::Network, "Login chain entry %u has an invalid identityPublicKey", (unsigned) i);
            ok = false;
            break;
        }
    }

    if (ok && !lastPayload.empty()) {
        extractIdentity(lastPayload);
        mIdentityPublicKey = ConnectionRequest::findJsonString(lastPayload, "identityPublicKey");
        mSigned = true;
    }

    EVP_PKEY_free(currentKey);
    EVP_PKEY_free(mojangKey);
    return ok && mSigned;
}

std::string LoginChainVerifier::extractToken(const std::string &authJwt) {
    return ConnectionRequest::findJsonString(authJwt, "Token");
}

bool LoginChainVerifier::verifyToken(const std::string &token) {
    std::string header;
    std::string payloadPart;
    std::string signaturePart;

    if (!splitJwt(token, header, payloadPart, signaturePart))
        return false;

    const std::string payloadJson = ConnectionRequest::decodeBase64Url(payloadPart);

    mXuid = ConnectionRequest::findJsonString(payloadJson, "xid");
    mDisplayName = ConnectionRequest::findJsonString(payloadJson, "xname");
    mIdentityPublicKey = ConnectionRequest::findJsonString(payloadJson, "cpk");
    mIdentity = xuidToUuid(mXuid);
    mSigned = !mXuid.empty();

    return mSigned;
}

std::string LoginChainVerifier::xuidToUuid(const std::string &xuid) {
    if (xuid.empty())
        return std::string();

    const std::hash<std::string> hasher;
    const uint64_t high = hasher("falcon-xuid-high-" + xuid);
    const uint64_t low = hasher("falcon-xuid-low-" + xuid);

    char buffer[40];
    snprintf(buffer, sizeof(buffer), "%08x-%04x-%04x-%04x-%012llx",
             (unsigned int) (high >> 32),
             (unsigned int) ((high >> 16) & 0xffff),
             (unsigned int) (high & 0xffff),
             (unsigned int) ((low >> 48) & 0xffff),
             (unsigned long long) (low & 0xffffffffffffULL));
    return std::string(buffer);
}

bool LoginChainVerifier::verify(const std::string &authJwt) {
    mSigned = false;
    mIdentity.clear();
    mXuid.clear();
    mDisplayName.clear();
    mTitleId.clear();
    mIdentityPublicKey.clear();

    const std::string token = extractToken(authJwt);
    if (!token.empty())
        return verifyToken(token);

    const std::vector<std::string> chain = extractChain(authJwt);

    if (chain.size() == 1) {
        std::string header;
        std::string payloadPart;
        std::string signaturePart;

        if (!splitJwt(chain[0], header, payloadPart, signaturePart))
            return false;

        const std::string payloadJson = ConnectionRequest::decodeBase64Url(payloadPart);
        extractIdentity(payloadJson);
        mIdentityPublicKey = ConnectionRequest::findJsonString(payloadJson, "identityPublicKey");
        mSigned = false;
        return !mDisplayName.empty() || !mIdentity.empty();
    }

    if (chain.size() == 3)
        return verifyChain(chain);

    LOG_WARN(LogAreaID::Network, "Unexpected login chain length %u", (unsigned) chain.size());
    return false;
}

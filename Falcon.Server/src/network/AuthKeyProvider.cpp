#include "network/AuthKeyProvider.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wincrypt.h>
#undef X509_NAME
#undef X509_EXTENSIONS
#undef PKCS7_ISSUER_AND_SERIAL
#undef PKCS7_SIGNER_INFO
#undef OCSP_REQUEST
#undef OCSP_RESPONSE
#else
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#endif

#include "core/debug/BedrockLog.h"
#include "network/ConnectionRequest.h"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <vector>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/evp.h>

#ifdef _WIN32
typedef SOCKET FalconSocket;
#define FALCON_INVALID_SOCKET INVALID_SOCKET
#define FALCON_CLOSE_SOCKET closesocket
#else
typedef int FalconSocket;
#define FALCON_INVALID_SOCKET (-1)
#define FALCON_CLOSE_SOCKET close
#endif

namespace {

    const int HTTP_TIMEOUT_SECONDS = 5;
    const long long KEY_REFRESH_INTERVAL_SECONDS = 30 * 60;
    const long long FAILURE_BACKOFF_SECONDS = 60;
    const size_t MAX_RESPONSE_SIZE = 1024 * 1024;

    const char *MINECRAFT_VERSION_NETWORK = "1.26.40";
    const char *DISCOVERY_URL_PREFIX =
            "https://client.discovery.minecraft-services.net/api/v1.0/discovery/MinecraftPE/builds/";
    const char *AUTHORIZATION_SERVICE_URI_FALLBACK = "https://authorization.franchise.minecraft-services.net";
    const char *OPENID_CONFIGURATION_PATH = "/.well-known/openid-configuration";
    const char *KEYS_PATH = "/.well-known/keys";

    long long currentTime() {
        return (long long) std::time(nullptr);
    }

    std::string findJsonObject(const std::string &json, const std::string &key) {
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

    std::vector<std::string> findJsonArrayObjects(const std::string &json, const std::string &key) {
        std::vector<std::string> objects;

        const std::string pattern = "\"" + key + "\"";
        size_t position = json.find(pattern);
        if (position == std::string::npos)
            return objects;

        position = json.find('[', position + pattern.size());
        if (position == std::string::npos)
            return objects;

        int arrayDepth = 0;
        int objectDepth = 0;
        size_t objectStart = std::string::npos;

        for (size_t i = position; i < json.size(); i++) {
            const char c = json[i];

            if (c == '[') {
                arrayDepth++;
                continue;
            }

            if (c == ']') {
                arrayDepth--;
                if (arrayDepth == 0)
                    break;
                continue;
            }

            if (c == '{') {
                if (objectDepth == 0)
                    objectStart = i;
                objectDepth++;
                continue;
            }

            if (c == '}') {
                objectDepth--;
                if (objectDepth == 0 && objectStart != std::string::npos) {
                    objects.push_back(json.substr(objectStart, i - objectStart + 1));
                    objectStart = std::string::npos;
                }
            }
        }

        return objects;
    }

    bool parseUrl(const std::string &url, std::string &host, std::string &port, std::string &path) {
        const std::string scheme = "https://";
        if (url.compare(0, scheme.size(), scheme) != 0)
            return false;

        const size_t authorityStart = scheme.size();
        size_t authorityEnd = url.find('/', authorityStart);
        if (authorityEnd == std::string::npos) {
            path = "/";
            authorityEnd = url.size();
        } else {
            path = url.substr(authorityEnd);
        }

        std::string authority = url.substr(authorityStart, authorityEnd - authorityStart);
        if (authority.empty())
            return false;

        const size_t colon = authority.find(':');
        if (colon == std::string::npos) {
            host = authority;
            port = "443";
        } else {
            host = authority.substr(0, colon);
            port = authority.substr(colon + 1);
        }

        return !host.empty() && !port.empty();
    }

    void initialiseSockets() {
#ifdef _WIN32
        static bool started = false;
        if (started)
            return;

        WSADATA data;
        WSAStartup(MAKEWORD(2, 2), &data);
        started = true;
#endif
    }

    void setSocketBlocking(FalconSocket descriptor, bool blocking) {
#ifdef _WIN32
        u_long mode = blocking ? 0 : 1;
        ioctlsocket(descriptor, FIONBIO, &mode);
#else
        int flags = fcntl(descriptor, F_GETFL, 0);
        if (flags < 0)
            return;

        if (blocking)
            flags &= ~O_NONBLOCK;
        else
            flags |= O_NONBLOCK;

        fcntl(descriptor, F_SETFL, flags);
#endif
    }

    void setSocketTimeouts(FalconSocket descriptor, int seconds) {
#ifdef _WIN32
        DWORD milliseconds = (DWORD) (seconds * 1000);
        setsockopt(descriptor, SOL_SOCKET, SO_RCVTIMEO, (const char *) &milliseconds, sizeof(milliseconds));
        setsockopt(descriptor, SOL_SOCKET, SO_SNDTIMEO, (const char *) &milliseconds, sizeof(milliseconds));
#else
        struct timeval value;
        value.tv_sec = seconds;
        value.tv_usec = 0;
        setsockopt(descriptor, SOL_SOCKET, SO_RCVTIMEO, &value, sizeof(value));
        setsockopt(descriptor, SOL_SOCKET, SO_SNDTIMEO, &value, sizeof(value));
#endif
    }

    FalconSocket connectWithTimeout(const std::string &host, const std::string &port, int seconds) {
        initialiseSockets();

        struct addrinfo hints;
        std::memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        struct addrinfo *results = nullptr;
        if (getaddrinfo(host.c_str(), port.c_str(), &hints, &results) != 0)
            return FALCON_INVALID_SOCKET;

        FalconSocket connected = FALCON_INVALID_SOCKET;

        for (struct addrinfo *entry = results; entry != nullptr; entry = entry->ai_next) {
            FalconSocket descriptor = socket(entry->ai_family, entry->ai_socktype, entry->ai_protocol);
            if (descriptor == FALCON_INVALID_SOCKET)
                continue;

            setSocketBlocking(descriptor, false);

            bool established = connect(descriptor, entry->ai_addr, (int) entry->ai_addrlen) == 0;

            if (!established) {
                fd_set writable;
                FD_ZERO(&writable);
                FD_SET(descriptor, &writable);

                struct timeval timeout;
                timeout.tv_sec = seconds;
                timeout.tv_usec = 0;

                if (select((int) descriptor + 1, nullptr, &writable, nullptr, &timeout) > 0) {
                    int error = 0;
                    socklen_t length = sizeof(error);
                    if (getsockopt(descriptor, SOL_SOCKET, SO_ERROR, (char *) &error, &length) == 0 && error == 0)
                        established = true;
                }
            }

            if (!established) {
                FALCON_CLOSE_SOCKET(descriptor);
                continue;
            }

            setSocketBlocking(descriptor, true);
            setSocketTimeouts(descriptor, seconds);
            connected = descriptor;
            break;
        }

        freeaddrinfo(results);
        return connected;
    }

    void loadSystemTrustStore(SSL_CTX *context) {
#ifdef _WIN32
        HCERTSTORE store = CertOpenSystemStoreA(0, "ROOT");
        if (store == nullptr)
            return;

        X509_STORE *target = SSL_CTX_get_cert_store(context);
        PCCERT_CONTEXT entry = nullptr;

        while ((entry = CertEnumCertificatesInStore(store, entry)) != nullptr) {
            const unsigned char *pointer = entry->pbCertEncoded;
            X509 *certificate = d2i_X509(nullptr, &pointer, (long) entry->cbCertEncoded);
            if (certificate != nullptr) {
                X509_STORE_add_cert(target, certificate);
                X509_free(certificate);
            }
        }

        CertCloseStore(store, 0);
#else
        SSL_CTX_set_default_verify_paths(context);
#endif
    }

    bool decodeChunkedBody(const std::string &raw, std::string &body) {
        size_t position = 0;

        for (;;) {
            const size_t lineEnd = raw.find("\r\n", position);
            if (lineEnd == std::string::npos)
                return false;

            const std::string sizeLine = raw.substr(position, lineEnd - position);
            const size_t chunkSize = (size_t) strtoul(sizeLine.c_str(), nullptr, 16);
            position = lineEnd + 2;

            if (chunkSize == 0)
                return true;

            if (position + chunkSize > raw.size())
                return false;

            body.append(raw, position, chunkSize);
            position += chunkSize + 2;
        }
    }

    bool parseHttpResponse(const std::string &raw, int &statusCode, std::string &body) {
        const size_t headerEnd = raw.find("\r\n\r\n");
        if (headerEnd == std::string::npos)
            return false;

        const std::string head = raw.substr(0, headerEnd);
        const size_t firstSpace = head.find(' ');
        if (firstSpace == std::string::npos)
            return false;

        statusCode = atoi(head.c_str() + firstSpace + 1);

        std::string lowered = head;
        for (size_t i = 0; i < lowered.size(); i++)
            lowered[i] = (char) tolower((unsigned char) lowered[i]);

        const std::string payload = raw.substr(headerEnd + 4);

        if (lowered.find("transfer-encoding: chunked") != std::string::npos)
            return decodeChunkedBody(payload, body);

        body = payload;
        return true;
    }

    bool httpsGet(const std::string &url, std::string &body) {
        std::string host;
        std::string port;
        std::string path;

        if (!parseUrl(url, host, port, path)) {
            LOG_WARN(LogAreaID::Network, "Cannot fetch authentication data from malformed url %s", url.c_str());
            return false;
        }

        FalconSocket descriptor = connectWithTimeout(host, port, HTTP_TIMEOUT_SECONDS);
        if (descriptor == FALCON_INVALID_SOCKET) {
            LOG_WARN(LogAreaID::Network, "Failed to connect to %s while fetching authentication data", host.c_str());
            return false;
        }

        SSL_CTX *context = SSL_CTX_new(TLS_client_method());
        if (context == nullptr) {
            FALCON_CLOSE_SOCKET(descriptor);
            return false;
        }

        SSL_CTX_set_min_proto_version(context, TLS1_2_VERSION);
        loadSystemTrustStore(context);
        SSL_CTX_set_verify(context, SSL_VERIFY_PEER, nullptr);

        SSL *ssl = SSL_new(context);
        if (ssl == nullptr) {
            SSL_CTX_free(context);
            FALCON_CLOSE_SOCKET(descriptor);
            return false;
        }

        SSL_set_fd(ssl, (int) descriptor);
        SSL_set_tlsext_host_name(ssl, host.c_str());
        SSL_set_hostflags(ssl, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
        SSL_set1_host(ssl, host.c_str());

        bool success = false;

        if (SSL_connect(ssl) == 1) {
            std::string request = "GET " + path + " HTTP/1.1\r\n";
            request += "Host: " + host + "\r\n";
            request += "User-Agent: Falcon\r\n";
            request += "Accept: application/json\r\n";
            request += "Connection: close\r\n\r\n";

            if (SSL_write(ssl, request.data(), (int) request.size()) > 0) {
                std::string raw;
                char buffer[4096];

                for (;;) {
                    const int read = SSL_read(ssl, buffer, sizeof(buffer));
                    if (read <= 0)
                        break;

                    raw.append(buffer, (size_t) read);
                    if (raw.size() > MAX_RESPONSE_SIZE)
                        break;
                }

                int statusCode = 0;
                std::string payload;

                if (parseHttpResponse(raw, statusCode, payload)) {
                    if (statusCode == 200) {
                        body = payload;
                        success = true;
                    } else {
                        LOG_WARN(LogAreaID::Network, "Unexpected http status %d while fetching %s", statusCode,
                                 url.c_str());
                    }
                }
            }
        } else {
            LOG_WARN(LogAreaID::Network, "Tls handshake with %s failed while fetching authentication data",
                     host.c_str());
        }

        SSL_shutdown(ssl);
        SSL_free(ssl);
        SSL_CTX_free(context);
        FALCON_CLOSE_SOCKET(descriptor);
        return success;
    }

    std::string encodeDerLength(size_t length) {
        std::string out;

        if (length <= 0x7f) {
            out.push_back((char) length);
            return out;
        }

        std::string bytes;
        size_t value = length;
        while (value > 0) {
            bytes.insert(bytes.begin(), (char) (value & 0xff));
            value >>= 8;
        }

        out.push_back((char) (0x80 | bytes.size()));
        out.append(bytes);
        return out;
    }

    std::string encodeDerBytes(unsigned char tag, const std::string &data) {
        std::string out;
        out.push_back((char) tag);
        out.append(encodeDerLength(data.size()));
        out.append(data);
        return out;
    }

    std::string encodeDerInteger(const std::string &value) {
        size_t start = 0;
        while (start + 1 < value.size() && value[start] == 0)
            start++;

        std::string trimmed = value.substr(start);
        if (trimmed.empty())
            trimmed.push_back((char) 0);

        if ((unsigned char) trimmed[0] >= 0x80)
            trimmed.insert(trimmed.begin(), (char) 0);

        return encodeDerBytes(0x02, trimmed);
    }

    EVP_PKEY *rsaPublicKeyFromModExp(const std::string &modulusBase64, const std::string &exponentBase64) {
        const std::string modulus = ConnectionRequest::decodeBase64Url(modulusBase64);
        const std::string exponent = ConnectionRequest::decodeBase64Url(exponentBase64);

        if (modulus.empty() || exponent.empty())
            return nullptr;

        const std::string sequence = encodeDerBytes(0x30, encodeDerInteger(modulus) + encodeDerInteger(exponent));

        std::string bitString;
        bitString.push_back((char) 0);
        bitString.append(sequence);

        const unsigned char rsaIdentifier[] = {
                0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86,
                0xf7, 0x0d, 0x01, 0x01, 0x01, 0x05, 0x00
        };

        std::string algorithm(reinterpret_cast<const char *>(rsaIdentifier), sizeof(rsaIdentifier));
        const std::string der = encodeDerBytes(0x30, algorithm + encodeDerBytes(0x03, bitString));

        const unsigned char *pointer = reinterpret_cast<const unsigned char *>(der.data());
        return d2i_PUBKEY(nullptr, &pointer, (long) der.size());
    }

    std::string resolveAuthServiceUri() {
        std::string body;
        const std::string url = std::string(DISCOVERY_URL_PREFIX) + MINECRAFT_VERSION_NETWORK;

        if (!httpsGet(url, body))
            return std::string();

        const std::string result = findJsonObject(body, "result");
        if (result.empty())
            return std::string();

        const std::string environments = findJsonObject(result, "serviceEnvironments");
        if (environments.empty())
            return std::string();

        const std::string auth = findJsonObject(environments, "auth");
        if (auth.empty())
            return std::string();

        const std::string prod = findJsonObject(auth, "prod");
        if (prod.empty())
            return std::string();

        return ConnectionRequest::findJsonString(prod, "serviceUri");
    }

    bool resolveOpenIdConfiguration(const std::string &authServiceUri, std::string &issuer, std::string &jwksUri) {
        std::string body;
        if (!httpsGet(authServiceUri + OPENID_CONFIGURATION_PATH, body))
            return false;

        issuer = ConnectionRequest::findJsonString(body, "issuer");
        jwksUri = ConnectionRequest::findJsonString(body, "jwks_uri");
        return !issuer.empty() && !jwksUri.empty();
    }

}

AuthKeyProvider &AuthKeyProvider::getInstance() {
    static AuthKeyProvider instance;
    return instance;
}

AuthKeyProvider::~AuthKeyProvider() {
    stop();
    _clear();
}

void AuthKeyProvider::_clear() {
    for (std::map<std::string, EVP_PKEY *>::iterator it = mKeys.begin(); it != mKeys.end(); ++it)
        EVP_PKEY_free(it->second);

    mKeys.clear();
}

bool AuthKeyProvider::_fetch(std::map<std::string, EVP_PKEY *> &outKeys, std::string &outIssuer) {
    LOG_TRACE(LogAreaID::Network, "Fetching authentication keys from the Minecraft services");

    std::string authServiceUri = resolveAuthServiceUri();
    if (authServiceUri.empty()) {
        LOG_WARN(LogAreaID::Network, "Service discovery failed, falling back to the well known authorization service");
        authServiceUri = AUTHORIZATION_SERVICE_URI_FALLBACK;
    }

    std::string issuer;
    std::string jwksUri;

    if (!resolveOpenIdConfiguration(authServiceUri, issuer, jwksUri)) {
        issuer = authServiceUri;
        jwksUri = authServiceUri + KEYS_PATH;
    }

    std::string body;
    if (!httpsGet(jwksUri, body)) {
        LOG_ERROR(LogAreaID::Network,
                  "Failed to fetch authentication keys, Xbox players will not be able to authenticate");
        return false;
    }

    std::map<std::string, EVP_PKEY *> keys;

    const std::vector<std::string> entries = findJsonArrayObjects(body, "keys");
    for (size_t i = 0; i < entries.size(); i++) {
        const std::string &entry = entries[i];

        const std::string kty = ConnectionRequest::findJsonString(entry, "kty");
        const std::string use = ConnectionRequest::findJsonString(entry, "use");
        const std::string kid = ConnectionRequest::findJsonString(entry, "kid");

        if (kid.empty())
            continue;

        if (kty != "RSA" || use != "sig") {
            LOG_WARN(LogAreaID::Network, "Authentication key %s is not a signing rsa key, skipping it", kid.c_str());
            continue;
        }

        EVP_PKEY *key = rsaPublicKeyFromModExp(ConnectionRequest::findJsonString(entry, "n"),
                                               ConnectionRequest::findJsonString(entry, "e"));
        if (key == nullptr) {
            LOG_WARN(LogAreaID::Network, "Failed to parse authentication key %s", kid.c_str());
            continue;
        }

        keys.insert(std::make_pair(kid, key));
    }

    if (keys.empty()) {
        LOG_ERROR(LogAreaID::Network, "No usable authentication key was returned by the Minecraft services");
        return false;
    }

    outKeys = keys;
    outIssuer = issuer;

    LOG_TRACE(LogAreaID::Network, "Loaded %u authentication keys from issuer %s", (unsigned) keys.size(),
              issuer.c_str());
    LOG_INFO(LogAreaID::Network, "Signed in to signaling service successfully");
    return true;
}

void AuthKeyProvider::_worker() {
    for (;;) {
        std::unique_lock<std::mutex> guard(mMutex);

        mCondition.wait(guard, [this]() { return !mRunning || mRefreshRequested; });

        if (!mRunning)
            return;

        mRefreshRequested = false;
        guard.unlock();

        std::map<std::string, EVP_PKEY *> keys;
        std::string issuer;
        const bool ok = _fetch(keys, issuer);

        guard.lock();

        if (!mRunning) {
            for (std::map<std::string, EVP_PKEY *>::iterator it = keys.begin(); it != keys.end(); ++it)
                EVP_PKEY_free(it->second);
            return;
        }

        if (ok) {
            _clear();
            mKeys = keys;
            mIssuer = issuer;
            mLastFetch = currentTime();
        } else {
            mLastFailure = currentTime();
        }
    }
}

void AuthKeyProvider::_requestRefresh() {
    if (mRefreshRequested)
        return;

    mRefreshRequested = true;
    mCondition.notify_one();
}

void AuthKeyProvider::start() {
    std::lock_guard<std::mutex> guard(mMutex);

    if (mRunning)
        return;

    mRunning = true;
    mWorker = std::thread(&AuthKeyProvider::_worker, this);
    _requestRefresh();
}

void AuthKeyProvider::stop() {
    {
        std::lock_guard<std::mutex> guard(mMutex);

        if (!mRunning)
            return;

        mRunning = false;
        mCondition.notify_one();
    }

    if (mWorker.joinable())
        mWorker.join();
}

EVP_PKEY *AuthKeyProvider::acquireKey(const std::string &keyId, std::string &issuer) {
    std::lock_guard<std::mutex> guard(mMutex);

    const long long now = currentTime();
    const bool known = mKeys.find(keyId) != mKeys.end();
    const bool stale = mLastFetch < now - KEY_REFRESH_INTERVAL_SECONDS;

    if ((mKeys.empty() || (!known && stale)) && mLastFailure < now - FAILURE_BACKOFF_SECONDS)
        _requestRefresh();

    std::map<std::string, EVP_PKEY *>::iterator it = mKeys.find(keyId);
    if (it == mKeys.end())
        return nullptr;

    issuer = mIssuer;
    EVP_PKEY_up_ref(it->second);
    return it->second;
}

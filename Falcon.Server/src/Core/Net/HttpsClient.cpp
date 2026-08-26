#include "Core/Net/HttpsClient.h"

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

#include "Core/Debug/BedrockLog.h"

#include <cctype>
#include <cstdlib>
#include <cstring>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#ifdef _WIN32
typedef SOCKET FalconHttpSocket;
#define FALCON_HTTP_INVALID_SOCKET INVALID_SOCKET
#define FALCON_HTTP_CLOSE_SOCKET ::closesocket
#else
typedef int FalconHttpSocket;
#define FALCON_HTTP_INVALID_SOCKET (-1)
#define FALCON_HTTP_CLOSE_SOCKET ::close
#endif

namespace {

    const size_t MAX_RESPONSE_SIZE = 1024 * 1024;

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

    void setSocketBlocking(FalconHttpSocket descriptor, bool blocking) {
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

    void setSocketTimeouts(FalconHttpSocket descriptor, int seconds) {
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

    FalconHttpSocket connectWithTimeout(const std::string &host, const std::string &port, int seconds) {
        initialiseSockets();

        struct addrinfo hints;
        std::memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        struct addrinfo *results = nullptr;
        if (getaddrinfo(host.c_str(), port.c_str(), &hints, &results) != 0)
            return FALCON_HTTP_INVALID_SOCKET;

        FalconHttpSocket connected = FALCON_HTTP_INVALID_SOCKET;

        for (struct addrinfo *entry = results; entry != nullptr; entry = entry->ai_next) {
            FalconHttpSocket descriptor = socket(entry->ai_family, entry->ai_socktype, entry->ai_protocol);
            if (descriptor == FALCON_HTTP_INVALID_SOCKET)
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
                FALCON_HTTP_CLOSE_SOCKET(descriptor);
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

}

bool HttpsClient::get(const std::string &url, std::string &body, int timeoutSeconds) {
    std::string host;
    std::string port;
    std::string path;

    if (!parseUrl(url, host, port, path)) {
        LOG_WARN(LogAreaID::Network, "Cannot fetch %s because the url is malformed", url.c_str());
        return false;
    }

    FalconHttpSocket descriptor = connectWithTimeout(host, port, timeoutSeconds);
    if (descriptor == FALCON_HTTP_INVALID_SOCKET) {
        LOG_WARN(LogAreaID::Network, "Failed to connect to %s", host.c_str());
        return false;
    }

    SSL_CTX *context = SSL_CTX_new(TLS_client_method());
    if (context == nullptr) {
        FALCON_HTTP_CLOSE_SOCKET(descriptor);
        return false;
    }

    SSL_CTX_set_min_proto_version(context, TLS1_2_VERSION);
    loadSystemTrustStore(context);
    SSL_CTX_set_verify(context, SSL_VERIFY_PEER, nullptr);

    SSL *ssl = SSL_new(context);
    if (ssl == nullptr) {
        SSL_CTX_free(context);
        FALCON_HTTP_CLOSE_SOCKET(descriptor);
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
        LOG_WARN(LogAreaID::Network, "Tls handshake with %s failed", host.c_str());
    }

    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(context);
    FALCON_HTTP_CLOSE_SOCKET(descriptor);
    return success;
}

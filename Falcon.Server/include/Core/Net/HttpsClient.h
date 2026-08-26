#pragma once

#include <string>

namespace HttpsClient {

    const int DEFAULT_TIMEOUT_SECONDS = 5;

    bool get(const std::string &url, std::string &body, int timeoutSeconds = DEFAULT_TIMEOUT_SECONDS);

}

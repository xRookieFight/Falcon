#pragma once

#include <string>

struct ActorSize {
    float mWidth;
    float mHeight;
};

namespace ActorSizeTable {

ActorSize getSize(const std::string &identifier);

}

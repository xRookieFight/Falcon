#pragma once

#include "Core/Math/Vector3f.h"

class Level;

struct LiquidContact {
    bool water = false;
    bool lava = false;
    bool bubble = false;
    bool dragDown = false;
    bool eyeSubmerged = false;
    Vector3f flow;
};

class LiquidBlocksFetch {
public:
    static LiquidContact at(Level &level, const Vector3f &feet);
};

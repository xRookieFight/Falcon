#pragma once

#include <cstdint>
#include <string>

class BiomeDefinitionData {
public:
    std::string mName;
    float mTemperature = 0.5f;
    float mDownfall = 0.5f;
    float mFoliageSnow = 0.0f;
    float mDepth = 0.1f;
    float mScale = 0.1f;
    int32_t mMapWaterColorArgb = (int32_t) 0xFF44AFF5;
    bool mRain = true;
};

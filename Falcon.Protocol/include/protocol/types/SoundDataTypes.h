#pragma once

class FadeSoundData {
public:
    float mTargetVolume = 0.0f;
    float mDuration = 0.0f;
};

class SetVolumeSoundData {
public:
    float mVolume = 0.0f;
};

class SetPitchSoundData {
public:
    float mPitch = 0.0f;
};

class SeekToSoundData {
public:
    float mSeconds = 0.0f;
};

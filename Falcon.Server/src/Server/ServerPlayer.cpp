#include "Server/ServerPlayer.h"

ServerPlayer::ServerPlayer(const NetworkIdentifier &id, uint64_t runtimeId)
        : mId(id), mRuntimeId(runtimeId), mLoginState(LoginState::Connecting) {}

#include "Block/BlockActor.h"

Tag BlockActor::saveWithPosition() const {
    Tag data = saveNbt();
    data.putString("id", getBlockActorId());
    data.putInt("x", mPosition.x);
    data.putInt("y", mPosition.y);
    data.putInt("z", mPosition.z);
    return data;
}

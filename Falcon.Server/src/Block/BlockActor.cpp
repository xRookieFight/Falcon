#include "Block/BlockActor.h"

Tag BlockActor::getSpawnCompound() const {
    Tag data = Tag::ofCompound();
    data.putString("id", getBlockActorId());
    data.putInt("x", mPosition.x);
    data.putInt("y", mPosition.y);
    data.putInt("z", mPosition.z);
    return data;
}

Tag BlockActor::saveWithPosition() const {
    Tag data = saveNbt();
    data.putString("id", getBlockActorId());
    data.putInt("x", mPosition.x);
    data.putInt("y", mPosition.y);
    data.putInt("z", mPosition.z);
    return data;
}

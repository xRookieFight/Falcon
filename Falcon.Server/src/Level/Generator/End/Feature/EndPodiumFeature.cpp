#include "Level/Generator/End/Feature/EndPodiumFeature.h"

#include "Level/Generator/End/Object/EndPodiumObject.h"
#include "Level/Generator/Feature/BlockManager.h"
#include "Level/Level.h"
#include "Level/LevelChunk.h"

EndPodiumFeature::EndPodiumFeature() {
}

const char *EndPodiumFeature::name() const {
    return "the_end_exit_portal";
}

void EndPodiumFeature::apply(ChunkGenerateContext &context) {
    if (mRoot == nullptr)
        return;

    LevelChunk &chunk = context.getChunk();
    if (chunk.getX() != 1 || chunk.getZ() != 0)
        return;

    BlockManager object(context.getLevel());
    EndPodiumObject exitPortal(false);
    exitPortal.generate(object, mRandom, 0, chunk.getHeight(0, 0), 0);
    queueObject(object);
}

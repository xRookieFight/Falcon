#include "Level/Generator/Feature/Ore/StoneOnlyOreFeature.h"

bool StoneOnlyOreFeature::canBeReplaced(const BlockState &state) const {
    return state == stoneState();
}

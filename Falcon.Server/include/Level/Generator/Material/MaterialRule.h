#pragma once

#include "Block/BlockState.h"
#include "Level/Generator/Density/DensityFunction.h"

#include <functional>

using MaterialRule = std::function<const BlockState *(FunctionContext &)>;

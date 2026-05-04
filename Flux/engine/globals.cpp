// =============================================================================
// globals.cpp  —  Flux Engine Global State  (overhauled)
// =============================================================================
#include "globals.h"
#include "gui/viewport/heiarchy.h"
#include "mechanics/luabridge.h"

namespace Flux {

    GameState gameState = Stopped;

    std::vector<std::string> logs;

    float deltaTime   = 0.0f;
    float elapsedTime = 0.0f;

    std::unordered_map<std::string, bool> inputState;

    Heiarchy    heiarchy;
    ScriptEngine scriptEngine;

} // namespace Flux
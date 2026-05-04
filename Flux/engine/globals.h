// =============================================================================
// globals.h  —  Flux Engine Global State  (overhauled)
// =============================================================================
#pragma once
#include <vector>
#include <string>
#include <unordered_map>

namespace Flux {

    // -------------------------------------------------------------------------
    // Engine play state
    // -------------------------------------------------------------------------
    enum GameState { Stopped, Playing, Paused };
    extern GameState gameState;

    // -------------------------------------------------------------------------
    // Output log (shown in the Output panel)
    // -------------------------------------------------------------------------
    extern std::vector<std::string> logs;

    // -------------------------------------------------------------------------
    // Time — updated every frame by Window::update()
    //   deltaTime   : seconds between the last two frames
    //   elapsedTime : seconds since Play was pressed (reset on Stop)
    // -------------------------------------------------------------------------
    extern float deltaTime;
    extern float elapsedTime;

    // -------------------------------------------------------------------------
    // Input — populated every frame by Window::update() from GLFW callbacks
    //   Key name → true while held, false otherwise
    //   e.g.  inputState["W"], inputState["Space"], inputState["LeftShift"]
    // -------------------------------------------------------------------------
    extern std::unordered_map<std::string, bool> inputState;

    // -------------------------------------------------------------------------
    // Forward-declare subsystems so other headers can reference them
    // -------------------------------------------------------------------------
    class Heiarchy;
    class ScriptEngine;

    extern Heiarchy    heiarchy;
    extern ScriptEngine scriptEngine;

} // namespace Flux
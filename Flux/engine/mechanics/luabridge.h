// =============================================================================
// LuaBridge.h  —  Flux Script Engine  (fixed)
// =============================================================================
// Bug fixed:
//   • makeVec3 captured `inst` by reference. `inst` lives inside an
//     unordered_map. Any insert into that map (a second node starting) can
//     reallocate and move all values, leaving the captured reference dangling.
//     Crash or silent memory corruption on frame 2+.
//     FIX: makeVec3 now captures only `lua_State*` (raw pointer, stable) and
//     rebuilds the table from scratch each call — no reference to inst at all.
//
//   • The dead placeholder `getPosition` that returned sol::lua_nil was
//     overwritten immediately after. Removed.
//
// All Lua API:
//   print(...)            → Output window  [LUA]
//   log("msg")            → Output window  [LUA]
//   warn("msg")           → Output window  [WARN]
//
//   time.delta()          → float: seconds since last frame
//   time.elapsed()        → float: seconds since Play pressed
//
//   input.keyDown("W")    → bool: key held this frame
//   input.keyPressed("W") → bool: key went down this frame (edge detect)
//
//   self.name             → string: node name
//   self.tag              → string: node tag
//   self.getPosition()    → {x,y,z}
//   self.setPosition(x,y,z)
//   self.getRotation()    → {x,y,z}  (Euler degrees)
//   self.setRotation(x,y,z)
//   self.getScale()       → {x,y,z}
//   self.setScale(x,y,z)
//   self.isSelected()     → bool
//   self.destroy()        → marks node for removal at end of frame
//
// Lifecycle (define these in your .lua file):
//   function onStart()        called once when Play is pressed
//   function onUpdate(dt)     called every frame while Playing (dt = delta seconds)
//   function onStop()         called when engine stops
// =============================================================================
#pragma once

#include <sol/sol.hpp>
#include <string>
#include <unordered_map>
#include "SceneNode.h"
#include "globals.h"
#include <filesystem>

namespace Flux {

// ---------------------------------------------------------------------------
// ScriptInstance — one isolated Lua VM per SceneNode
// ---------------------------------------------------------------------------
struct ScriptInstance {
    sol::state              lua;
    sol::protected_function fnStart;
    sol::protected_function fnUpdate;
    sol::protected_function fnStop;

    // For edge-detection in input.keyPressed()
    std::unordered_map<std::string, bool> prevKeyState;

    bool started  = false;
    bool hasError = false;  // once true, stops spamming the same error every frame
};

// ---------------------------------------------------------------------------
// ScriptEngine
// ---------------------------------------------------------------------------
class ScriptEngine {
public:

    void Init() { /* per-instance setup happens lazily in _InitInstance */ }

    // -----------------------------------------------------------------------
    // RunScript — call on Play for every node that hasScript
    // -----------------------------------------------------------------------
    void RunScript(SceneNode& node) {
        // Erase before inserting so the map doesn't hold a stale instance
        instances_.erase(&node);
        ScriptInstance& inst = instances_[&node];
        _InitInstance(inst, node);
        if (!inst.hasError)
            _LoadAndStart(inst, node);
    }

    // -----------------------------------------------------------------------
    // UpdateScript — call every frame while Playing
    // -----------------------------------------------------------------------
    void UpdateScript(SceneNode& node, float dt) {
        auto it = instances_.find(&node);
        if (it == instances_.end()) return;

        ScriptInstance& inst = it->second;
        if (!inst.started || inst.hasError) return;
        if (!inst.fnUpdate.valid())          return;

        // Snapshot previous key states for edge detection
        std::unordered_map<std::string, bool> prevSnap = inst.prevKeyState;

        auto result = inst.fnUpdate(dt);
        if (!result.valid()) {
            sol::error err = result;
            _LogError(node, "onUpdate", err.what());
            inst.hasError = true;
        }

        // Update prevKeyState after the frame
        for (auto& [key, _] : inst.prevKeyState) {
            auto ci = Flux::inputState.find(key);
            inst.prevKeyState[key] = (ci != Flux::inputState.end()) && ci->second;
        }
    }

    // -----------------------------------------------------------------------
    // StopScript — call onStop() and mark as not started
    // -----------------------------------------------------------------------
    void StopScript(SceneNode& node) {
        auto it = instances_.find(&node);
        if (it == instances_.end()) return;

        ScriptInstance& inst = it->second;
        if (inst.started && !inst.hasError && inst.fnStop.valid()) {
            auto result = inst.fnStop();
            if (!result.valid()) {
                sol::error err = result;
                _LogError(node, "onStop", err.what());
            }
        }
        inst.started  = false;
        inst.hasError = false;
    }

    // -----------------------------------------------------------------------
    // StopAll — called when the engine Stop button is pressed
    // -----------------------------------------------------------------------
    void StopAll(std::vector<SceneNode>& nodes) {
        for (auto& node : nodes)
            if (node.hasScript) StopScript(node);
        instances_.clear();
    }

    // -----------------------------------------------------------------------
    // ReloadScript — hot-reload while the engine is running
    // -----------------------------------------------------------------------
    void ReloadScript(SceneNode& node) {
        StopScript(node);
        RunScript(node);
    }

private:
    std::unordered_map<SceneNode*, ScriptInstance> instances_;

    // -----------------------------------------------------------------------
    // _InitInstance — build a fresh Lua VM and bind the entire Flux API
    // -----------------------------------------------------------------------
    void _InitInstance(ScriptInstance& inst, SceneNode& node) {
        inst.lua.open_libraries(
            sol::lib::base,
            sol::lib::math,
            sol::lib::string,
            sol::lib::table
        );

        // ---- print / log / warn → Output window ---------------------------
        inst.lua.set_function("print", [](sol::variadic_args va) {
            std::string out = "[LUA] ";
            for (auto v : va) {
                auto s = v.as<sol::optional<std::string>>();
                out += s ? *s
                         : std::string(sol::type_name(v.lua_state(),
                                                      v.get_type()));
                out += "  ";
            }
            Flux::logs.push_back(out);
        });

        inst.lua.set_function("log", [](const std::string& msg) {
            Flux::logs.push_back("[LUA] " + msg);
        });

        inst.lua.set_function("warn", [](const std::string& msg) {
            Flux::logs.push_back("[WARN] " + msg);
        });

        // ---- time API -----------------------------------------------------
        inst.lua.create_named_table("time");
        inst.lua["time"]["delta"]   = []() -> float { return Flux::deltaTime;   };
        inst.lua["time"]["elapsed"] = []() -> float { return Flux::elapsedTime; };

        // ---- input API ----------------------------------------------------
        // Capture raw pointer to prevKeyState — stable because RunScript erases
        // and re-inserts before calling _InitInstance, so the map entry won't
        // move until StopAll clears the whole map.
        auto* prevKeys = &inst.prevKeyState;

        inst.lua.create_named_table("input");

        inst.lua["input"]["keyDown"] = [](const std::string& key) -> bool {
            auto it = Flux::inputState.find(key);
            return it != Flux::inputState.end() && it->second;
        };

        inst.lua["input"]["keyPressed"] = [prevKeys](const std::string& key) -> bool {
            // Register this key for tracking if we haven't seen it yet
            if (prevKeys->find(key) == prevKeys->end())
                (*prevKeys)[key] = false;

            bool cur  = false;
            auto ci   = Flux::inputState.find(key);
            if (ci != Flux::inputState.end()) cur = ci->second;

            bool prev = (*prevKeys)[key];
            return cur && !prev;  // true only on the rising edge
        };

        // ---- self (node) API ----------------------------------------------
        // CRITICAL: capture only nodePtr and L (raw pointers).
        // NEVER capture inst or &inst — the unordered_map can reallocate.
        SceneNode* nodePtr = &node;
        lua_State* L       = inst.lua.lua_state();

        // makeVec3 uses only the raw lua_State* — no dependency on inst.
        auto makeVec3 = [L](float x, float y, float z) -> sol::table {
            sol::state_view sv(L);
            sol::table t = sv.create_table();
            t["x"] = x;  t["y"] = y;  t["z"] = z;
            return t;
        };

        auto selfTable = inst.lua.create_named_table("self");

        selfTable["name"] = node.name;   // string copied by value — safe
        selfTable["tag"]  = node.tag;

        selfTable["getPosition"] = [nodePtr, makeVec3]() mutable {
            return makeVec3(nodePtr->position.x,
                            nodePtr->position.y,
                            nodePtr->position.z);
        };
        selfTable["setPosition"] = [nodePtr](float x, float y, float z) {
            nodePtr->position = { x, y, z };
        };

        selfTable["getRotation"] = [nodePtr, makeVec3]() mutable {
            return makeVec3(nodePtr->rotation.x,
                            nodePtr->rotation.y,
                            nodePtr->rotation.z);
        };
        selfTable["setRotation"] = [nodePtr](float x, float y, float z) {
            nodePtr->rotation = { x, y, z };
        };

        selfTable["getScale"] = [nodePtr, makeVec3]() mutable {
            return makeVec3(nodePtr->scale.x,
                            nodePtr->scale.y,
                            nodePtr->scale.z);
        };
        selfTable["setScale"] = [nodePtr](float x, float y, float z) {
            nodePtr->scale = { x, y, z };
        };

        selfTable["isSelected"] = [nodePtr]() -> bool {
            return nodePtr->isSelected;
        };

        // self.destroy() — node is removed from heiarchy at end of frame
        selfTable["destroy"] = [nodePtr]() {
            nodePtr->pendingDestroy = true;
        };
    }

    // -----------------------------------------------------------------------
    // _LoadAndStart — parse + execute the .lua file, then call onStart()
    // -----------------------------------------------------------------------
    void _LoadAndStart(ScriptInstance& inst, SceneNode& node) {
        if (node.scriptPath.empty()) {
            Flux::logs.push_back("[WARN] Node '" + node.name + "' has no script assigned!");
            return;
        }

        // 2. We gonna convert it to an absolute path to be 100% sure the engine finds it
        // Translating: We make sure the "Recipe" is found in the current "Project Kitchen"
        std::filesystem::path absolutePath = std::filesystem::absolute(node.scriptPath);

        if (!std::filesystem::exists(absolutePath)) {
            Flux::logs.push_back("[ERROR] Could not find script at: " + absolutePath.string());
            inst.hasError = true;
            return;
        }

        // 3. Now we tell Sol2 to load the file from that specific project location
        auto loadResult = inst.lua.load_file(absolutePath.string());
        
        if (!loadResult.valid()) {
            sol::error err = loadResult;
            _LogError(node, "parse", err.what());
            inst.hasError = true;
            return;
        }
        // 2. Execute top-level (defines onStart / onUpdate / onStop)
        auto execResult = loadResult();
        if (!execResult.valid()) {
            sol::error err = execResult;
            _LogError(node, "exec", err.what());
            inst.hasError = true;
            return;
        }

        // 3. Cache lifecycle handles
        inst.fnStart  = inst.lua["onStart"];
        inst.fnUpdate = inst.lua["onUpdate"];
        inst.fnStop   = inst.lua["onStop"];

        // 4. Call onStart() — user setup happens here
        if (inst.fnStart.valid()) {
            auto result = inst.fnStart();
            if (!result.valid()) {
                sol::error err = result;
                _LogError(node, "onStart", err.what());
                inst.hasError = true;
                return;
            }
        }

        inst.started = true;
        Flux::logs.push_back("[LUA] '" + node.name + "' started");
    }

    void _LogError(const SceneNode& node,
                   const std::string& phase,
                   const std::string& what)
    {
        Flux::logs.push_back("[ERROR] '" + node.name + "' ["
                             + phase + "] " + what);
    }
};

} // namespace Flux
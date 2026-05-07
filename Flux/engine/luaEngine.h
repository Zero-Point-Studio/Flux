#pragma once

#include <lua.hpp>
#include <lua.h>

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>
#include <string>
#include <iostream>
#include "gui/viewport/output.h"

namespace Flux {
    class Output;

    class LuaEngine {
        public:
            LuaEngine() = default;
            ~LuaEngine() = default;

            bool isRunning = false;

            void init();
            void step();

            void start();
            void stop();

            void runScript(const std::string& code);

        private:
            sol::state lua;
            sol::protected_function luaOnStart;
            sol::protected_function luaOnUpdate;
            sol::protected_function luaOnEnd;
    };
}
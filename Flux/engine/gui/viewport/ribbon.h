#pragma once
#include <string>
#include <vector>

#include "luaEngine.h"
#include "../texteditor.h"

namespace Flux {
	enum ToolMode {
		TOOL_MOVE = 0,
		TOOL_ROTATE = 1,
		TOOL_SCALE = 2
	};

	class LuaEngine;
	class TextEditor;

	class Ribbon {
		public:
			void renderRibbon();

			LuaEngine* luaEnginePtr = nullptr;
			TextEditor* textEditorPtr = nullptr;
		private:
			void drawFileMenu();
			void drawEditMenu();
			void drawProjectControls();
			void drawTransformTools();
	};
}
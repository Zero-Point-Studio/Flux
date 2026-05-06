#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <filesystem>
#include "imgui.h"
#include "viewport/explorer.h"

namespace Flux {
    class Explorer;

    class TextEditor {
        public:
            Explorer* explorerPtr = nullptr;
            
            std::string scriptName;
            std::string originalContent;

            bool isVisible = false;
            bool isUnsaved = false;

            std::filesystem::path filePath;

            ImFont* editorFont = nullptr;

            void SetupFont();

            void Render();

            void Indent();

            void saveFile();

            void autoIndent();

            void openFile();

        private:
            char textBuf[16384] = "";
    };
}
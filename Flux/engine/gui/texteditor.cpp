#include "texteditor.h"
#include <cstring>
#include <iostream>

namespace Flux {
    void TextEditor::SetupFont() {
        ImGuiIO& io = ImGui::GetIO();
        if (editorFont == nullptr) {
            editorFont = io.Fonts->AddFontFromFileTTF("assets/fonts/JetBrainsMono/JetBrainsMonoNL-Regular.ttf", 22.0f);
        }
    }

    void TextEditor::Render() {
        if (!isVisible) return;

        isUnsaved = (std::string(textBuf) != originalContent);

        ImVec4 graycolor = ImVec4(0.1f, 0.1f, 0.1f, 1.0f); // Gray color preset 
        
        ImGui::PushStyleColor(ImGuiCol_WindowBg, graycolor);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, graycolor);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

        std::string windowTitle = "Text Editor - " + scriptName + (isUnsaved ? "*" : "") + "###UniqueEditorID";

        ImGui::Begin(windowTitle.c_str(),  &isVisible);

        if (ImGui::IsWindowHovered()) {
			ImGui::SetWindowFocus();
		}

        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_S)) {
            saveFile();
        }
 
        if (editorFont) ImGui::PushFont(editorFont);

        if (ImGui::InputTextMultiline("##TextEditor", textBuf, sizeof(textBuf), ImVec2(-1.0f, -1.0f), ImGuiInputTextFlags_AllowTabInput)) {
            if (isUnsaved) {
                std::filesystem::path backupDir = filePath.parent_path() / ".flux" / "backups";
                std::filesystem::create_directories(backupDir);

                std::ofstream backup(backupDir / (scriptName + ".tmp"));
                backup << textBuf;
                backup.close();
            }
        }
        
        if (editorFont) ImGui::PopFont();
        
        ImGui::End();

        ImGui::PopStyleColor(3);
    }

    void TextEditor::openFile() {
        try {
            std::cout << "Editor Address: " << this << std::endl;

            std::filesystem::path backupPath = filePath.parent_path() / ".flux" / "backups" / (scriptName + ".tmp");

            if (std::filesystem::exists(backupPath)) {
                std::ifstream backupFile(backupPath);
                std::stringstream buf;
                buf << backupFile.rdbuf();
                std::string content = buf.str();

                std::strncpy(textBuf, content.c_str(), sizeof(textBuf) - 1);
                textBuf[content.size()] = '\0';

                this->isUnsaved = true;

                std::cout << "RECOVERED UNSAVED WORK" << std::endl;
                return;
            }

            std::ifstream file(filePath);
            
            if (file.is_open()) {
                std::stringstream buf;
                buf << file.rdbuf();
                this->originalContent = buf.str();
                std::string content = buf.str();

                if (content.size() < sizeof(textBuf)) {
                    std::strncpy(textBuf, content.c_str(), sizeof(textBuf) - 1);

                    textBuf[content.size()] = '\0';

                    this->isUnsaved = false;
                }

                file.close();
            }
        } catch (const std::exception& e) {
            std::cerr << "FAILED TO OPEN " << scriptName << ": " << e.what() << std::endl;
        }
    }

    void TextEditor::saveFile() {
        std::ofstream file(this->filePath);

        if (file.is_open()) {
            file << textBuf;
            file.close();

            this->originalContent = std::string(textBuf);

            std::filesystem::path backupPath = filePath.parent_path() / ".flux" / "backups" / (scriptName + ".tmp");
            if (std::filesystem::exists(backupPath)) {
                std::filesystem::remove(backupPath);

                if (explorerPtr != nullptr) {
                    explorerPtr->scanForBackups();
                }
            }

            std::cout << "SAVED " << scriptName << " SUCCESSFULLY!" << std::endl;
            std::cout << sizeof(textBuf) << std::endl;
        } else {
            std::cerr << "ERROR: FAILED TO SAVE " << scriptName << std::endl;
        }
    }
}
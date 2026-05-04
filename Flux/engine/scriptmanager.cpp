// =============================================================================
// scriptmanager.cpp  —  Flux Script Text Editor  (overhauled)
// =============================================================================
#include "scriptmanager.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace Flux {

// =============================================================================
//  Public API
// =============================================================================

void ScriptManager::OpenFile(const std::filesystem::path& path) {
    // --- Deduplicate: if already open, just switch to that tab --------------
    for (int i = 0; i < (int)tabs_.size(); ++i) {
        if (tabs_[i].path == path) {
            activeTab_ = i;
            isOpen     = true;
            return;
        }
    }

    // --- Read file from disk ------------------------------------------------
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[ScriptManager] Could not open: " << path << std::endl;
        return;
    }
    std::ostringstream buf;
    buf << file.rdbuf();

    // --- Build tab ----------------------------------------------------------
    ScriptTab tab;
    tab.name       = path.filename().string();
    tab.path       = path;
    tab.textBuffer = buf.str();
    tab.isDirty    = false;

    tabs_.push_back(std::move(tab));
    activeTab_ = (int)tabs_.size() - 1;
    isOpen     = true;
}

void ScriptManager::SaveTab(int index) {
    if (index < 0 || index >= (int)tabs_.size()) return;
    ScriptTab& tab = tabs_[index];

    std::ofstream file(tab.path);
    if (!file.is_open()) {
        std::cerr << "[ScriptManager] Could not save: " << tab.path << std::endl;
        return;
    }
    file << tab.textBuffer;
    tab.isDirty = false;
}

void ScriptManager::SaveActiveTab() {
    SaveTab(activeTab_);
}

void ScriptManager::CloseTab(int index) {
    if (index < 0 || index >= (int)tabs_.size()) return;

    if (tabs_[index].isDirty) {
        // Defer actual close — the unsaved dialog will handle it
        pendingCloseTab_ = index;
        return;
    }

    tabs_.erase(tabs_.begin() + index);

    // Clamp active tab
    if (tabs_.empty())           activeTab_ = -1;
    else if (activeTab_ >= (int)tabs_.size())
                                 activeTab_ = (int)tabs_.size() - 1;
}

std::filesystem::path ScriptManager::GetActiveScriptPath() const {
    if (activeTab_ >= 0 && activeTab_ < (int)tabs_.size())
        return tabs_[activeTab_].path;
    return {};
}

// =============================================================================
//  RenderEditor  —  called every frame
// =============================================================================
void ScriptManager::RenderEditor() {
    if (!isOpen) return;

    ImGui::SetNextWindowSize(ImVec2(700, 500), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Text Editor", &isOpen)) {
        ImGui::End();
        return;
    }

    // Handle Ctrl+S globally for this window
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
            SaveActiveTab();
        }
    }

    _DrawTabBar();
    _DrawEditorArea();
    _DrawStatusBar();
    _DrawUnsavedDialog();

    ImGui::End();
}

// =============================================================================
//  Private helpers
// =============================================================================

void ScriptManager::_DrawTabBar() {
    if (!ImGui::BeginTabBar("##ScriptTabs", ImGuiTabBarFlags_Reorderable)) return;

    for (int i = 0; i < (int)tabs_.size(); ++i) {
        ScriptTab& tab = tabs_[i];

        // Build the visible tab label
        std::string label = tab.name + (tab.isDirty ? " *" : "") + "##tab" + std::to_string(i);

        ImGuiTabItemFlags flags = ImGuiTabItemFlags_None;
        bool open = true;

        if (ImGui::BeginTabItem(label.c_str(), &open, flags)) {
            activeTab_ = i;
            ImGui::EndTabItem();
        }

        // User clicked the × button on the tab
        if (!open) CloseTab(i);
    }

    ImGui::EndTabBar();
}

void ScriptManager::_DrawEditorArea() {
    if (activeTab_ < 0 || activeTab_ >= (int)tabs_.size()) {
        ImGui::TextDisabled("No file open. Drag a .lua file from the Explorer.");
        return;
    }

    ScriptTab& tab = tabs_[activeTab_];

    // Dark background for the code area
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.10f, 0.10f, 0.10f, 1.0f));
    ImGui::SetWindowFontScale(2.5f);

    // Reserve space: full window minus status bar height (~22 px)
    ImVec2 editorSize = ImGui::GetContentRegionAvail();
    editorSize.y -= 24.0f;

    // Ensure the buffer has capacity for typing (InputTextMultiline needs slack)
    tab.textBuffer.reserve(tab.textBuffer.size() + 512);

    ImGuiInputTextFlags editorFlags =
        ImGuiInputTextFlags_AllowTabInput |
        ImGuiInputTextFlags_CallbackEdit;   // lets us detect edits

    // Use InputTextMultiline with a callback so we know when the text changed
    struct EditCallback {
        bool* dirty;
        static int Fn(ImGuiInputTextCallbackData*) { return 0; }
    };

    bool edited = ImGui::InputTextMultiline(
        "##editor",
        tab.textBuffer.data(),
        tab.textBuffer.capacity(),
        editorSize,
        ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_CallbackAlways,
        [](ImGuiInputTextCallbackData* data) -> int {
            // Resize underlying string if ImGui grew it
            if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
                std::string* buf = (std::string*)data->UserData;
                buf->resize(data->BufTextLen);
                data->Buf     = buf->data();
                data->BufSize = (int)buf->capacity();
            }
            return 0;
        },
        &tab.textBuffer
    );

    // Keep std::string length in sync with what ImGui wrote
    tab.textBuffer.resize(std::strlen(tab.textBuffer.c_str()));

    if (edited) tab.isDirty = true;

    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();
}

void ScriptManager::_DrawStatusBar() {
    if (activeTab_ < 0 || activeTab_ >= (int)tabs_.size()) return;
    const ScriptTab& tab = tabs_[activeTab_];

    // Count lines
    int lines = (int)std::count(tab.textBuffer.begin(), tab.textBuffer.end(), '\n') + 1;

    ImGui::Separator();
    ImGui::Text("  %s  |  %d lines  |  %s",
        tab.path.string().c_str(),
        lines,
        tab.isDirty ? "Unsaved" : "Saved"
    );
    ImGui::SameLine();

    // Quick save button on the right
    float btnW = 60.0f;
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - btnW - 8.0f);
    if (ImGui::Button("Save", ImVec2(btnW, 0))) {
        SaveActiveTab();
    }
}

void ScriptManager::_DrawUnsavedDialog() {
    if (pendingCloseTab_ < 0) return;

    ImGui::OpenPopup("Unsaved Changes");
    if (ImGui::BeginPopupModal("Unsaved Changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        const ScriptTab& tab = tabs_[pendingCloseTab_];
        ImGui::Text("'%s' has unsaved changes.", tab.name.c_str());
        ImGui::Text("Save before closing?");
        ImGui::Separator();

        if (ImGui::Button("Save & Close", ImVec2(120, 0))) {
            SaveTab(pendingCloseTab_);
            tabs_.erase(tabs_.begin() + pendingCloseTab_);
            if (activeTab_ >= (int)tabs_.size())
                activeTab_ = (int)tabs_.size() - 1;
            pendingCloseTab_ = -1;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Discard", ImVec2(80, 0))) {
            tabs_.erase(tabs_.begin() + pendingCloseTab_);
            if (activeTab_ >= (int)tabs_.size())
                activeTab_ = (int)tabs_.size() - 1;
            pendingCloseTab_ = -1;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(70, 0))) {
            pendingCloseTab_ = -1;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

} // namespace Flux
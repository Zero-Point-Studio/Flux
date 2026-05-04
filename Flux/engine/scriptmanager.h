// =============================================================================
// scriptmanager.h  —  Flux Script Text Editor  (overhauled)
// =============================================================================
#pragma once
#include "imgui.h"
#include <string>
#include <filesystem>
#include <vector>

namespace Flux {

    // -------------------------------------------------------------------------
    // One tab = one open .lua file
    // -------------------------------------------------------------------------
    struct ScriptTab {
        std::string            name;       // display name (filename)
        std::filesystem::path  path;       // full path on disk
        std::string            textBuffer; // current in-editor content
        bool                   isDirty = false; // unsaved changes → shows "*"
    };

    // -------------------------------------------------------------------------
    // ScriptManager — embedded Lua text editor panel
    // Improvements:
    //   • Correct per-tab dirty tracking (editing marks only the active tab)
    //   • Ctrl+S saves only the active tab
    //   • CloseTab with unsaved-changes guard
    //   • OpenFile deduplicates (won't open the same file twice)
    //   • Line-count display in status bar
    // -------------------------------------------------------------------------
    class ScriptManager {
    public:
        bool isOpen = false;

        // Draw the editor window — call every frame
        void RenderEditor();

        // Open a file into a new tab (deduplicates automatically)
        void OpenFile(const std::filesystem::path& path);

        // Save the currently active tab to disk
        void SaveActiveTab();

        // Save a specific tab
        void SaveTab(int index);

        // Close a tab (prompts if dirty — handled inside RenderEditor)
        void CloseTab(int index);

        // Returns the script path of the active tab, or empty string
        std::filesystem::path GetActiveScriptPath() const;

    private:
        std::vector<ScriptTab> tabs_;
        int                    activeTab_ = -1;

        // Pending close request (set when user clicks × on a dirty tab)
        int pendingCloseTab_ = -1;

        void _DrawTabBar();
        void _DrawEditorArea();
        void _DrawStatusBar();
        void _DrawUnsavedDialog();
    };

} // namespace Flux
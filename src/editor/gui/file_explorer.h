#ifndef FILE_EXPLORER_H
#define FILE_EXPLORER_H

#include <ImGuiFileDialog/ImGuiFileDialog.h>
#include <imgui.h>

namespace Panels {
//---------------------------------------------------------------------------
// This struct keeps dialog state alive between frames
// - non-owned singleton
//---------------------------------------------------------------------------
inline constexpr const char* kDlgKey = "FileExplorerDlg";    // internal ID
inline constexpr const char* kDlgCaption = "File Explorer";  // visible tab
inline constexpr const char* kDlgWinName =
    "File Explorer###FileExplorerDlg";  // caption###key

struct FileExplorerPanel {
  IGFD::FileDialog dialog_;
  bool opened_ = false;

  void Draw() {
    if (!opened_) {
      IGFD::FileDialogConfig cfg{};
      cfg.path = std::filesystem::current_path().string();  // asset dir
      cfg.flags = 0;                                        // allows docking

      dialog_.OpenDialog(kDlgKey, kDlgCaption, ".*", cfg);
      opened_ = true;
    }

    const ImGuiWindowFlags win_flags =
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar;

    if (dialog_.Display(kDlgKey, win_flags, {0, 0}, {FLT_MAX, FLT_MAX})) {
      if (dialog_.IsOk()) {
        const std::string path = dialog_.GetFilePathName();
        /* Selection should handled here */
      }
      dialog_.Close();
    }
  }
};

inline void FileExplorer() {
  static FileExplorerPanel panel;
  panel.Draw();
}

}  // namespace Panels

#endif  // FILE_EXPLORER_H
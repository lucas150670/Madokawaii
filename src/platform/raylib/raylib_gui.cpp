//
// Created by madoka on 2026/1/9.
//

#include "Madokawaii/platform/gui.h"

#include <raylib.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#undef RAYGUI_IMPLEMENTATION

#define GUI_WINDOW_FILE_DIALOG_IMPLEMENTATION
#include "gui_window_file_dialog.h"

namespace Madokawaii::Platform::Gui {

    static ::Rectangle ToRayRect(Shape::Rectangle r) {
        return { r.x, r.y, r.width, r.height };
    }

    void InitGui() {
        GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
    }

    void SetStyleTextSize(int size) {
        GuiSetStyle(DEFAULT, TEXT_SIZE, size);
    }

    void SetStyleDefault() {
        GuiLoadStyleDefault();
    }

    bool Button(Shape::Rectangle bounds, const char* text) {
        return GuiButton(ToRayRect(bounds), text) != 0;
    }

    bool TextBox(Shape::Rectangle bounds, char* text, int textSize, bool editMode) {
        return GuiTextBox(ToRayRect(bounds), text, textSize, editMode) != 0;
    }

    void Label(Shape::Rectangle bounds, const char* text) {
        GuiLabel(ToRayRect(bounds), text);
    }

    void Panel(Shape::Rectangle bounds, const char* title) {
        GuiPanel(ToRayRect(bounds), title);
    }

    void GroupBox(Shape::Rectangle bounds, const char* text) {
        GuiGroupBox(ToRayRect(bounds), text);
    }

    void StatusBar(Shape::Rectangle bounds, const char* text) {
        GuiStatusBar(ToRayRect(bounds), text);
    }

    FileDialogState InitFileDialog(const char* initPath) {
        FileDialogState state{};
        auto* rlState = new GuiWindowFileDialogState();
        *rlState = InitGuiWindowFileDialog(initPath);
        state.implementationDefined = rlState;
        state.windowActive = false;
        state.selectFilePressed = false;
        return state;
    }

    void UpdateFileDialog(FileDialogState& state) {
        if (!state.implementationDefined) return;

        auto* rlState = static_cast<GuiWindowFileDialogState*>(state.implementationDefined);

        GuiWindowFileDialog(rlState);

        state.windowActive = rlState->windowActive;

        if (rlState->SelectFilePressed) {
            state.selectFilePressed = true;
            state.selectedPath = std::string(rlState->dirPathText) + "/" +
                std::string(rlState->fileNameText);
            rlState->SelectFilePressed = false;
        }
    }

    void UnloadFileDialog(FileDialogState& state) {
        if (state.implementationDefined) {
            auto* rlState = static_cast<GuiWindowFileDialogState*>(state.implementationDefined);
            delete rlState;
            state.implementationDefined = nullptr;
        }
    }

    void OpenFileDialog(FileDialogState& state) {
        if (!state.implementationDefined) return;
        auto* rlState = static_cast<GuiWindowFileDialogState*>(state.implementationDefined);
        rlState->windowActive = true;
        state.windowActive = true;
        state.selectFilePressed = false;
    }

    std::string GetSelectedFilePath(const FileDialogState& state) {
        return state.selectedPath;
    }

    void Lock() {
        GuiLock();
    }

    void Unlock() {
        GuiUnlock();
    }

    bool IsLocked() {
        return GuiIsLocked() != 0;
    }

}
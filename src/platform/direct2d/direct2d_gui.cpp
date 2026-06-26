//
// Direct2D backend immediate-mode GUI subset.
//

#include "direct2d_platform.h"

#include "Madokawaii/platform/gui.h"
#include "Madokawaii/platform/graphics.h"

#include <algorithm>
#include <cstring>

#include <commdlg.h>
#include <d2d1helper.h>

namespace Madokawaii::Platform::Gui {
    using Direct2D::FileDialogData;

    namespace {
        void DrawRect(Shape::Rectangle bounds, Graphics::Color fill, Graphics::Color border) {
            auto* renderTarget = Direct2D::RenderTarget();
            if (!renderTarget) return;

            auto* fillBrush = Direct2D::CreateBrush(fill);
            auto* borderBrush = Direct2D::CreateBrush(border);
            const auto rect = D2D1::RectF(bounds.x, bounds.y, bounds.x + bounds.width, bounds.y + bounds.height);

            if (fillBrush) renderTarget->FillRectangle(rect, fillBrush);
            if (borderBrush) renderTarget->DrawRectangle(rect, borderBrush, 1.0f);

            Direct2D::SafeRelease(fillBrush);
            Direct2D::SafeRelease(borderBrush);
        }

        void DrawGuiText(const char* text, Shape::Rectangle bounds, Graphics::Color color, bool centered = false) {
            if (!text) return;

            const auto fontSize = Direct2D::GetState().guiTextSize;
            int x = static_cast<int>(bounds.x + 8.0f);
            if (centered) {
                const auto width = Graphics::MeasureText(text, fontSize);
                x = static_cast<int>(bounds.x + (bounds.width - width) * 0.5f);
            }
            const auto y = static_cast<int>(bounds.y + (bounds.height - fontSize) * 0.5f - 1.0f);
            Graphics::DrawText(text, x, y, fontSize, color);
        }

        FileDialogData* AsDialogData(FileDialogState& state) {
            return static_cast<FileDialogData*>(state.implementationDefined);
        }

        const FileDialogData* AsDialogData(const FileDialogState& state) {
            return static_cast<const FileDialogData*>(state.implementationDefined);
        }

        void AppendText(char* text, int textSize, const std::wstring& typedText) {
            if (!text || textSize <= 1 || typedText.empty()) return;

            std::string updated = text;
            updated += Direct2D::WideToUtf8(typedText);
            if (updated.size() >= static_cast<std::size_t>(textSize)) {
                updated.resize(static_cast<std::size_t>(textSize - 1));
            }

            std::memset(text, 0, static_cast<std::size_t>(textSize));
            std::memcpy(text, updated.c_str(), updated.size());
        }
    }

    void InitGui() {
        Direct2D::GetState().guiTextSize = 16;
    }

    void SetStyleTextSize(int size) {
        Direct2D::GetState().guiTextSize = std::max(1, size);
    }

    void SetStyleDefault() {
        Direct2D::GetState().guiTextSize = 16;
    }

    bool Button(Shape::Rectangle bounds, const char* text) {
        const auto locked = Direct2D::GetState().guiLocked;
        const auto hover = Direct2D::IsMouseOver(bounds);
        const auto down = hover && Direct2D::IsLeftMouseDown();

        Graphics::Color fill = locked ? Graphics::Color{52, 52, 52, 255}
            : down ? Graphics::Color{86, 86, 98, 255}
            : hover ? Graphics::Color{74, 74, 88, 255}
            : Graphics::Color{58, 58, 68, 255};
        DrawRect(bounds, fill, Graphics::Color{130, 130, 145, 255});
        DrawGuiText(text, bounds, locked ? Graphics::M_GRAY : Graphics::M_RAYWHITE, true);

        return !locked && hover && Direct2D::IsLeftMouseReleased();
    }

    bool TextBox(Shape::Rectangle bounds, char* text, int textSize, bool editMode) {
        const auto locked = Direct2D::GetState().guiLocked;
        const auto hover = Direct2D::IsMouseOver(bounds);
        const auto& platformState = Direct2D::GetState();

        DrawRect(
            bounds,
            editMode ? Graphics::Color{26, 26, 32, 255} : Graphics::Color{18, 18, 24, 255},
            hover || editMode ? Graphics::Color{160, 160, 180, 255} : Graphics::Color{92, 92, 104, 255});
        DrawGuiText(text, bounds, locked ? Graphics::M_GRAY : Graphics::M_LIGHTGRAY);

        if (!locked && editMode && text && textSize > 0) {
            if (platformState.backspacePressed) {
                const auto length = std::strlen(text);
                if (length > 0) text[length - 1] = '\0';
            }
            AppendText(text, textSize, platformState.typedText);

            const auto caretX = bounds.x + 8.0f + static_cast<float>(Graphics::MeasureText(text, platformState.guiTextSize));
            auto* renderTarget = Direct2D::RenderTarget();
            auto* brush = Direct2D::CreateBrush(Graphics::M_RAYWHITE);
            if (renderTarget && brush) {
                renderTarget->DrawLine(
                    D2D1::Point2F(caretX, bounds.y + 7.0f),
                    D2D1::Point2F(caretX, bounds.y + bounds.height - 7.0f),
                    brush,
                    1.0f);
            }
            Direct2D::SafeRelease(brush);
        }

        if (locked) return false;
        if (!editMode && hover && Direct2D::IsLeftMouseReleased()) return true;
        if (editMode && platformState.enterPressed) return true;
        if (editMode && Direct2D::IsLeftMousePressed() && !hover) return true;
        return false;
    }

    void Label(Shape::Rectangle bounds, const char* text) {
        DrawGuiText(text, bounds, Graphics::M_LIGHTGRAY);
    }

    void Panel(Shape::Rectangle bounds, const char* title) {
        DrawRect(bounds, Graphics::Color{24, 24, 30, 220}, Graphics::Color{90, 90, 104, 255});
        if (title && title[0] != '\0') {
            StatusBar(Shape::Rectangle{bounds.x, bounds.y, bounds.width, 28.0f}, title);
        }
    }

    void GroupBox(Shape::Rectangle bounds, const char* text) {
        DrawRect(bounds, Graphics::M_BLANK, Graphics::Color{90, 90, 104, 255});
        if (text && text[0] != '\0') {
            Graphics::DrawText(text, static_cast<int>(bounds.x + 8.0f), static_cast<int>(bounds.y - 10.0f), Direct2D::GetState().guiTextSize, Graphics::M_LIGHTGRAY);
        }
    }

    void StatusBar(Shape::Rectangle bounds, const char* text) {
        DrawRect(bounds, Graphics::Color{42, 42, 50, 255}, Graphics::Color{90, 90, 104, 255});
        DrawGuiText(text, bounds, Graphics::M_LIGHTGRAY);
    }

    FileDialogState InitFileDialog(const char* initPath) {
        FileDialogState state{};
        auto* data = new FileDialogData;
        data->initialPath = Direct2D::Utf8ToWide(initPath ? initPath : "");
        state.implementationDefined = data;
        return state;
    }

    void UpdateFileDialog(FileDialogState&) {
    }

    void UnloadFileDialog(FileDialogState& state) {
        delete AsDialogData(state);
        state.implementationDefined = nullptr;
        state.windowActive = false;
    }

    void OpenFileDialog(FileDialogState& state) {
        const auto* data = AsDialogData(state);
        if (!data) return;

        state.windowActive = true;
        state.selectFilePressed = false;

        std::vector<wchar_t> fileName(4096, L'\0');
        OPENFILENAMEW openFileName{};
        openFileName.lStructSize = sizeof(openFileName);
        openFileName.hwndOwner = Direct2D::GetState().window;
        openFileName.lpstrFile = fileName.data();
        openFileName.nMaxFile = static_cast<DWORD>(fileName.size());
        openFileName.lpstrInitialDir = data->initialPath.empty() ? nullptr : data->initialPath.c_str();
        openFileName.lpstrFilter = L"All files\0*.*\0";
        openFileName.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetOpenFileNameW(&openFileName)) {
            state.selectedPath = Direct2D::WideToUtf8(std::wstring(fileName.data()));
            state.selectFilePressed = true;
        }

        state.windowActive = false;
    }

    std::string GetSelectedFilePath(const FileDialogState& state) {
        return state.selectedPath;
    }

    void Lock() {
        Direct2D::GetState().guiLocked = true;
    }

    void Unlock() {
        Direct2D::GetState().guiLocked = false;
    }

    bool IsLocked() {
        return Direct2D::GetState().guiLocked;
    }
}

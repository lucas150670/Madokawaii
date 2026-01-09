//
// Created by madoka on 2026/1/9.
//

#ifndef MADOKAWAII_PLATFORM_GUI_H
#define MADOKAWAII_PLATFORM_GUI_H

#include <string>
#include "Madokawaii/platform/shape.h"

namespace Madokawaii::Platform::Gui {

    // 文件对话框状态
    struct FileDialogState {
        void* implementationDefined{ nullptr };
        bool windowActive{ false };
        bool selectFilePressed{ false };
        std::string selectedPath;
    };

    // 初始化 GUI 系统
    void InitGui();

    // 设置 GUI 样式
    void SetStyleTextSize(int size);
    void SetStyleDefault();

    // 按钮控件
    // 返回 true 表示按钮被点击
    bool Button(Shape::Rectangle bounds, const char* text);

    // 文本框控件 (可编辑)
    // 返回 true 表示正在编辑
    bool TextBox(Shape::Rectangle bounds, char* text, int textSize, bool editMode);

    // 标签控件
    void Label(Shape::Rectangle bounds, const char* text);

    // 面板控件 (带标题的容器)
    void Panel(Shape::Rectangle bounds, const char* title);

    // 分组框控件
    void GroupBox(Shape::Rectangle bounds, const char* text);

    // 状态栏
    void StatusBar(Shape::Rectangle bounds, const char* text);

    // 文件对话框相关
    FileDialogState InitFileDialog(const char* initPath);
    void UpdateFileDialog(FileDialogState& state);
    void UnloadFileDialog(FileDialogState& state);

    // 打开文件对话框
    void OpenFileDialog(FileDialogState& state);

    // 获取选中的文件路径
    std::string GetSelectedFilePath(const FileDialogState& state);

    // 锁定/解锁 GUI (用于模态对话框)
    void Lock();
    void Unlock();
    bool IsLocked();

}

#endif //MADOKAWAII_PLATFORM_GUI_H
#pragma once
#include <wx/wx.h>
#include <wx/sizer.h>
#include <vector>

// 自定义按钮类
class sorButton : public wxWindow
{
public:
    /**
     * @brief 构造函数
     *
     * @param parent 父窗口
     * @param id ID
     * @param bitmap 要显示的位图
     * @param label 显示的文本
     * @param group 同组按钮的引用（用于互斥逻辑）
     */
    sorButton(wxWindow *parent, wxWindowID id,
              const wxBitmap &bitmap, const wxString &label,
              std::vector<sorButton *> &group);
    void SetSelected(bool selected);

private:
    wxBitmap m_bitmap;         // 原生位图（用于缩放）
    wxBitmap m_normalBitmap;   // 缩放后的正常位图（彩色）
    wxBitmap m_selectedBitmap; // 缩放后的选中位图（单色，颜色为父框架背景色）
    wxString m_label;
    bool m_isSelected;
    std::vector<sorButton *> &m_group;
    wxSize m_iconSize; // 缩放后的实际尺寸

    // 绘制事件处理
    void OnPaint(wxPaintEvent &evt);
    // 鼠标点击事件
    void OnMouseClick(wxMouseEvent &evt);

    // 生成缩放后的位图，并生成选中版本
    void PrepareBitmaps(const wxBitmap &bitmap);
    // 将彩色位图替换为单色
    wxBitmap CreateMonoBitmap(const wxBitmap &src, const wxColour &colur);

    // 禁止拷贝，包含引用成员
    sorButton(const sorButton &) = delete;
    sorButton &operator=(const sorButton &) = delete;
};
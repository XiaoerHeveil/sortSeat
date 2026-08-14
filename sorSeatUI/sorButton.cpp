#include "sorButton.h"
#include <wx/dc.h>
#include <wx/dcmemory.h>
#include <wx/colour.h>
#include <wx/image.h>

static const int ICON_SIZE = 36;
static const int PADDING = 8; // 左右内边距

sorButton::sorButton(wxWindow *parent, wxWindowID id,
                     const wxBitmap &bitmap, const wxString &label,
                     std::vector<sorButton *> &group)
    : wxWindow(parent, id, wxDefaultPosition, wxSize(120, 90)),
      m_bitmap(bitmap),
      m_label(label),
      m_isSelected(false),
      m_group(group)
{
    // 设置较大字体
    wxFont font(20, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    SetFont(font);

    PrepareBitmaps(bitmap);

    Bind(wxEVT_PAINT, &sorButton::OnPaint, this);
    Bind(wxEVT_LEFT_DOWN, &sorButton::OnMouseClick, this);

    SetMinSize(wxSize(90, 64));
}

void sorButton::SetSelected(bool selected)
{
    if (m_isSelected != selected)
    {
        m_isSelected = selected;
        // 触发重绘
        Refresh();
    }
}

void sorButton::PrepareBitmaps(const wxBitmap& bitmap) {
    if (!bitmap.IsOk())
        return;
    wxImage img = bitmap.ConvertToImage();
    if (!img.IsOk())
        return;
    
    // 等比例缩放，使变成为ICON_SIZE
    int w = img.GetWidth(), h = img.GetHeight();
    double ratio = (double)ICON_SIZE / std::max(w, h);
    int newW = std::max(1, (int)(w * ratio));
    int newH = std::max(1, (int)(h * ratio));
    img.Rescale(newW, newH, wxIMAGE_QUALITY_HIGH);

    // 保留正常位图（保留Alpha通道）
    m_normalBitmap = wxBitmap(img);

    // 生成选中位图（单色，颜色为父框架背景色）
    wxColour bg = GetParent()->GetBackgroundColour();
    m_selectedBitmap = CreateMonoBitmap(wxBitmap(img), bg);
}

void sorButton::OnPaint(wxPaintEvent &evt)
{
    wxPaintDC dc(this);
    wxSize size = GetSize();

    wxColour parentBg = GetParent()->GetBackgroundColour();

    // ---- 1. 背景 ----
    if (m_isSelected)
    {
        dc.SetBrush(wxBrush(*wxWHITE));
        dc.SetPen(wxPen(*wxWHITE));
    }
    else
    {
        dc.SetBrush(wxBrush(parentBg));
        dc.SetPen(wxPen(parentBg));
    }
    dc.DrawRectangle(0, 0, size.GetWidth(), size.GetHeight());

    // ---- 2. 准备图标和文本 ----
    wxBitmap *useBitmap = m_isSelected ? &m_selectedBitmap : &m_normalBitmap;
    wxColour textColor = m_isSelected ? parentBg : *wxBLACK;
    dc.SetTextForeground(textColor);
    dc.SetFont(GetFont());

    wxCoord tw, th;
    dc.GetTextExtent(m_label, &tw, &th);

    // ---- 3. 垂直居中基准 ----
    int centerY = size.GetHeight() / 2;

    // ---- 4. 图标：垂直居中，水平靠左（留内边距） ----
    if (useBitmap->IsOk())
    {
        int iconX = PADDING;
        int iconY = 4; // centerY - m_iconSize.GetHeight() / 2;
        dc.DrawBitmap(*useBitmap, iconX, iconY, true);
    }

    // ---- 5. 文本：垂直居中，水平靠右（留内边距） ----
    int textX = size.GetWidth() - tw - PADDING;
    int textY = centerY - th / 2;
    dc.DrawText(m_label, textX, textY);
}

wxBitmap sorButton::CreateMonoBitmap(const wxBitmap& src, const wxColour& colour) {
    wxImage img = src.ConvertToImage();
    if (!img.HasAlpha())
        return src;
    
    // 直接替换所有不透明像素颜色
    unsigned char *data = img.GetData();
    unsigned char *alpha = img.GetAlpha();
    int w = img.GetWidth(), h = img.GetHeight();
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int idx = (y * w + x) * 3;
            if (alpha[y*w+x] > 0) {
                data[idx] = colour.Red();
                data[idx + 1] = colour.Green();
                data[idx + 2] = colour.Blue();
            }
        }
    }
    return wxBitmap(img);
}

void sorButton::OnMouseClick(wxMouseEvent &evt)
{
    // 将同组所有按钮取消选中
    for (auto *btn : m_group)
    {
        if (btn != this) {
            btn->SetSelected(false);
        }
    }
    // 选中当前按钮
    SetSelected(true);

    // 发送wxEVT_BUTTON事件，以便外部切换页面
    wxCommandEvent event(wxEVT_BUTTON, GetId());
    event.SetEventObject(this);
    ProcessWindowEvent(event);
}
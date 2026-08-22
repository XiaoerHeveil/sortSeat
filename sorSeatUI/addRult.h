#pragma once
#include <wx/wx.h>

class addSetSeatPanel;
class MainFrame;

class addRult : public wxFrame
{
private:
    wxPanel *basicLevelPanel = nullptr;
    wxScrolledWindow *presetsRultPanel = nullptr;
    wxScrolledWindow *definitionRultPanel = nullptr;
    wxPanel *ButtonPanel = nullptr;
    wxButton *confirmButton = nullptr;
    wxButton *ClearRultButton = nullptr;

    MainFrame *m_mainFrame = nullptr;             // 父窗口（主框架），用于回传规则

    addSetSeatPanel *m_presetTemplate = nullptr; // 预设模板
    addSetSeatPanel *m_dragClone = nullptr;      // 拖拽中的克隆
    wxBoxSizer *m_definitionSizer = nullptr;     // 定义面板的垂直 sizer

    wxPoint m_dragStart = wxPoint(0, 0);
    bool m_isDragging = false;

    void DestroyDragClone();
    void ClearAllRult();

public:
    addRult(wxWindow *parent);

    wxString addSetSeat(const wxString &name, const int &row, const int &col);

    // 处理面板拖动事件
    void OnMouseDown(wxMouseEvent &evt);
    void OnMouseMove(wxMouseEvent &evt);
    void OnMouseUp(wxMouseEvent &evt);
    void OnMouseLost(wxMouseCaptureLostEvent &evt);

    // 确认按钮：收集定义面板中的规则并回传
    void OnConfirm(wxCommandEvent &evt);
};
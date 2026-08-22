#pragma once
#include <wx/wx.h>

class addSetSeatPanel;

class addRult : public wxFrame
{
    private:
        wxPanel *basicLevelPanel = nullptr;
        wxScrolledWindow *presetsRultPanel = nullptr;
        wxScrolledWindow *definitionRultPanel = nullptr;

        addSetSeatPanel *m_presetTemplate = nullptr; // 预设模板
        addSetSeatPanel *m_dragClone = nullptr;      // 拖拽中的克隆
        wxBoxSizer *m_definitionSizer = nullptr;     // 定义面板的垂直 sizer

        wxPoint m_dragStart = wxPoint(0,0);
        bool m_isDragging = false;

        void DestroyDragClone();

    public:
        addRult(wxWindow *parent);

        void addSetSeat(const wxString &name, const int &row, const int &col);

        // 处理面板拖动事件
        void OnMouseDown(wxMouseEvent &evt);
        void OnMouseMove(wxMouseEvent &evt);
        void OnMouseUp(wxMouseEvent &evt);
        void OnMouseLost(wxMouseCaptureLostEvent &evt);
};
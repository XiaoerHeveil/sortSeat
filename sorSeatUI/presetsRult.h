#pragma once
#include <wx/wx.h>
#include <wx/spinctrl.h>

class addSetSeatPanel : public wxPanel
{
    public:
        addSetSeatPanel(wxWindow *parent, wxWindowID id = wxID_ANY,
                        const wxPoint &pos = wxDefaultPosition,
                        const wxSize &size = wxDefaultSize,
                        long style = wxTAB_TRAVERSAL);
        wxString getName();
        int getRow();
        int getCollom();
        // 设置内部输入控件是否可编辑（预设模板设为不可编辑）
        void SetEditable(bool editable);
        // 克隆方法：创建一个与当前对象数据相同的新面板
        addSetSeatPanel *Clone(wxWindow *newParent) const;

    private:
        wxStaticText *Text1 = nullptr;
        wxStaticText *Text2 = nullptr;
        wxStaticText *Text3 = nullptr;
        wxStaticText *Text4 = nullptr;
        wxTextCtrl *Name = nullptr;
        wxSpinCtrl *Row = nullptr;
        wxSpinCtrl *Collom = nullptr;
};
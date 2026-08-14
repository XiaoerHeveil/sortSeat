#include "SeatPanel.h"
#include <wx/sizer.h>
#include <wx/stattext.h>

SeatPanel::SeatPanel(wxWindow *parent, const wxString &name) : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(180, 60))
{
    SetBackgroundColour(wxColour(30, 145, 255));
    // 创建姓名文本，居中显示
    wxStaticText *text = new wxStaticText(this, wxID_ANY, name,
                                          wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
    wxFont font = text->GetFont();
    font.SetPointSize(14);
    text->SetFont(font);
    wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(text, 1, wxALIGN_CENTER);
    SetSizer(sizer);

    Bind(wxEVT_PAINT, &SeatPanel::OnPaint, this);
}

void SeatPanel::OnPaint(wxPaintEvent &evt)
{
    wxPaintDC dc(this);
    wxRect rect = GetClientRect();
    // 绘制黑色内边框，线宽2px
    dc.SetPen(wxPen(*wxBLACK, 2));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawRectangle(rect);
}
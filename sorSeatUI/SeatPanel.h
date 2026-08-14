#pragma once
#include <wx/panel.h>
#include <wx/dcclient.h>
#include <vector>

class SeatPanel : public wxPanel
{
public:
    SeatPanel(wxWindow *parent, const wxString &name);
    void OnPaint(wxPaintEvent &evt);
};
#include "presetsRult.h"

addSetSeatPanel::addSetSeatPanel(wxWindow *parent, wxWindowID id, const wxPoint &pos, const wxSize &size, long style)
    : wxPanel(parent, id, pos, size, style)
{
    // 1#       指定（）坐在第_列，第_行
    this->SetBackgroundColour(wxColour(8, 224, 220)); // #08E0DC
    wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    Text1 = new wxStaticText(this, wxID_ANY, L"1#               指定 ");
    Name = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
    Text2 = new wxStaticText(this, wxID_ANY, L" 坐在第 ");
    Row = new wxSpinCtrl(this, wxID_ANY, wxEmptyString,
                         wxDefaultPosition, wxSize(60, -1), wxSP_ARROW_KEYS | wxSP_WRAP, 1, 12);
    Text3 = new wxStaticText(this, wxID_ANY, L" 列，第 ");
    Collom = new wxSpinCtrl(this, wxID_ANY, wxEmptyString,
                            wxDefaultPosition, wxSize(60, -1), wxSP_ARROW_KEYS | wxSP_WRAP, 1, 12);
    Text4 = new wxStaticText(this, wxID_ANY, L" 行");
    // 添加布局
    sizer->Add(Text1, 0, wxEXPAND | wxALL, 5);
    // sizer->AddStretchSpacer();
    sizer->Add(Name, 0, wxEXPAND | wxALL, 5);
    sizer->Add(Text2, 0, wxEXPAND | wxALL, 5);
    sizer->Add(Row, 0, wxEXPAND | wxALL, 5);
    sizer->Add(Text3, 0, wxEXPAND | wxALL, 5);
    sizer->Add(Collom, 0, wxEXPAND | wxALL, 5);
    sizer->Add(Text4, 0, wxEXPAND | wxALL, 5);
    this->SetSizer(sizer);
}

wxString addSetSeatPanel::getName()
{
    return Name->GetValue();
}

int addSetSeatPanel::getRow()
{
    return Row->GetValue();
}

int addSetSeatPanel::getCollom()
{
    return Collom->GetValue();
}

void addSetSeatPanel::SetEditable(bool editable)
{
    Name->Enable(editable);
    Row->Enable(editable);
    Collom->Enable(editable);
}

addSetSeatPanel *addSetSeatPanel::Clone(wxWindow *newParent) const
{
    addSetSeatPanel *clone = new addSetSeatPanel(newParent, wxID_ANY,
                                                 wxDefaultPosition, this->GetSize());

    clone->Name->SetValue(this->Name->GetValue());
    clone->Row->SetValue(this->Row->GetValue());
    clone->Collom->SetValue(this->Collom->GetValue());

    return clone;
}
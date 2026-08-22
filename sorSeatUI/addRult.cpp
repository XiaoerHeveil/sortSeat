#include "addRult.h"
#include "presetsRult.h"
#include "mappingRult.h"
#include "MainFrame.h"
#include <wx/spinctrl.h>

addRult::addRult(wxWindow *parent)
    : wxFrame(parent, wxID_ANY, L"添加规则", wxDefaultPosition, wxSize(600, 800))
{
    m_mainFrame = dynamic_cast<MainFrame *>(parent);

    // 设置窗口背景颜色
    SetBackgroundColour(wxColour(210, 210, 210));

    // 创建一个垂直布局的基层面板
    basicLevelPanel = new wxPanel(this);
    wxBoxSizer *basicLevelSizer = new wxBoxSizer(wxVERTICAL);

    presetsRultPanel = new wxScrolledWindow(basicLevelPanel, wxID_ANY);
    presetsRultPanel->SetBackgroundColour(wxColour(230, 230, 230));
    presetsRultPanel->SetScrollRate(5, 5);
    wxBoxSizer *presetsRultSizer = new wxBoxSizer(wxVERTICAL);

    m_presetTemplate = new addSetSeatPanel(presetsRultPanel, wxID_ANY);
    m_presetTemplate->SetEditable(false); // 预设模板不可编辑
    // 添加到预设规则面板布局
    presetsRultSizer->Add(m_presetTemplate, 0, wxEXPAND | wxALL, 5);
    presetsRultPanel->SetSizer(presetsRultSizer);

    definitionRultPanel = new wxScrolledWindow(basicLevelPanel, wxID_ANY);
    definitionRultPanel->SetBackgroundColour(*wxWHITE);
    m_definitionSizer = new wxBoxSizer(wxVERTICAL);
    definitionRultPanel->SetSizer(m_definitionSizer);
    definitionRultPanel->SetScrollRate(5, 5);

    ButtonPanel = new wxPanel(basicLevelPanel, wxID_ANY, wxDefaultPosition, wxSize(-1, 80));
    wxBoxSizer *ButtonSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton *ConfirmBtn = new wxButton(ButtonPanel, wxID_ANY, L"确认", wxDefaultPosition, wxSize(100, 60));
    ConfirmBtn->SetBackgroundColour(wxColour(235, 226, 11)); // #EBE20B
    ConfirmBtn->Bind(wxEVT_BUTTON, &addRult::OnConfirm, this);
    wxButton *ClearRultBtn = new wxButton(ButtonPanel, wxID_ANY, L"清除", wxDefaultPosition, wxSize(100, 60));
    ClearRultBtn->SetBackgroundColour(wxColour(94, 94, 94)); // #5E5E5E
    // 添加按钮到布局
    ButtonSizer->AddStretchSpacer();
    ButtonSizer->Add(ConfirmBtn, 0, wxEXPAND | wxALL, 10);
    ButtonSizer->Add(ClearRultBtn, 0, wxEXPAND | wxALL, 10);
    ButtonPanel->SetSizer(ButtonSizer);

    // 将这三个面板添加到基层面板
    basicLevelSizer->Add(presetsRultPanel, 1, wxEXPAND);
    basicLevelSizer->Add(definitionRultPanel, 3, wxEXPAND);
    basicLevelSizer->Add(ButtonPanel, 0, wxEXPAND);
    basicLevelPanel->SetSizer(basicLevelSizer);

    // 根布局：让基层面板铺满整个窗口
    wxBoxSizer *rootSizer = new wxBoxSizer(wxVERTICAL);
    rootSizer->Add(basicLevelPanel, 1, wxEXPAND);
    SetSizer(rootSizer);

    // 绑定模板的拖拽事件（子控件被禁用后，点击会落到模板面板上）
    m_presetTemplate->Bind(wxEVT_LEFT_DOWN, &addRult::OnMouseDown, this);
    m_presetTemplate->Bind(wxEVT_MOTION, &addRult::OnMouseMove, this);
    m_presetTemplate->Bind(wxEVT_LEFT_UP, &addRult::OnMouseUp, this);
    m_presetTemplate->Bind(wxEVT_MOUSE_CAPTURE_LOST, &addRult::OnMouseLost, this);
}

wxString addRult::addSetSeat(const wxString &name, const int &row, const int &col)
{
    return "";
}

void addRult::OnConfirm(wxCommandEvent &evt)
{
    // 收集定义面板中所有已拖入的 addSetSeatPanel，生成 DSL 文本并回传
    std::vector<addSetSeatPanel *> panels;
    for (wxWindow *w : definitionRultPanel->GetChildren())
    {
        if (auto *p = dynamic_cast<addSetSeatPanel *>(w))
            panels.push_back(p);
    }

    wxString rulesText = mappingRult::BuildRulesText(panels);
    if (m_mainFrame)
        m_mainFrame->ApplyVisualRules(rulesText);

    Close();
}

void addRult::DestroyDragClone()
{
    if (m_dragClone)
    {
        m_dragClone->Destroy();
        m_dragClone = nullptr;
    }
}

void addRult::OnMouseDown(wxMouseEvent &evt)
{
    if (m_isDragging)
        return;

    m_isDragging = true;
    // 记录抓取点在模板内的偏移
    m_dragStart = evt.GetPosition();
    m_presetTemplate->CaptureMouse();

    // 以 frame 为父创建克隆，自由漂浮
    m_dragClone = m_presetTemplate->Clone(this);
    m_dragClone->Move(this->ScreenToClient(wxGetMousePosition() - m_dragStart));
    m_dragClone->Raise();
}

void addRult::OnMouseMove(wxMouseEvent &evt)
{
    if (!m_isDragging || !m_dragClone)
        return;

    m_dragClone->Move(this->ScreenToClient(wxGetMousePosition() - m_dragStart));
}

void addRult::OnMouseUp(wxMouseEvent &evt)
{
    if (!m_isDragging)
        return;

    // 释放鼠标捕获，结束拖拽
    m_isDragging = false;
    if (m_presetTemplate->HasCapture())
        m_presetTemplate->ReleaseMouse();

    wxPoint mouseScreen = wxGetMousePosition();
    if (definitionRultPanel->GetScreenRect().Contains(mouseScreen))
    {
        // 落入定义面板：换父并加入垂直 sizer
        m_dragClone->Reparent(definitionRultPanel);
        m_definitionSizer->Add(m_dragClone, 0, wxEXPAND | wxALL, 5);
        definitionRultPanel->Layout();
        definitionRultPanel->FitInside();
    }
    else
    {
        // 回到预设面板或其它区域：销毁克隆
        DestroyDragClone();
    }
    m_dragClone = nullptr; // 已安置或已销毁，解除追踪
}

// 处理鼠标捕获丢失事件（如用户按下 Alt+Tab 切换窗口）
void addRult::OnMouseLost(wxMouseCaptureLostEvent &evt)
{
    m_isDragging = false;
    DestroyDragClone();
}

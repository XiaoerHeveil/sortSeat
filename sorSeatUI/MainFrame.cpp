#include "MainFrame.h"
#include "SeatPanel.h"
#include "SequenceButton.h"
#include "IpcClient.h"
#include "Validate.h"
#include <wx/dcmemory.h>
#include <wx/spinctrl.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/tokenzr.h>
#include <wx/choice.h>
#include <wx/hyperlink.h>
#include <wx/dirdlg.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

// 将内容写入 %temp% 文本文件，返回 UTF-8 路径
std::string WriteTempFile(const std::string &prefix, const std::string &content)
{
    std::filesystem::path dir = std::filesystem::temp_directory_path();
    auto ts = std::chrono::steady_clock::now().time_since_epoch().count();
    std::filesystem::path file = dir / (prefix + "_" + std::to_string(ts) + ".txt");
    std::ofstream out(file, std::ios::binary);
    out << content;
    out.close();
    return ipc::pathToUtf8(std::filesystem::absolute(file));
}

// 读取 UTF-8 路径的文本文件内容
std::string ReadFileUtf8(const std::string &utf8path)
{
    std::ifstream in(std::filesystem::u8path(utf8path), std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(in)),
                       std::istreambuf_iterator<char>());
}

} // namespace

MainFrame::MainFrame(const wxString &title)
	: wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
{
	// 初始化图片加载器
	wxInitAllImageHandlers();
	wxString resourceBase =
		wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() + wxFileName::GetPathSeparator() + "resource" + wxFileName::GetPathSeparator();
	// 未来计划：设计可以从配置文件里读取
	titleColour = wxColour(30, 145, 255);
	themeColour = wxColour(90, 90, 90);
	// 创建标题面板
	TitlePanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 60));
	TitlePanel->SetBackgroundColour(titleColour);
	// 创建界面面板
	wxPanel *InterfacePanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
	InterfacePanel->SetBackgroundColour(wxColour(0, 0, 0));
	// 创建按钮面板
	ButtonPanel = new wxPanel(TitlePanel, wxID_ANY, wxDefaultPosition, wxSize(-1, 54));
	ButtonPanel->SetBackgroundColour(titleColour);

	// 创建面板布局管理
	wxBoxSizer *titleSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *topSizer = new wxBoxSizer(wxHORIZONTAL);

	// 加载图片显示
	const wxString imageName[3] = {L"排座位logo.png", L"最小化.png", L"关闭.png"};
	for (int i = 0; i < 3; ++i)
	{
		wxString tempStr = resourceBase + imageName[i];
		switch (i)
		{
		case 0:
		{
			wxImage image(tempStr, wxBITMAP_TYPE_PNG);
			if (image.IsOk())
			{
				ImageScaling(45, TitlePanel, image, logo);
				if (logo)
				{
					topSizer->Add(logo, 0, wxALL | wxALIGN_CENTER_VERTICAL, 8);
				}
			}
			else
			{
				wxLogMessage(L"未找到该图片");
			}
			break;
		}
		case 1:
		{
			wxImage image(tempStr, wxBITMAP_TYPE_PNG);
			if (image.IsOk())
			{
				// 这里在设计时失误了，图片大小为128*6，如果高度调整为64，这会把标题栏撑爆！
				ImageScaling(2, TitlePanel, image, minimize);
			}
			else
			{
				wxLogMessage(L"未找到该图片");
			}
			break;
		}
		case 2:
		{
			wxImage image(tempStr, wxBITMAP_TYPE_PNG);
			if (image.IsOk())
			{
				ImageScaling(36, TitlePanel, image, close);
			}
			else
			{
				wxLogMessage(L"未找到该图片");
			}
			break;
		}
		}
	}

	// 创建四按钮
	// 创建按钮组

	HomeButton = new sorButton(ButtonPanel, wxID_ANY,
							   wxBitmap(resourceBase + L"首页.png", wxBITMAP_TYPE_PNG), L"首页", buttonGroup);
	ResultButton = new sorButton(ButtonPanel, wxID_ANY,
								 wxBitmap(resourceBase + L"结果.png", wxBITMAP_TYPE_PNG), L"结果", buttonGroup);
	SetUpButton = new sorButton(ButtonPanel, wxID_ANY,
								wxBitmap(resourceBase + L"设置.png", wxBITMAP_TYPE_PNG), L"设置", buttonGroup);
	InformationButton = new sorButton(ButtonPanel, wxID_ANY,
									  wxBitmap(resourceBase + L"信息.png", wxBITMAP_TYPE_PNG), L"信息", buttonGroup);

	buttonGroup.push_back(HomeButton);
	buttonGroup.push_back(ResultButton);
	buttonGroup.push_back(SetUpButton);
	buttonGroup.push_back(InformationButton);

	// 绑定事件
	// 为自定义标题栏绑定移动窗口逻辑
	TitlePanel->Bind(wxEVT_LEFT_DOWN, &MainFrame::OnMouseLeftDown, this);
	TitlePanel->Bind(wxEVT_MOTION, &MainFrame::OnMouseMotion, this);
	TitlePanel->Bind(wxEVT_LEFT_UP, &MainFrame::OnMouseLeftUp, this);
	// 官方文档强调：一旦使用了CaptureMouse，就必须处理捕获丢失事件
	TitlePanel->Bind(wxEVT_MOUSE_CAPTURE_LOST, &MainFrame::OnMouseCaptureLost, this);

	// 为按钮绑定切换页面逻辑
	HomeButton->Bind(wxEVT_BUTTON, &MainFrame::OnHomeClicked, this);
	ResultButton->Bind(wxEVT_BUTTON, &MainFrame::OnResultClicked, this);
	SetUpButton->Bind(wxEVT_BUTTON, &MainFrame::OnSetUpClicked, this);
	InformationButton->Bind(wxEVT_BUTTON, &MainFrame::OnInforClicked, this);
	if (minimize)
	{
		minimize->Bind(wxEVT_BUTTON, &MainFrame::OnMinimizeClicked, this);
	}
	if (close)
	{
		close->Bind(wxEVT_BUTTON, &MainFrame::OnCloseClicked, this);
	}

	// 按钮布局
	wxBoxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	buttonSizer->Add(HomeButton, 1, wxEXPAND | wxALL, 5);
	buttonSizer->Add(ResultButton, 1, wxEXPAND | wxALL, 5);
	buttonSizer->Add(SetUpButton, 1, wxEXPAND | wxALL, 5);
	buttonSizer->Add(InformationButton, 1, wxEXPAND | wxALL, 5);
	ButtonPanel->SetSizer(buttonSizer);

	// 创建标签
	simpleBook = new wxSimplebook(InterfacePanel, wxID_ANY, wxDefaultPosition, wxDefaultSize);

	// 创建四个界面
	// Home界面
	wxScrolledWindow *HomePanel = new wxScrolledWindow(simpleBook, wxID_ANY);
	HomePanel->SetBackgroundColour(wxColour(255, 255, 255));
	HomePanel->SetScrollRate(5, 5); // 设置滚动速率，单位为像素

	// Result界面
	wxScrolledWindow *ResultPanel = new wxScrolledWindow(simpleBook, wxID_ANY);
	ResultPanel->SetBackgroundColour(wxColour(255, 255, 255));
	ResultPanel->SetScrollRate(3, 5);
	ResultPanel->SetMinSize(wxSize(1600, 1200));

	// SetUp界面
	wxScrolledWindow *SetUpPanel = new wxScrolledWindow(simpleBook, wxID_ANY);
	SetUpPanel->SetBackgroundColour(wxColour(255, 255, 255));
	SetUpPanel->SetScrollRate(2, 5);

	// Information界面
	wxPanel *InforPanel = new wxPanel(simpleBook, wxID_ANY);
	InforPanel->SetBackgroundColour(wxColour(255, 255, 255));

	// 添加标签
	simpleBook->AddPage(HomePanel, "Home", false);
	simpleBook->AddPage(ResultPanel, "Result", false);
	simpleBook->AddPage(SetUpPanel, "SetUp", false);
	simpleBook->AddPage(InforPanel, "Information", false);

	// 让Result初始化显示
	UpdataResultPanel();

	// 让 simpleBook 填满整个 InterfacePanel
	wxBoxSizer *interfaceSizer = new wxBoxSizer(wxVERTICAL);
	interfaceSizer->Add(simpleBook, 1, wxEXPAND | wxALL, 0);
	InterfacePanel->SetSizer(interfaceSizer);

	topSizer->AddStretchSpacer(2);
	topSizer->Add(ButtonPanel, 4, wxALL | wxALIGN_CENTER_VERTICAL, 0);
	topSizer->AddStretchSpacer(1);

	topSizer->AddStretchSpacer(1);
	if (minimize)
	{
		topSizer->Add(minimize, 0, wxALL | wxALIGN_CENTER_VERTICAL, 8);
	}
	if (close)
	{
		topSizer->Add(close, 0, wxALL | wxALIGN_CENTER_VERTICAL, 8);
	}

	// titleSizer 垂直布局：topSizer（固定高度）
	titleSizer->Add(topSizer, 0, wxEXPAND | wxALL, 0);
	TitlePanel->SetSizer(titleSizer);

	// 根布局：标题栏（水平扩展，垂直固定）+ 界面面板（填满）
	wxBoxSizer *rootSizer = new wxBoxSizer(wxVERTICAL);
	rootSizer->Add(TitlePanel, 0, wxEXPAND | wxALL, 0);
	rootSizer->Add(InterfacePanel, 1, wxEXPAND | wxALL, 0);
	SetSizer(rootSizer);
	SetMinSize(wxSize(1280, 720));

	// 默认显示Home界面
	simpleBook->SetSelection(0);

	// 创建Home界面布局
	wxBoxSizer *HomeSizer = new wxBoxSizer(wxVERTICAL);
	// 建设Home界面
	wxStaticText *titleText = new wxStaticText(HomePanel, wxID_ANY, L"排座位",
											   wxPoint(-1, 10), wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
	// 设置标题字体大小
	wxFont titleFont = titleText->GetFont();
	titleFont.SetPointSize(64);
	titleText->SetFont(titleFont);

	/* 从上往下 从外到内(布局同理)
	 * HomeFoundationPanel
	 * 		Home_StudentPanel
	 * 			Student_NumberInputPanel
	 * 			Student_InforInputPanel
	 * 				StuName_TextInputPanel+StuFile_InputPanel(同行, 3:1 or 4:1)
	 * 		Home_RulesPanel
	 * 			Rules_TextPanel(附带文件导入控件)
	 * 			Rules_InputPanel
	 * 		Home_ResultDisplay(由于不能嵌套ScrolledWindow,所以可能还需要设计一个标签页)
	 * 			ResDis_TextPanel(与“开始”按钮在一起)
	 * 			Result_DisplayPanel(由于不能嵌套ScrolledWindow，所以这里改为一个按钮跳转至结果页)
	 * 			ResDis_ExportButtonPanel
	 */

	// 创建Home界面基板面板
	wxPanel *HomeFoundationPanel = new wxPanel(HomePanel, wxID_ANY);
	HomeFoundationPanel->SetBackgroundColour(themeColour);
	themeColourPanels.push_back({HomeFoundationPanel, 1.0f});
	HomePanel->SetMinSize(wxSize(-1, 1200));
	// 创建Home界面基板布局
	wxBoxSizer *HomeFoundationSizer = new wxBoxSizer(wxVERTICAL);

	// 创建学生面板
	wxPanel *Home_StudentPanel = new wxPanel(HomeFoundationPanel, wxID_ANY);
	Home_StudentPanel->SetBackgroundColour(AdjustBrightnessByPercent(themeColour, 1.5f));
	themeColourPanels.push_back({Home_StudentPanel, 1.5f});

	// 创建学生面板布局
	wxBoxSizer *Home_StudentSizer = new wxBoxSizer(wxVERTICAL);

	// 创建学生信息输入面板
	wxPanel *Student_NumberInputPanel = new wxPanel(Home_StudentPanel, wxID_ANY);

	// 创建学生信息输入布局
	// 文本-输入框-文本-输入框-文本
	wxBoxSizer *Student_NumberInputSizer = new wxBoxSizer(wxHORIZONTAL);

	// 文本
	wxStaticText *columnsText = new wxStaticText(Student_NumberInputPanel, wxID_ANY, L"列数：");
	wxFont columnsFont = columnsText->GetFont();
	columnsFont.SetPointSize(21);
	columnsText->SetFont(columnsFont);
	wxStaticText *groupText = new wxStaticText(Student_NumberInputPanel, wxID_ANY, L"小组组数：");
	wxFont groupFont = groupText->GetFont();
	groupFont.SetPointSize(21);
	groupText->SetFont(groupFont);
	wxStaticText *fileName = new wxStaticText(Student_NumberInputPanel, wxID_ANY, L"学生名单：");
	wxFont fileFont = fileName->GetFont();
	fileFont.SetPointSize(21);
	fileName->SetFont(fileFont);
	// 输入框
	columnsNum = new wxSpinCtrl(Student_NumberInputPanel, wxID_ANY, wxEmptyString,
								wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS | wxSP_WRAP, 1, 12, 6);
	groupNum = new wxSpinCtrl(Student_NumberInputPanel, wxID_ANY, wxEmptyString,
							  wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS | wxSP_WRAP, 1, 6, 4);
	// 添加进布局
	Student_NumberInputSizer->AddSpacer(10);
	Student_NumberInputSizer->Add(columnsText, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
	Student_NumberInputSizer->Add(columnsNum, 1, wxEXPAND | wxRIGHT, 10);
	Student_NumberInputSizer->Add(groupText, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
	Student_NumberInputSizer->Add(groupNum, 1, wxEXPAND | wxRIGHT, 10);
	Student_NumberInputSizer->Add(fileName, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
	Student_NumberInputPanel->SetSizer(Student_NumberInputSizer);

	// 创建姓名或人员文件导入区
	wxPanel *Student_InforInputPanel = new wxPanel(Home_StudentPanel, wxID_ANY);
	Student_InforInputPanel->SetBackgroundColour(AdjustBrightnessByPercent(themeColour, 1.8f));
	themeColourPanels.push_back({Student_InforInputPanel, 1.8f});
	// 创建布局
	wxBoxSizer *Student_InforInputSizer = new wxBoxSizer(wxHORIZONTAL);

	// 创建手动导入区
	wxPanel *StuName_TextInputPanel = new wxPanel(Student_InforInputPanel, wxID_ANY);
	// 创建布局
	wxBoxSizer *StuName_TextInputSizer = new wxBoxSizer(wxVERTICAL);
	// 文本
	wxStaticText *StudentNameInputText = new wxStaticText(StuName_TextInputPanel, wxID_ANY, L"手动输入人员名：");
	wxFont StudentNameInputFont = StudentNameInputText->GetFont();
	StudentNameInputFont.SetPointSize(18);
	StudentNameInputText->SetFont(StudentNameInputFont);
	// 人员输入框
	StudentNameInput = new wxTextCtrl(StuName_TextInputPanel, wxID_ANY,
									  L"请使用，；[空格]进行区分人员组，使用：用于在人员后跟性别", wxDefaultPosition, wxSize(-1, 400), wxTE_MULTILINE);
	// 添加至布局
	StuName_TextInputSizer->Add(StudentNameInputText, 0, wxEXPAND | wxRIGHT, 10);
	StuName_TextInputSizer->Add(StudentNameInput, 0, wxEXPAND);
	StuName_TextInputPanel->SetSizer(StuName_TextInputSizer);

	// 创建选择文件区
	wxPanel *StuFile_InputPanel = new wxPanel(Student_InforInputPanel, wxID_ANY);
	// 创建布局
	wxBoxSizer *StuFile_InputSizer = new wxBoxSizer(wxVERTICAL);

	// 创建选择规则
	wxString wildcard = L"文本文件 (*.txt)|*.txt|CSV文件 (*.csv)|*.csv|Excel文件 (*.xlsx)|*.xlsx";
	// 用于显示文件路径的静态文本
	wxStaticText *FileText = new wxStaticText(StuFile_InputPanel, wxID_ANY, L"未选择文件");
	wxFont FileFont = FileText->GetFont();
	FileFont.SetPointSize(18);
	FileText->SetFont(FileFont);
	// 创建文件选取控件
	// 创建“选择文件”按钮
	wxButton *chooseFileBtn = new wxButton(StuFile_InputPanel, wxID_ANY, L"选择文件",
										   wxDefaultPosition, wxSize(120, 40));
	// 设置按钮样式（可选）
	chooseFileBtn->SetBackgroundColour(wxColour(50, 150, 255));
	chooseFileBtn->SetForegroundColour(*wxWHITE);

	// 绑定按钮点击事件
	chooseFileBtn->Bind(wxEVT_BUTTON, [=](wxCommandEvent &)
						{
    // 创建文件对话框
    wxFileDialog fileDialog(
        StuFile_InputPanel,                 // 父窗口
        L"选择学生名单文件",                   // 标题
        wxEmptyString,                      // 默认目录
        wxEmptyString,                      // 默认文件名
        L"文本文件 (*.txt)|*.txt|CSV文件 (*.csv)|*.csv|Excel文件 (*.xlsx)|*.xlsx",
        wxFD_OPEN | wxFD_FILE_MUST_EXIST
    );

    if (fileDialog.ShowModal() == wxID_OK) {
		m_filePath = fileDialog.GetPath();
		// 更新静态文本显示文件路径（可只显示文件名）
		wxFileName fn(m_filePath);
		FileText->SetLabel(L"已选择文件: " + fn.GetFullName());
        // 如果需要，还可以将路径保存到成员变量中供后续使用
    } else {
        // 用户取消，可做相应处理
    } });

	// 添加至选择文件区布局
	StuFile_InputSizer->Add(FileText, 0, wxALIGN_LEFT | wxALL, 5);
	StuFile_InputSizer->AddStretchSpacer();
	StuFile_InputSizer->Add(chooseFileBtn, 0, wxALIGN_RIGHT | wxALL, 5);
	StuFile_InputPanel->SetSizer(StuFile_InputSizer);
	// 添加至姓名或人员文件导入区布局
	Student_InforInputSizer->Add(StuName_TextInputPanel, 4, wxEXPAND, 10);
	Student_InforInputSizer->Add(StuFile_InputPanel, 1, wxEXPAND, 10);
	Student_InforInputPanel->SetSizer(Student_InforInputSizer);
	// 添加至学生面板布局
	Home_StudentSizer->Add(Student_NumberInputPanel, 0, wxEXPAND | wxALL, 10);
	Home_StudentSizer->Add(Student_InforInputPanel, 1, wxEXPAND | wxALL, 10);
	Home_StudentPanel->SetSizer(Home_StudentSizer);

	// 创建规则输入面板
	wxPanel *Home_RulesPanel = new wxPanel(HomeFoundationPanel, wxID_ANY);
	Home_RulesPanel->SetBackgroundColour(AdjustBrightnessByPercent(themeColour, 1.5f));
	themeColourPanels.push_back({Home_RulesPanel, 1.5f});
	// 创建规则面板布局
	wxBoxSizer *Home_RulesSizer = new wxBoxSizer(wxVERTICAL);
	// 创建规则面板标题栏（加清除、导入按钮）
	wxPanel *Rules_TextPanel = new wxPanel(Home_RulesPanel, wxID_ANY);
	// 创建标题栏布局
	wxBoxSizer *Rules_TextSizer = new wxBoxSizer(wxHORIZONTAL);
	// 文本-按钮-按钮
	wxButton *ClearData = new wxButton(Rules_TextPanel, wxID_ANY, L"清除",
									   wxDefaultPosition, wxSize(100, 60));
	wxStaticText *RulesFileText = new wxStaticText(Rules_TextPanel, wxID_ANY, L"未选择文件");
	wxFont RluseFileFont = RulesFileText->GetFont();
	RluseFileFont.SetPointSize(18);
	RulesFileText->SetFont(RluseFileFont);
	wxButton *FileImportBtn = new wxButton(Rules_TextPanel, wxID_ANY, L"导入规则文件",
										   wxDefaultPosition, wxSize(100, 60));
	FileImportBtn->SetBackgroundColour(wxColour(50, 150, 255));
	FileImportBtn->SetForegroundColour(*wxWHITE);

	FileImportBtn->Bind(wxEVT_BUTTON, [=](wxCommandEvent &)
						{
		wxFileDialog fileDialog(
			Rules_TextPanel,
			L"选择规则文件",
			wxEmptyString,
			wxEmptyString,
			L"文本文件 (*.txt)|*.txt",
			wxFD_OPEN | wxFD_FILE_MUST_EXIST
		);

		if (fileDialog.ShowModal() == wxID_OK) {
			m_rulesFilePath = fileDialog.GetPath();
			wxFileName fn(m_rulesFilePath);
			RulesFileText->SetLabel(L"已选择文件: " + fn.GetFullName());
		} });
	wxStaticText *RulesText = new wxStaticText(Rules_TextPanel, wxID_ANY, L"输入排座规则（留空随机排序）：");
	wxFont RulesFont = RulesText->GetFont();
	RulesFont.SetPointSize(18);
	RulesText->SetFont(RulesFont);
	// 添加至标题栏布局
	Rules_TextSizer->AddSpacer(20);
	Rules_TextSizer->Add(RulesText, 0, wxALIGN_CENTER_VERTICAL);
	Rules_TextSizer->AddStretchSpacer(2);
	Rules_TextSizer->Add(RulesFileText, 1, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);
	Rules_TextSizer->AddStretchSpacer(1);
	Rules_TextSizer->Add(ClearData, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);
	Rules_TextSizer->Add(FileImportBtn, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);
	Rules_TextPanel->SetSizer(Rules_TextSizer);

	// 创建文本输入框
	wxPanel *Rules_InputPanel = new wxPanel(Home_RulesPanel, wxID_ANY);
	// 创建布局
	wxBoxSizer *Rules_InputSizer = new wxBoxSizer(wxVERTICAL);
	// 创建文本输入控件
	RulesInput = new wxTextCtrl(Rules_InputPanel, wxID_ANY, wxEmptyString,
								wxDefaultPosition, wxSize(-1, 400), wxTE_MULTILINE);
	// 添加布局
	Rules_InputSizer->Add(RulesInput, 1, wxEXPAND | wxALL);
	Rules_InputPanel->SetSizer(Rules_InputSizer);

	// 添加至输入布局
	Home_RulesSizer->Add(Rules_TextPanel, 0, wxEXPAND | wxALL, 10);
	Home_RulesSizer->Add(Rules_InputPanel, 0, wxEXPAND | wxALL, 10);
	Home_RulesPanel->SetSizer(Home_RulesSizer);

	// 添加排序结果面板（带跳转）
	wxPanel *Home_ResultDisplay = new wxPanel(HomeFoundationPanel, wxID_ANY);
	Home_ResultDisplay->SetBackgroundColour(AdjustBrightnessByPercent(themeColour, 1.5f));
	themeColourPanels.push_back({Home_ResultDisplay, 1.5f});
	// 布局
	wxBoxSizer *Home_DisplaySizer = new wxBoxSizer(wxVERTICAL);
	// 创建标题
	wxPanel *ResDis_TextPanel = new wxPanel(Home_ResultDisplay, wxID_ANY);
	wxBoxSizer *ResDis_TextSizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText *ResultDisplayText = new wxStaticText(ResDis_TextPanel, wxID_ANY, L"排序结果");
	wxFont ResultDisplayFont = ResultDisplayText->GetFont();
	ResultDisplayFont.SetPointSize(32);
	ResultDisplayText->SetFont(ResultDisplayFont);
	StartButton = new wxButton(ResDis_TextPanel, wxID_ANY, L"开始",
							   wxDefaultPosition, wxSize(160, 60));
	StartButton->SetBackgroundColour(wxColour(204, 51, 255));
	PositionButton = new wxButton(ResDis_TextPanel, wxID_ANY, L"归位",
								  wxDefaultPosition, wxSize(160, 60));
	PositionButton->SetBackgroundColour(wxColour(64, 64, 64));
	StartButton->Bind(wxEVT_BUTTON, &MainFrame::OnStartClicked, this);
	PositionButton->Bind(wxEVT_BUTTON, &MainFrame::OnPositionClicked, this);
	PositionButton->Enable(false);
	// 添加布局
	ResDis_TextSizer->AddStretchSpacer(3);
	ResDis_TextSizer->Add(ResultDisplayText, 0, wxEXPAND | wxALL, 10);
	ResDis_TextSizer->AddStretchSpacer(1);
	ResDis_TextSizer->Add(PositionButton, 0, wxEXPAND | wxALL, 10);
	ResDis_TextSizer->Add(StartButton, 0, wxEXPAND | wxALL, 10);
	ResDis_TextPanel->SetSizer(ResDis_TextSizer);

	// 添加面板
	wxPanel *Result_DispalyPanel = new wxPanel(Home_ResultDisplay, wxID_ANY);
	wxBoxSizer *Result_DispalySizer = new wxBoxSizer(wxVERTICAL);
	wxStaticText *Result_DispalyText = new wxStaticText(Result_DispalyPanel, wxID_ANY,
														L"由于框架原因，请点击下面的按钮跳转到单独的界面查看结果");
	wxFont Result_DispalyFont = Result_DispalyText->GetFont();
	Result_DispalyFont.SetPointSize(21);
	Result_DispalyText->SetFont(Result_DispalyFont);
	wxButton *GotuDispalyButton = new wxButton(Result_DispalyPanel, wxID_ANY, L"点击跳转",
											   wxDefaultPosition, wxSize(-1, 80));
	GotuDispalyButton->SetBackgroundColour(wxColour(235, 235, 0));
	GotuDispalyButton->Bind(wxEVT_BUTTON, &MainFrame::OnResultClicked, this);
	// 添加布局
	Result_DispalySizer->AddSpacer(60);
	Result_DispalySizer->Add(Result_DispalyText, 0, wxEXPAND | wxALL, 10);
	Result_DispalySizer->AddSpacer(60);
	Result_DispalySizer->Add(GotuDispalyButton, 0, wxEXPAND, 10);
	Result_DispalyPanel->SetSizer(Result_DispalySizer);

	// 添加导出按钮面板
	wxPanel *ResDis_ExportButtonPanel = new wxPanel(Home_ResultDisplay, wxID_ANY);
	wxBoxSizer *ResDis_ExportButtonSizer = new wxBoxSizer(wxHORIZONTAL);
	wxButton *ExportTextButton = new wxButton(ResDis_ExportButtonPanel, wxID_ANY, L"导出TXT文件",
											  wxDefaultPosition, wxSize(160, 40));
	ExportTextButton->SetBackgroundColour(wxColour(0, 120, 255));
	ExportTextButton->Bind(wxEVT_BUTTON, &MainFrame::OnExportText, this);
	wxButton *ExportExcelButton = new wxButton(ResDis_ExportButtonPanel, wxID_ANY, L"导出Excel文件",
											   wxDefaultPosition, wxSize(160, 40));
	ExportExcelButton->SetBackgroundColour(wxColour(0, 165, 40));
	ExportExcelButton->Bind(wxEVT_BUTTON, &MainFrame::OnExportExcel, this);
	wxButton *ExportPNGButton = new wxButton(ResDis_ExportButtonPanel, wxID_ANY, L"导出PNG图片",
											 wxDefaultPosition, wxSize(160, 40));
	ExportPNGButton->SetBackgroundColour(wxColour(153, 51, 255));
	ExportPNGButton->Bind(wxEVT_BUTTON, &MainFrame::OnExportPNG, this);
	// 添加布局
	ResDis_ExportButtonSizer->Add(ExportTextButton, 0, wxEXPAND | wxALL, 10);
	ResDis_ExportButtonSizer->AddStretchSpacer();
	ResDis_ExportButtonSizer->Add(ExportExcelButton, 0, wxEXPAND | wxALL, 10);
	ResDis_ExportButtonSizer->AddStretchSpacer();
	ResDis_ExportButtonSizer->Add(ExportPNGButton, 0, wxEXPAND | wxALL, 10);
	ResDis_ExportButtonPanel->SetSizer(ResDis_ExportButtonSizer);

	// 添加布局
	Home_DisplaySizer->Add(ResDis_TextPanel, 0, wxEXPAND | wxALL, 10);
	Home_DisplaySizer->Add(Result_DispalyPanel, 0, wxEXPAND | wxALL, 10);
	Home_DisplaySizer->AddSpacer(100);
	Home_DisplaySizer->Add(ResDis_ExportButtonPanel, 0, wxEXPAND | wxALL, 10);
	Home_ResultDisplay->SetSizer(Home_DisplaySizer);

	// 添加进基板布局
	HomeFoundationSizer->Add(Home_StudentPanel, 1, wxEXPAND | wxALL, 10);
	HomeFoundationSizer->Add(Home_RulesPanel, 1, wxEXPAND | wxALL, 10);
	HomeFoundationSizer->Add(Home_ResultDisplay, 1, wxEXPAND | wxALL, 10);
	HomeFoundationPanel->SetSizer(HomeFoundationSizer);

	// 应用Home界面布局
	HomeSizer->Add(titleText, 0, wxALIGN_CENTER_HORIZONTAL);
	HomeSizer->Add(HomeFoundationPanel, 0, wxEXPAND | wxALL, 10);
	HomePanel->SetSizer(HomeSizer);

	// Result界面
	// 此字符串仅用作测试参考
	wxString testText = L"张1,张2 张3,张4 张5,张6 张7,张8\n李1,李2 李3,李4 李5,李6 李7,李8\n王1,王2 王3,王4 王5,王6 王7,王8";
	ParseResultText(testText);

	// SetUp界面
	// 创建主布局
	wxBoxSizer *SetUpSizer = new wxBoxSizer(wxVERTICAL);

	// 选择语言面板及布局
	wxPanel *ChoiceLangPanel = new wxPanel(SetUpPanel, wxID_ANY, wxDefaultPosition, wxSize(-1, 64));
	ChoiceLangPanel->SetBackgroundColour(AdjustBrightnessByPercent(themeColour, 1.5f));
	themeColourPanels.push_back({ChoiceLangPanel, 1.5f});
	wxBoxSizer *ChoiceLangSizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText *ChoiceLangText = new wxStaticText(ChoiceLangPanel, wxID_ANY, L"选择语言");
	wxFont ChoiceLangFont = ChoiceLangText->GetFont();
	ChoiceLangFont.SetPointSize(14);
	ChoiceLangText->SetFont(ChoiceLangFont);
	wxArrayString LangString;
	LangString.Add(L"简体中文");
	wxChoice *LangChoice = new wxChoice(ChoiceLangPanel, wxID_ANY, wxDefaultPosition, wxSize(120, 64), LangString);
	LangChoice->SetSelection(0);
	// 添加至布局
	ChoiceLangSizer->Add(ChoiceLangText, 0, wxEXPAND | wxALL, 16);
	ChoiceLangSizer->AddStretchSpacer();
	ChoiceLangSizer->Add(LangChoice, 0, wxEXPAND | wxALL, 16);
	ChoiceLangPanel->SetSizer(ChoiceLangSizer);

	// 设置深色模式的按钮（目前没有任何实际作用，单纯图个心里安慰）
	wxPanel *SetDarkModePanel = new wxPanel(SetUpPanel, wxID_ANY);
	SetDarkModePanel->SetSize(-1, 96);
	SetDarkModePanel->SetBackgroundColour(AdjustBrightnessByPercent(themeColour, 1.5f));
	themeColourPanels.push_back({SetDarkModePanel, 1.5f});
	wxBoxSizer *SetDarkModeSizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText *DarkModeText = new wxStaticText(SetDarkModePanel, wxID_ANY, L"深色模式",
												  wxDefaultPosition, wxDefaultSize);
	wxFont DarkModeFont = DarkModeText->GetFont();
	DarkModeFont.SetPointSize(21);
	DarkModeText->SetFont(DarkModeFont);
	// 一个有点复杂的按钮
	wxString resourceDir = resourceBase + L"AnBtn";
	std::vector<wxBitmap> frames = LoadButtonFrames(resourceDir, 18);
	// 容错：如果一张图片都没加载出来，默认给个红色的占位图，防止崩溃
	if (frames.empty())
	{
		wxLogError("No frames loaded! Creating a dummy red square.");
		wxBitmap dummy(100, 100, 24);
		wxMemoryDC dc(dummy);
		dc.SetBackground(*wxRED_BRUSH);
		dc.Clear();
		dc.SelectObject(wxNullBitmap);
		frames.push_back(dummy);
	}
	// 创建示例
	SequenceButton *animBtn = new SequenceButton(SetDarkModePanel, frames);
	SetDarkModeSizer->Add(DarkModeText, 0, wxEXPAND | wxALL, 24);
	SetDarkModeSizer->AddStretchSpacer();
	SetDarkModeSizer->Add(animBtn, 0, wxEXPAND | wxALL, 16);
	SetDarkModePanel->SetSizer(SetDarkModeSizer);

	// 设置标题色
	wxPanel *SetTitleColourPanel = new wxPanel(SetUpPanel, wxID_ANY);
	SetTitleColourPanel->SetSize(-1, 64);
	SetTitleColourPanel->SetBackgroundColour(AdjustBrightnessByPercent(themeColour, 1.5f));
	themeColourPanels.push_back({SetTitleColourPanel, 1.5f});
	wxBoxSizer *SetTitleColourSizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText *SetTitleColourText = new wxStaticText(SetTitleColourPanel, wxID_ANY, L"设置标题栏颜色");
	wxFont SetTitleColourFont = SetTitleColourText->GetFont();
	SetTitleColourFont.SetPointSize(16);
	SetTitleColourText->SetFont(SetTitleColourFont);
	wxColourPickerCtrl *SetTitleColour = new wxColourPickerCtrl(SetTitleColourPanel, wxID_ANY, titleColour);
	// 绑定颜色更新逻辑
	SetTitleColour->Bind(wxEVT_COLOURPICKER_CHANGED, &MainFrame::OnSetTitleColour, this);
	// 添加布局
	SetTitleColourSizer->Add(SetTitleColourText, 0, wxEXPAND | wxALL, 10);
	SetTitleColourSizer->AddStretchSpacer();
	SetTitleColourSizer->Add(SetTitleColour, 0, wxEXPAND | wxALL, 10);
	SetTitleColourPanel->SetSizer(SetTitleColourSizer);

	// 设置主题色
	wxPanel *SetThemeColourPanel = new wxPanel(SetUpPanel, wxID_ANY);
	SetThemeColourPanel->SetSize(-1, 64);
	SetThemeColourPanel->SetBackgroundColour(AdjustBrightnessByPercent(themeColour, 1.5f));
	themeColourPanels.push_back({SetThemeColourPanel, 1.5f});
	wxBoxSizer *SetThemeColourSizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText *SetThemeColourText = new wxStaticText(SetThemeColourPanel, wxID_ANY, L"设置主题色");
	wxFont SetThemeColourFont = SetThemeColourText->GetFont();
	SetThemeColourFont.SetPointSize(16);
	SetThemeColourText->SetFont(SetThemeColourFont);
	wxColourPickerCtrl *SetThemeColour = new wxColourPickerCtrl(SetThemeColourPanel, wxID_ANY, themeColour);
	// 绑定颜色跟新逻辑
	SetThemeColour->Bind(wxEVT_COLOURPICKER_CHANGED, &MainFrame::OnSetThemeColour, this);
	// 添加布局
	SetThemeColourSizer->Add(SetThemeColourText, 0, wxEXPAND | wxALL, 10);
	SetThemeColourSizer->AddStretchSpacer();
	SetThemeColourSizer->Add(SetThemeColour, 0, wxEXPAND | wxALL, 10);
	SetThemeColourPanel->SetSizer(SetThemeColourSizer);

	// ---------------------------高级设置区------------------------------

	// 显示/隐藏高级设置按钮
	// 左右两段一模一样的文本
	wxPanel *AdvancedSettingPanel = new wxPanel(SetUpPanel, wxID_ANY);
	wxBoxSizer *AdvancedSettingSizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText *dividingLine1 = new wxStaticText(AdvancedSettingPanel, wxID_ANY,
												   "-----------------------------------------------------------");
	wxStaticText *dividingLine2 = new wxStaticText(AdvancedSettingPanel, wxID_ANY,
												   "-----------------------------------------------------------");
	AdvancedSetting = new wxToggleButton(AdvancedSettingPanel, wxID_ANY, L"高级设置");
	AdvancedSetting->Bind(wxEVT_TOGGLEBUTTON, &MainFrame::OnToggle, this);
	AdvancedSettingSizer->Add(dividingLine1, 1, wxEXPAND);
	AdvancedSettingSizer->Add(AdvancedSetting, 0, wxEXPAND, 5);
	AdvancedSettingSizer->Add(dividingLine2, 1, wxEXPAND);
	AdvancedSettingPanel->SetSizer(AdvancedSettingSizer);

	// 导出日志
	ExportLogPanel = new wxPanel(SetUpPanel, wxID_ANY);
	ExportLogPanel->SetBackgroundColour(AdjustBrightnessByPercent(themeColour, 1.5f));
	themeColourPanels.push_back({ExportLogPanel, 1.5f});
	wxBoxSizer *ExportLogSizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText *ExportLogText = new wxStaticText(ExportLogPanel, wxID_ANY, L"导出日志");
	wxFont ExportLogFont = ExportLogText->GetFont();
	ExportLogFont.SetPointSize(16);
	ExportLogText->SetFont(ExportLogFont);
	wxDirPickerCtrl *FileLogPath = new wxDirPickerCtrl(ExportLogPanel, wxID_ANY,
													   wxEmptyString, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxDIRP_USE_TEXTCTRL | wxDIRP_DIR_MUST_EXIST);
	FileLogPath->Bind(wxEVT_DIRPICKER_CHANGED, &MainFrame::OnExportLogFile, this);
	ExportLogSizer->Add(ExportLogText, 0, wxEXPAND | wxALL, 10);
	ExportLogSizer->AddStretchSpacer();
	ExportLogSizer->Add(FileLogPath, 0, wxEXPAND | wxALL, 10);
	ExportLogPanel->SetSizer(ExportLogSizer);

	// 添加至SetUp总界面
	SetUpSizer->Add(ChoiceLangPanel, 0, wxEXPAND | wxALL, 10);
	SetUpSizer->Add(SetDarkModePanel, 0, wxEXPAND | wxALL, 10);
	SetUpSizer->Add(SetTitleColourPanel, 0, wxEXPAND | wxALL, 10);
	SetUpSizer->Add(SetThemeColourPanel, 0, wxEXPAND | wxALL, 10);
	SetUpSizer->Add(AdvancedSettingPanel, 0, wxEXPAND | wxTOP | wxBOTTOM, 10);
	SetUpSizer->Add(ExportLogPanel, 0, wxEXPAND | wxALL, 10);
	ExportLogPanel->Hide(); // 默认隐藏，点击高级设置后显示
	SetUpPanel->SetSizer(SetUpSizer);

	// Infor界面
	wxBoxSizer *InforSizer = new wxBoxSizer(wxVERTICAL);

	// 感谢语
	wxPanel *ThankPanel = new wxPanel(InforPanel, wxID_ANY);
	wxBoxSizer *ThankSizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText *ThankYourText = new wxStaticText(ThankPanel, wxID_ANY, L"感谢您使用本软件");
	wxFont ThankYourFont = ThankYourText->GetFont();
	ThankYourFont.SetPointSize(64);
	ThankYourText->SetFont(ThankYourFont);
	// 添加布局
	ThankSizer->AddStretchSpacer();
	ThankSizer->Add(ThankYourText, 0, wxEXPAND | wxALL, 10);
	ThankSizer->AddStretchSpacer();
	ThankPanel->SetSizer(ThankSizer);

	// 声明版本号和开源协议
	wxPanel *VersionOrAgreementPanel = new wxPanel(InforPanel, wxID_ANY);
	wxBoxSizer *VersionOrAgreementSizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText *VersionNumber = new wxStaticText(VersionOrAgreementPanel, wxID_ANY, L"早期开发版: Alpha 0.1.5");
	wxFont VersionFont = VersionNumber->GetFont();
	VersionFont.SetPointSize(12);
	VersionNumber->SetFont(VersionFont);
	wxStaticText *OpenSourceLicense = new wxStaticText(VersionOrAgreementPanel, wxID_ANY, L"本软件使用LGPT开源协议");
	wxFont LicenseFont = OpenSourceLicense->GetFont();
	LicenseFont.SetPointSize(42);
	OpenSourceLicense->SetFont(LicenseFont);
	// 添加布局
	VersionOrAgreementSizer->Add(VersionNumber, 0, wxTOP, 60);
	VersionOrAgreementSizer->AddStretchSpacer(1);
	VersionOrAgreementSizer->Add(OpenSourceLicense, 0, wxEXPAND | wxALL, 10);
	VersionOrAgreementSizer->AddStretchSpacer(2);
	VersionOrAgreementPanel->SetSizer(VersionOrAgreementSizer);

	// 声明项目地址（虽然还没有推流至远程仓库，但不妨碍先把框架搭好）
	projectAddressPanel = new wxPanel(InforPanel, wxID_ANY);
	projectAddressPanel->SetBackgroundColour(AdjustBrightnessByPercent(themeColour, 1.6f));
	themeColourPanels.push_back({projectAddressPanel, 1.6f});
	wxBoxSizer *projectAddressSizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText *githubRepositoryText = new wxStaticText(projectAddressPanel, wxID_ANY, L"项目地址: ");
	wxFont githubRepositoryFont = githubRepositoryText->GetFont();
	githubRepositoryFont.SetPointSize(16);
	githubRepositoryText->SetFont(githubRepositoryFont);
	// 超链接（目前为空）
	wxHyperlinkCtrl *githubRepositoryURL = new wxHyperlinkCtrl(projectAddressPanel, wxID_ANY, wxEmptyString, wxEmptyString);
	// 添加布局
	projectAddressSizer->Add(githubRepositoryText, 0, wxEXPAND | wxALL, 10);
	projectAddressSizer->Add(githubRepositoryURL, 0, wxEXPAND | wxLEFT | wxTOP, 20);
	projectAddressPanel->SetSizer(projectAddressSizer);

	// 声明作者(并附赠跳转作者视频平台链接，目前暂时指向自己)
	authorPanel = new wxPanel(InforPanel, wxID_ANY);
	authorPanel->SetBackgroundColour(AdjustBrightnessByPercent(themeColour, 1.6f));
	themeColourPanels.push_back({authorPanel, 1.6f});
	wxBoxSizer *authorSizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText *authorText = new wxStaticText(authorPanel, wxID_ANY, L"作者: ");
	wxFont authorFont = authorText->GetFont();
	authorFont.SetPointSize(16);
	authorText->SetFont(authorFont);
	wxHyperlinkCtrl *authorURL = new wxHyperlinkCtrl(authorPanel, wxID_ANY, "XiaoBingGun", "http://127.0.0.1:8080");
	// 添加布局
	authorSizer->Add(authorText, 0, wxEXPAND | wxALL, 10);
	authorSizer->Add(authorURL, 0, wxEXPAND | wxLEFT | wxTOP, 20);
	authorPanel->SetSizer(authorSizer);

	// 添加至信息页总布局
	InforSizer->Add(ThankPanel, 0, wxEXPAND | wxALL, 10);
	InforSizer->Add(VersionOrAgreementPanel, 0, wxEXPAND | wxALL, 10);
	InforSizer->Add(projectAddressPanel, 0, wxEXPAND | wxALL, 10);
	InforSizer->Add(authorPanel, 0, wxEXPAND | wxALL, 10);
	InforPanel->SetSizer(InforSizer);

	// 强制布局刷新
	Layout();
}

/* 这里没有任何有用说明
 * 单纯代码写多了
 * 用来区分构造函数和函数定义的
 */

void MainFrame::OnHomeClicked(wxCommandEvent &evt)
{
	// 切换到Home界面
	simpleBook->SetSelection(0);
	simpleBook->Layout();
}

void MainFrame::OnResultClicked(wxCommandEvent &evt)
{
	// 切换到Result界面
	simpleBook->SetSelection(1);
	UpdataResultPanel(); // 保证显示最新内容
	simpleBook->Layout();
}

void MainFrame::OnSetUpClicked(wxCommandEvent &evt)
{
	// 切换到SetUp界面
	simpleBook->SetSelection(2);
	simpleBook->Layout();
}

void MainFrame::OnInforClicked(wxCommandEvent &evt)
{
	// 切换到Infor界面
	simpleBook->SetSelection(3);
	simpleBook->Layout();
}

void MainFrame::OnStartClicked(wxCommandEvent &evt)
{
	// 判断是否输入了人员或选择了人员文件
	bool hasManual = !StudentNameInput->GetValue().IsEmpty();
	bool hasFile = !m_filePath.IsEmpty();
	if (!hasManual && !hasFile)
	{
		wxMessageBox(L"您还没有输入任何有效数据或选择有效文件", L"提示",
					 wxOK | wxICON_INFORMATION);
		return;
	}

	auto &backend = ipc::BackendClient::instance();
	if (!backend.isOpen())
	{
		wxMessageBox(L"主进程未就绪，请稍后重试", L"错误", wxOK | wxICON_ERROR);
		return;
	}

	// 学生名单：手动输入写 %temp%，否则使用选择的文件路径
	std::string studentPath;
	if (hasManual)
	{
		std::string normalized = validate::normalizeStudentInput(
			StudentNameInput->GetValue().utf8_string());
		studentPath = WriteTempFile("sortSeat_students", normalized);
	}
	else
	{
		studentPath = m_filePath.utf8_string();
	}

	// 规则：输入框写 %temp%，否则使用导入的规则文件路径
	std::string rulesPath;
	if (!RulesInput->GetValue().IsEmpty())
	{
		std::string normalized = validate::normalizeRulesInput(
			RulesInput->GetValue().utf8_string());
		rulesPath = WriteTempFile("sortSeat_rules", normalized);
	}
	else
	{
		rulesPath = m_rulesFilePath.utf8_string();
	}

	int x_row = columnsNum->GetValue();
	int groupRow = groupNum->GetValue();
	ipc::Message msg;
	msg.op = ipc::Op::START;
	msg.payload = ipc::packFields({std::to_string(x_row), std::to_string(groupRow),
								   studentPath, rulesPath});
	if (!backend.send(msg))
	{
		wxMessageBox(L"发送失败", L"错误", wxOK | wxICON_ERROR);
		return;
	}

	ipc::Message resp;
	if (!backend.receive(resp, 15000))
	{
		wxMessageBox(L"主进程无响应", L"错误", wxOK | wxICON_ERROR);
		return;
	}
	if (resp.op == ipc::Op::ERR)
	{
		wxMessageBox(wxString::FromUTF8(resp.payload), L"排序失败",
					 wxOK | wxICON_ERROR);
		return;
	}
	if (resp.op != ipc::Op::RESULT)
	{
		wxMessageBox(L"未知响应", L"错误", wxOK | wxICON_ERROR);
		return;
	}

	std::string text = ReadFileUtf8(resp.payload);
	ParseResultText(wxString::FromUTF8(text));

	sortResult = true;
	PositionButton->Enable(true);
	StartButton->Enable(false);
	simpleBook->SetSelection(1);
	UpdataResultPanel();
}

void MainFrame::OnPositionClicked(wxCommandEvent &evt)
{
	sortResult = false;
	// 启用开始按钮，并禁用自己
	StartButton->Enable(true);
	PositionButton->Enable(false);
	if (simpleBook->GetSelection() == 1)
	{
		// 如果在当前页，立即刷新
		UpdataResultPanel();
	}
}

void MainFrame::OnMinimizeClicked(wxCommandEvent &evt)
{
	Iconize(true);
}

void MainFrame::OnCloseClicked(wxCommandEvent &evt)
{
	Close();
}

void MainFrame::OnToggle(wxCommandEvent &evt)
{
	AdvancedSettingStatus = AdvancedSetting->GetValue();
	if (AdvancedSettingStatus)
	{
		AdvancedSetting->SetLabel(L"高级设置 ▼");
	}
	else
	{
		AdvancedSetting->SetLabel(L"高级设置 ▲");
	}
	if (ExportLogPanel)
		ExportLogPanel->Show(AdvancedSettingStatus);
	// 刷新SetUp页（第2页）布局与滚动区域
	if (wxScrolledWindow *setUp = dynamic_cast<wxScrolledWindow *>(simpleBook->GetPage(2)))
	{
		setUp->Layout();
		setUp->FitInside();
	}
}

void MainFrame::OnExportLogFile(wxFileDirPickerEvent &evt)
{
	wxString path = evt.GetPath();
	if (!path.empty() && wxFileName::DirExists(path))
	{
		LogFilePath = true;
		m_logFilePath = path;
		SendExport(ipc::Op::EXPORT_LOG, path.utf8_string());
		LogFilePath = false;
	}
	else
	{
		LogFilePath = false;
		m_logFilePath.clear();
	}
}

void MainFrame::OnExportText(wxCommandEvent &evt)
{
	wxDirDialog dlg(this, L"选择导出目录", wxEmptyString,
					wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
	if (dlg.ShowModal() != wxID_OK)
		return;
	SendExport(ipc::Op::EXPORT_TXT, dlg.GetPath().utf8_string());
}

void MainFrame::OnExportExcel(wxCommandEvent &evt)
{
	wxDirDialog dlg(this, L"选择导出目录", wxEmptyString,
					wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
	if (dlg.ShowModal() != wxID_OK)
		return;
	SendExport(ipc::Op::EXPORT_EXCEL, dlg.GetPath().utf8_string());
}

void MainFrame::OnExportPNG(wxCommandEvent &evt)
{
	wxMessageBox(L"目前还没有完成导出PNG图片的逻辑，请等待后续版本更新", L"提示",
				 wxOK | wxICON_INFORMATION);
}

void MainFrame::SendExport(ipc::Op op, const std::string &dirUtf8)
{
	auto &backend = ipc::BackendClient::instance();
	if (!backend.isOpen())
	{
		wxMessageBox(L"主进程未就绪", L"错误", wxOK | wxICON_ERROR);
		return;
	}
	ipc::Message msg;
	msg.op = op;
	msg.payload = dirUtf8;
	if (!backend.send(msg))
	{
		wxMessageBox(L"发送失败", L"错误", wxOK | wxICON_ERROR);
		return;
	}
	ipc::Message resp;
	if (!backend.receive(resp, 10000))
	{
		wxMessageBox(L"主进程无响应", L"错误", wxOK | wxICON_ERROR);
		return;
	}
	if (resp.op == ipc::Op::ACK)
	{
		wxMessageBox(L"导出成功", L"提示", wxOK | wxICON_INFORMATION);
	}
	else
	{
		wxMessageBox(wxString::FromUTF8(resp.payload), L"导出失败",
					 wxOK | wxICON_ERROR);
	}
}

void MainFrame::UpdataResultPanel()
{
	// 获取 ResultPanel（simpleBook 的第 1 页）
	wxScrolledWindow *resultPanel = dynamic_cast<wxScrolledWindow *>(simpleBook->GetPage(1));
	if (!resultPanel)
		return;

	// 清空原有内容
	resultPanel->DestroyChildren();

	wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

	if (!sortResult)
	{
		// 显示提示文本
		wxStaticText *hint = new wxStaticText(resultPanel, wxID_ANY,
											  L"你还没有开始排序",
											  wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE);
		wxFont font = hint->GetFont();
		font.SetPointSize(24);
		hint->SetFont(font);
		mainSizer->Add(hint, 1, wxALIGN_CENTER | wxALL, 20);
	}
	else
	{
		// 构建网格
		const int seatWidth = 180;
		const int seatHeight = 60;
		const int gapWidth = 60; // 空格占位宽度

		for (size_t rowIdx = 0; rowIdx < m_resultRows.size(); ++rowIdx)
		{
			const auto &row = m_resultRows[rowIdx];
			wxBoxSizer *rowSizer = new wxBoxSizer(wxHORIZONTAL);

			for (size_t colIdx = 0; colIdx < row.size(); ++colIdx)
			{
				const wxString &name = row[colIdx];
				if (name.empty())
				{
					// 空位：使用固定大小的间隔（不创建控件，更轻量）
					rowSizer->AddSpacer(gapWidth);
				}
				else
				{
					SeatPanel *seat = new SeatPanel(resultPanel, name);
					rowSizer->Add(seat, 0, wxALL, 2); // 2px 间距让边框明显
				}
			}
			mainSizer->Add(rowSizer, 0, wxALIGN_LEFT | wxALL, 4);
		}
	}

	resultPanel->SetSizer(mainSizer);
	resultPanel->Layout();
	resultPanel->FitInside(); // 更新滚动区域
	resultPanel->SetScrollRate(5, 5);
}

void MainFrame::ParseResultText(const wxString &text)
{
	m_resultRows.clear();
	if (text.IsEmpty())
		return;

	// 换行分割
	wxStringTokenizer lineTok(text, "\n", wxTOKEN_RET_EMPTY_ALL);
	while (lineTok.HasMoreTokens())
	{
		wxString line = lineTok.GetNextToken();
		// 按空格分割，得到元素（可能包含逗号分隔的多个姓名）
		wxArrayString elements = wxSplit(line, ' ', '\0');
		std::vector<wxString> row;
		for (size_t i = 0; i < elements.size(); ++i)
		{
			wxString elem = elements[i];
			if (elem.empty())
			{
				// 连续空格：保留一个空位（可根据需求调整）
				row.push_back(wxEmptyString);
			}
			else
			{
				// 按逗号拆分，逗号表示连续绘制，不追加额外空格
				wxArrayString names = wxSplit(elem, ',', '\0');
				for (size_t j = 0; j < names.size(); ++j)
				{
					row.push_back(names[j]);
				}
			}
		}
		m_resultRows.push_back(row);
	}
}

void MainFrame::ImageScaling(const int &Height, wxPanel *panel, const wxImage &image, wxStaticBitmap *&stamap)
{
	// 计算缩放后的宽度，保持原图的宽高比
	int Width = image.GetWidth() * ((double)Height / image.GetHeight());
	wxImage scaledImage = image;
	scaledImage.Rescale(Width, Height, wxIMAGE_QUALITY_HIGH);
	wxBitmap resizedBitmap(scaledImage);
	stamap = new wxStaticBitmap(panel, wxID_ANY, resizedBitmap);
}

void MainFrame::ImageScaling(const int &Height, wxPanel *panel, const wxImage &image, wxBitmapButton *&bitbtn)
{
	int Width = image.GetWidth() * ((double)Height / image.GetHeight());
	wxImage scaledImage = image;
	scaledImage.Rescale(Width, Height, wxIMAGE_QUALITY_HIGH);
	// 创建缩放后的位图
	wxBitmap resizedBitmap(scaledImage);
	// 再次将缩放后的位图进行处理，将处理后的位图叠加在新位图上
	// 获取父窗口的颜色
	wxColour bgColour = panel->GetBackgroundColour();
	// 创建一张与处理后的图片一样大的新位图，并填充为父框架的背景色
	wxBitmap prepardBmp(resizedBitmap.GetWidth(), resizedBitmap.GetHeight());
	wxMemoryDC memDC(prepardBmp);
	memDC.SetBackground(bgColour);
	memDC.Clear();
	// 将处理后的位图叠加在新位图上
	memDC.DrawBitmap(resizedBitmap, 0, 0, true);
	memDC.SelectObject(wxNullBitmap);
	// 创建位图按钮
	bitbtn = new wxBitmapButton(panel, wxID_ANY, prepardBmp);
}

// 鼠标左键被按下——记录偏移量，只捕获鼠标，不移动窗口
void MainFrame::OnMouseLeftDown(wxMouseEvent &evt)
{
	if (!m_isDragging)
	{
		m_isDragging = true;
		m_captureWindow = static_cast<wxWindow *>(evt.GetEventObject());
		// 记录鼠标屏幕坐标与窗口屏幕位置之间的偏移量
		m_dragOffset = wxGetMousePosition() - GetPosition();

		m_captureWindow->CaptureMouse();
	}
}

// 鼠标移动——拖拽过程中不移动窗口，消除实时重绘导致的抽搐
void MainFrame::OnMouseMotion(wxMouseEvent &evt)
{
	// 不实时跟随，松开鼠标后一次性定位
	// 后续等某位大神修复这个问题吧
}

// 鼠标左键松开——计算最终位置并移动窗口
void MainFrame::OnMouseLeftUp(wxMouseEvent &evt)
{
	if (m_isDragging)
	{
		m_isDragging = false;
		// 基于当前鼠标屏幕位置和初始偏移量计算窗口新位置
		wxPoint newPos = wxGetMousePosition() - m_dragOffset;
		SetPosition(newPos);
		if (m_captureWindow && m_captureWindow->HasCapture())
		{
			m_captureWindow->ReleaseMouse();
		}
		m_captureWindow = nullptr;
	}
}

// 处理鼠标捕获丢失（例如其他窗口弹出对话框）
void MainFrame::OnMouseCaptureLost(wxMouseCaptureLostEvent &evt)
{
	if (m_isDragging)
	{
		m_isDragging = false;
		m_captureWindow = nullptr;
	}
}

void MainFrame::OnSetTitleColour(wxColourPickerEvent &evt)
{
	titleColour = evt.GetColour();
	ApplyColours();
}

void MainFrame::OnSetThemeColour(wxColourPickerEvent &evt)
{
	themeColour = evt.GetColour();
	ApplyColours();
}

void MainFrame::ApplyColours()
{
	if (TitlePanel)
		TitlePanel->SetBackgroundColour(titleColour);
	if (ButtonPanel)
		ButtonPanel->SetBackgroundColour(titleColour);
	for (auto &p : themeColourPanels)
		if (p.first)
			p.first->SetBackgroundColour(AdjustBrightnessByPercent(themeColour, p.second));
	Refresh(); // 触发整窗重绘
}

/**
 * @brief 将颜色按百分比提升或减少亮度
 *
 * @param originalColour 颜色
 * @param percent 亮度百分比
 * @return wxColour 处理亮度后的颜色
 */
wxColour MainFrame::AdjustBrightnessByPercent(const wxColour &originalColour, float percent)
{
	int r = wxMin(255, wxMax(0, (int)(originalColour.GetRed() * percent)));
	int g = wxMin(255, wxMax(0, (int)(originalColour.GetGreen() * percent)));
	int b = wxMin(255, wxMax(0, (int)(originalColour.GetBlue() * percent)));

	return wxColour(r, g, b);
}

/* 这里没有任何有用的说明
 * 单纯只是想区分
 * 成员函数和普通函数
 */

static std::vector<wxBitmap> LoadButtonFrames(const wxString &basePath, int count)
{
	std::vector<wxBitmap> frames;
	for (int i = 0; i < count; ++i)
	{
		wxString path = wxString::Format("%s/animationButton_%02d.png", basePath, i);
		wxImage image;
		if (image.LoadFile(path) && image.IsOk())
		{
			frames.push_back(wxBitmap(image));
		}
		else
		{
			// 容错处理：如果加载失败，可以用一个空位或默认颜色占位
			wxLogWarning("Failde to load: %s", path);
		}
	}
	return frames;
}
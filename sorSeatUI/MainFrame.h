#pragma once
#include "sorButton.h"
#include "IpcCommon.h"
#include "SequenceButton.h"
#include <wx/wx.h>
#include <wx/simplebook.h>
#include <wx/bmpbuttn.h>
#include <wx/clrpicker.h>
#include <wx/tglbtn.h>
#include <wx/filepicker.h>
#include <vector>

class wxTextCtrl;
class wxSpinCtrl;

class MainFrame : public wxFrame
{
public:
	MainFrame(const wxString &title);

	// 可视化规则确认后回传：按 synchronizationBehavior 同步到 RulesInput 或静默写入规则文件
	void ApplyVisualRules(const wxString &rulesText);

private:
	wxSimplebook *simpleBook = nullptr;
	sorButton *HomeButton = nullptr;
	sorButton *ResultButton = nullptr;
	sorButton *SetUpButton = nullptr;
	sorButton *InformationButton = nullptr;
	wxStaticBitmap *logo = nullptr;
	wxBitmapButton *minimize = nullptr;
	wxBitmapButton *close = nullptr;
	wxPoint m_dragOffset;				 // 鼠标按下时，鼠标屏幕坐标与窗口屏幕位置的偏移量
	wxWindow *m_captureWindow = nullptr; // 当前捕获鼠标的窗口
	wxButton *StartButton = nullptr;
	wxButton *PositionButton = nullptr;
	wxButton *AddRultButton = nullptr;
	wxPanel *RulesTextPanel = nullptr;
	wxPanel *HomeRulesPanel = nullptr;
	wxToggleButton *AdvancedSetting = nullptr;
	wxToggleButton *visualizationRultSetting = nullptr;
	wxTextCtrl *StudentNameInput = nullptr; // 手动输入人员名
	wxTextCtrl *RulesInput = nullptr;       // 规则输入
	wxSpinCtrl *columnsNum = nullptr;       // 座位列数
	wxSpinCtrl *groupNum = nullptr;         // 小组组数
	std::vector<sorButton *> buttonGroup;
	std::vector<std::vector<wxString>> m_resultRows;
	bool m_isDragging = false;
	bool result = false;
	bool sortResult = false;
	bool AdvancedSettingStatus = false;
	bool LogFilePath = false;
	bool visualizationRult = false;
	bool synchronizationBehavior = true;
	SequenceButton *openAddRultBtn = nullptr;
	SequenceButton *synchrBehBtn = nullptr;
	wxStaticText *synchrBehText2 = nullptr;
	wxString m_filePath;
	wxString m_rulesFilePath;
	wxString m_logFilePath;
	wxColour titleColour;
	wxColour themeColour;
	wxPanel *TitlePanel = nullptr;
	wxPanel *ButtonPanel = nullptr;
	wxPanel *visualizationRultPanel = nullptr;
	wxPanel *synchronizationBehaviorPanel = nullptr;
	wxPanel *ExportLogPanel = nullptr;
	wxPanel *projectAddressPanel = nullptr;
	wxPanel *authorPanel = nullptr;
	std::vector<std::pair<wxPanel *, float>>
		themeColourPanels; // (面板, 亮度系数)

	void OnHomeClicked(wxCommandEvent &evt);
	void OnResultClicked(wxCommandEvent &evt);
	void OnSetUpClicked(wxCommandEvent &evt);
	void OnInforClicked(wxCommandEvent &evt);
	void OnStartClicked(wxCommandEvent &evt);
	void OnPositionClicked(wxCommandEvent &evt);
	void OnMinimizeClicked(wxCommandEvent &evt);
	void OnCloseClicked(wxCommandEvent &evt);
	void OnToggle(wxCommandEvent &evt);				 // 设置切换状态
	void OnExportLogFile(wxFileDirPickerEvent &evt); // 导出日志
	void OnExportText(wxCommandEvent &evt);			 // 导出 TXT
	void OnExportExcel(wxCommandEvent &evt);		 // 导出 Excel
	void OnExportPNG(wxCommandEvent &evt);			 // 导出 PNG（未实现）
	void SendExport(ipc::Op op, const std::string &dirUtf8); // 通用导出请求
	void UpdataResultPanel();						 // 刷新ResultPanel
	void ParseResultText(const wxString &text);		 // 解析文本到m_resultRows
	// 拖拽窗口事件处理函数
	void OnMouseLeftDown(wxMouseEvent &evt);
	void OnMouseMotion(wxMouseEvent &evt);
	void OnMouseLeftUp(wxMouseEvent &evt);
	void OnMouseCaptureLost(wxMouseCaptureLostEvent &evt);
	// 设置全局颜色
	void OnSetTitleColour(wxColourPickerEvent &evt);
	void OnSetThemeColour(wxColourPickerEvent &evt);
	void ApplyColours(); // 重新应用标题色与主题色到所有相关面板
	// 设置是否启用可视化规则编辑面板
	void OnSetvisualizationRultClicked(wxCommandEvent &evt);
	// 切换规则同步行为
	void OnSynchrBehClicked(wxCommandEvent &evt);
	// 按「高级设置」与「可视化开关」双条件刷新同步行为面板显示
	void UpdateSynchronizationBehaviorPanel();
	// 对亮度进行处理的函数
	wxColour AdjustBrightnessByPercent(const wxColour &originalColour, float percent);

	void ImageScaling(const int &Height, wxPanel *panel, const wxImage &image, wxStaticBitmap *&stamap);
	void ImageScaling(const int &Height, wxPanel *panel, const wxImage &image, wxBitmapButton *&bitbtn);
};

static std::vector<wxBitmap> LoadButtonFrames(const wxString &basePath, int count);

#pragma once
#include "sorButton.h"
#include <wx/wx.h>
#include <wx/simplebook.h>
#include <wx/bmpbuttn.h>
#include <wx/clrpicker.h>
#include <wx/tglbtn.h>
#include <wx/filepicker.h>
#include <vector>

class MainFrame : public wxFrame
{
public:
	MainFrame(const wxString &title);

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
	wxToggleButton *AdvancedSetting = nullptr;
	std::vector<sorButton *> buttonGroup;
	std::vector<std::vector<wxString>> m_resultRows;
	bool m_isDragging = false;
	bool result = false;
	bool sortResult = false;
	bool AdvancedSettingStatus = false;
	bool LogFilePath = false;
	wxString m_filePath;
	wxString m_rulesFilePath;
	wxString m_logFilePath;
	wxColour titleColour;
	wxColour themeColour;
	wxPanel *TitlePanel = nullptr;
	wxPanel *ButtonPanel = nullptr;
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
	// 对亮度进行处理的函数
	wxColour AdjustBrightnessByPercent(const wxColour &originalColour, float percent);

	void ImageScaling(const int &Height, wxPanel *panel, const wxImage &image, wxStaticBitmap *&stamap);
	void ImageScaling(const int &Height, wxPanel *panel, const wxImage &image, wxBitmapButton *&bitbtn);
};

static std::vector<wxBitmap> LoadButtonFrames(const wxString &basePath, int count);
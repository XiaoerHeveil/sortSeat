#pragma once
#include <wx/wx.h>
#include <vector>

class addSetSeatPanel;
class wxTextCtrl;

// 将可视化规则编辑的结果（一组 addSetSeatPanel）映射为 DSL 文本，并按同步行为传输
class mappingRult
{
public:
    // 单个面板 -> setSeat("名字", 列, 行) 规则行
    static wxString ToRuleText(addSetSeatPanel *panel);

    // 一组面板 -> 多行 DSL 文本（每行一条规则）
    static wxString BuildRulesText(const std::vector<addSetSeatPanel *> &panels);

    // 传输：sync=true 时覆盖写入 rulesInput（同步到规则文本框）；否则静默写 %temp% 文件并更新 rulesFilePath
    static void Transfer(const wxString &rulesText, bool sync,
                         wxTextCtrl *rulesInput, wxString &rulesFilePath);
};

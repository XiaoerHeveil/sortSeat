#include "mappingRult.h"
#include "presetsRult.h"
#include <wx/textctrl.h>
#include <filesystem>
#include <fstream>
#include <chrono>

/**
 * @brief 将单个面板映射为规则文本
 * 
 * @param panel 单个面板
 * @return wxString 
 */
wxString mappingRult::ToRuleText(addSetSeatPanel *panel)
{
    return wxString::Format("setSeat(\"%s\", %d, %d)",
                            panel->getName(), panel->getRow(), panel->getCollom());
}

/**
 * @brief 将一组面板映射为规则文本
 * 
 * @param panels 一组面板
 * @return wxString 
 */
wxString mappingRult::BuildRulesText(const std::vector<addSetSeatPanel *> &panels)
{
    wxString out;
    for (size_t i = 0; i < panels.size(); ++i)
    {
        if (i)
            out += '\n';
        out += ToRuleText(panels[i]);
    }
    return out;
}

/**
 * @brief 将规则文本传输到指定位置
 * 
 * @param rulesText 规则文本
 * @param sync 同步标志
 * @param rulesInput 规则输入框
 * @param rulesFilePath 规则文件路径
 */
void mappingRult::Transfer(const wxString &rulesText, bool sync,
                           wxTextCtrl *rulesInput, wxString &rulesFilePath)
{
    if (sync)
    {
        if (rulesInput)
            rulesInput->SetValue(rulesText);
        return;
    }

    std::string utf8 = rulesText.utf8_string();
    std::filesystem::path dir = std::filesystem::temp_directory_path();
    auto ts = std::chrono::steady_clock::now().time_since_epoch().count();
    std::filesystem::path file =
        dir / ("sortSeat_visual_rules_" + std::to_string(ts) + ".txt");
    std::ofstream out(file, std::ios::binary);
    out << utf8;
    out.close();

    std::filesystem::path abs = std::filesystem::absolute(file);
    std::u8string u = abs.u8string();
    std::string pathUtf8(u.begin(), u.end());
    rulesFilePath = wxString::FromUTF8(pathUtf8);
}

#include "Validate.h"

#include <algorithm>
#include <cctype>

namespace validate {

namespace {

// 全角符号对应的 UTF-8 字节序列
const std::string FULLWIDTH_COMMA = "\xEF\xBC\x8C";     // ，
const std::string FULLWIDTH_SEMICOLON = "\xEF\xBC\x9B"; // ；
const std::string FULLWIDTH_COLON = "\xEF\xBC\x9A";     // ：
const std::string FULLWIDTH_LPAREN = "\xEF\xBC\x88";    // （
const std::string FULLWIDTH_RPAREN = "\xEF\xBC\x89";    // ）
const std::string LEFT_DQUOTE = "\xE2\x80\x9C";         // “
const std::string RIGHT_DQUOTE = "\xE2\x80\x9D";        // ”

std::string replaceAll(std::string s, const std::string &from,
                       const std::string &to) {
    if (from.empty())
        return s;
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}

std::string trim(const std::string &s) {
    auto a = std::find_if(s.begin(), s.end(),
                          [](unsigned char c) { return !std::isspace(c); });
    auto b = std::find_if(s.rbegin(), s.rend(),
                          [](unsigned char c) { return !std::isspace(c); });
    if (a == s.end())
        return "";
    return std::string(a, b.base());
}

} // namespace

std::string normalizeStudentInput(const std::string &raw) {
    // 全角 → 半角
    std::string s = raw;
    s = replaceAll(s, FULLWIDTH_COLON, ":");
    s = replaceAll(s, FULLWIDTH_SEMICOLON, ";");
    s = replaceAll(s, FULLWIDTH_COMMA, ",");
    // 分号/逗号 → 换行（一行多人拆成多行）
    s = replaceAll(s, ";", "\n");
    s = replaceAll(s, ",", "\n");

    // 按换行切分，再按空格切分（界面提示空格也用于区分人员）
    std::string out;
    size_t pos = 0;
    while (pos <= s.size()) {
        size_t nl = s.find('\n', pos);
        std::string line = (nl == std::string::npos)
                               ? s.substr(pos)
                               : s.substr(pos, nl - pos);
        pos = (nl == std::string::npos) ? s.size() + 1 : nl + 1;

        size_t sp = 0;
        while (sp <= line.size()) {
            size_t nextSp = line.find(' ', sp);
            std::string seg = (nextSp == std::string::npos)
                                  ? line.substr(sp)
                                  : line.substr(sp, nextSp - sp);
            sp = (nextSp == std::string::npos) ? line.size() + 1 : nextSp + 1;

            seg = trim(seg);
            if (seg.empty())
                continue;

            std::string name = seg;
            std::string sex = "男";
            size_t c = seg.find(':');
            if (c != std::string::npos) {
                name = trim(seg.substr(0, c));
                sex = trim(seg.substr(c + 1));
                if (sex.empty())
                    sex = "男";
            }
            if (name.empty())
                continue;
            out += name + ":" + sex + "\n";
        }
    }
    return out;
}

std::string normalizeRulesInput(const std::string &raw) {
    std::string s = raw;
    s = replaceAll(s, LEFT_DQUOTE, "\"");
    s = replaceAll(s, RIGHT_DQUOTE, "\"");
    s = replaceAll(s, FULLWIDTH_LPAREN, "(");
    s = replaceAll(s, FULLWIDTH_RPAREN, ")");
    s = replaceAll(s, FULLWIDTH_COMMA, ",");
    s = replaceAll(s, FULLWIDTH_COLON, ":");
    s = replaceAll(s, FULLWIDTH_SEMICOLON, ";");
    // 分号 → 换行
    s = replaceAll(s, ";", "\n");
    return s;
}

} // namespace validate

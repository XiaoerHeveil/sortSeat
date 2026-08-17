#include "sorting.h"
#include "exceptions.h"
#include "fileInput.h"
#include "student.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <sstream>

// ========== UTF-8 quote helpers ==========
static const std::string LQ = "\xE2\x80\x9C";
static const std::string RQ = "\xE2\x80\x9D";

static bool isLeftQuote(const std::string &s, size_t pos) {
    if (s[pos] == '"') return true;
    return pos + 2 < s.size() && s.substr(pos, 3) == LQ;
}
static bool isRightQuote(const std::string &s, size_t pos) {
    if (s[pos] == '"') return true;
    return pos + 2 < s.size() && s.substr(pos, 3) == RQ;
}
static std::string stripQuotes(const std::string &s) {
    if (s.empty()) return s;
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
        return s.substr(1, s.size() - 2);
    if (s.size() >= 6 && s.substr(0, 3) == LQ && s.substr(s.size() - 3) == RQ)
        return s.substr(3, s.size() - 6);
    return s;
}
static bool startsWithQuote(const std::string &s) {
    if (s.empty()) return false;
    if (s.front() == '"') return true;
    return s.size() >= 3 && s.substr(0, 3) == LQ;
}

// ========== Worksheet lazy-load cache ==========
struct WorksheetCache {
    bool loaded = false;
    std::string filePath;
    int fileType = -1; // 1=CSV, 2=XLSX
    std::map<std::string, std::vector<std::pair<std::string, double>>> columns;
};
static WorksheetCache gWorksheetCache;
static int gNextReserved = 254;

static std::string trimCsv(const std::string &s) {
    auto a = std::find_if(s.begin(), s.end(),
                          [](unsigned char c) { return !std::isspace(c); });
    auto b = std::find_if(s.rbegin(), s.rend(),
                          [](unsigned char c) { return !std::isspace(c); });
    if (a == s.end())
        return "";
    return std::string(a, b.base());
}

static std::vector<std::string> splitCsvLine(const std::string &line) {
    std::vector<std::string> fields;
    size_t pos = 0;
    while (pos <= line.size()) {
        size_t c = line.find(',', pos);
        if (c == std::string::npos) {
            fields.push_back(line.substr(pos));
            break;
        }
        fields.push_back(line.substr(pos, c - pos));
        pos = c + 1;
    }
    return fields;
}

// 从 CSV 读取指定标题列下所有学生的值（姓名 -> 数值），取代历史上的 getCellCSV 逐格查找
static std::map<std::string, double> readCsvColumn(const std::string &title) {
    std::map<std::string, double> result;
    std::ifstream in(std::filesystem::u8path(gWorksheetCache.filePath));
    if (!in)
        return result;

    std::string header;
    std::getline(in, header);
    if (!header.empty() && header.back() == '\r')
        header.pop_back();
    if (header.size() >= 3 && (unsigned char)header[0] == 0xEF &&
        (unsigned char)header[1] == 0xBB && (unsigned char)header[2] == 0xBF)
        header.erase(0, 3);

    auto headers = splitCsvLine(header);
    int titleCol = -1;
    for (size_t i = 0; i < headers.size(); ++i)
        if (trimCsv(headers[i]) == title) {
            titleCol = static_cast<int>(i);
            break;
        }
    if (titleCol < 0)
        return result;

    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.empty())
            continue;
        auto fields = splitCsvLine(line);
        if (static_cast<int>(fields.size()) <= titleCol)
            continue;
        std::string name = trimCsv(fields[0]);
        std::string valStr = trimCsv(fields[titleCol]);
        double value = 0.0;
        try {
            value = std::stod(valStr);
        } catch (...) {
            value = 0.0;
        }
        result[name] = value;
    }
    return result;
}

static void loadTitleColumn(const std::string &title,
    const std::vector<std::shared_ptr<Student>> &studentGroup) {
    if (gWorksheetCache.columns.count(title)) return; // already loaded

    std::vector<std::pair<std::string, double>> colData;

    if (gWorksheetCache.fileType == 2) { // XLSX
        OpenXLSX::XLDocument xlsx;
        xlsx.open(gWorksheetCache.filePath);
        auto ws = xlsx.workbook().worksheet("Sheet1");

        int col = getTitleColumnXLSX(ws, title);
        if (col < 0) return; // not found, will throw later

        for (int row = 2; ; ++row) {
            auto [cellValue, typeCode] = determineCellType(ws, row, col);
            if (typeCode == 0) break;
            std::string name = getNameXLSX(ws, row - 1);
            double value = 0.0;
            if (typeCode == 2)
                value = static_cast<double>(cellValue.get<int>());
            else if (typeCode == 3)
                value = cellValue.get<double>();
            else if (typeCode == 5) {
                try { value = std::stod(cellValue.get<std::string>()); }
                catch (...) { value = 0.0; }
            }
            colData.push_back({name, value});
        }
    } else if (gWorksheetCache.fileType == 1) { // CSV
        auto col = readCsvColumn(title);
        for (const auto &s : studentGroup) {
            auto it = col.find(s->getName());
            double value = it != col.end() ? it->second : 0.0;
            colData.push_back({s->getName(), value});
        }
    }
    gWorksheetCache.columns[title] = std::move(colData);
}

static const std::vector<std::pair<std::string, double>> *
getTitleData(const std::string &title,
             const std::vector<std::shared_ptr<Student>> &studentGroup) {
    loadTitleColumn(title, studentGroup);
    auto it = gWorksheetCache.columns.find(title);
    if (it == gWorksheetCache.columns.end()) return nullptr;
    return &it->second;
}

// ========== Existing: exchange, priority, sort ==========

void exchangeSeatNumber(int *seatSheet, int columns, const int &oneRow,
                        const int &oneColumn, const int &twoRow,
                        const int &twoColumn) {
    int temp = seatSheet[oneRow * columns + oneColumn];
    seatSheet[oneRow * columns + oneColumn] =
        seatSheet[twoRow * columns + twoColumn];
    seatSheet[twoRow * columns + twoColumn] = temp;
}

int parseFunctionPriority(const std::string &line) {
    auto start = std::find_if(line.begin(), line.end(),
                              [](unsigned char ch) { return !std::isspace(ch); });
    std::string trimmed(start, line.end());
    if (trimmed.empty()) return -1;
    if (trimmed.size() > 4 && trimmed.substr(0, 4) == "not ") {
        trimmed = trimmed.substr(4);
        auto ns = std::find_if(trimmed.begin(), trimmed.end(),
                               [](unsigned char ch) { return !std::isspace(ch); });
        trimmed = std::string(ns, trimmed.end());
    }
    if (trimmed.find("setSeat(") == 0) return 1;
    if (trimmed.find("overallSituation(") == 0) return 4;
    if (trimmed.find("on(") == 0) return 3;
    if (trimmed.find("setGender(") == 0) return 3;
    if (trimmed.find("groupCondition(") == 0) return 2;
    if (trimmed.find("condition(") == 0) return 3;
    if (trimmed.find("adjacent(") == 0) return 2;
    if (trimmed.find("deskmate(") == 0) return 2;
    return -1;
}

static std::string functionNameOf(const std::string &line) {
    auto start = std::find_if(line.begin(), line.end(),
                              [](unsigned char ch) { return !std::isspace(ch); });
    std::string trimmed(start, line.end());
    if (trimmed.size() > 4 && trimmed.substr(0, 4) == "not ") {
        trimmed = trimmed.substr(4);
        auto ns = std::find_if(trimmed.begin(), trimmed.end(),
                               [](unsigned char ch) { return !std::isspace(ch); });
        trimmed = std::string(ns, trimmed.end());
    }
    size_t p = trimmed.find('(');
    return p == std::string::npos ? trimmed : trimmed.substr(0, p);
}

void sortFunctionsByPriority(std::vector<std::string> &lines) {
    std::vector<std::string> b1, b4, b3setGender, b3rest, b2adj, b2rest;
    for (const auto &line : lines) {
        if (line.empty()) continue;
        switch (parseFunctionPriority(line)) {
        case 1: b1.push_back(line); break;
        case 4: b4.push_back(line); break;
        case 3:
            // setGender 在 on/condition 之前执行，使其设置的性别约束先于放置生效
            if (functionNameOf(line) == "setGender")
                b3setGender.push_back(line);
            else
                b3rest.push_back(line);
            break;
        case 2:
            // b2 桶内 adjacent 排在 groupCondition 之前，其余保持原文件相对顺序
            if (functionNameOf(line) == "adjacent")
                b2adj.push_back(line);
            else
                b2rest.push_back(line);
            break;
        default: break;
        }
    }
    std::vector<std::string> result;
    result.reserve(lines.size());
    result.insert(result.end(), b1.begin(), b1.end());
    result.insert(result.end(), b4.begin(), b4.end());
    result.insert(result.end(), b3setGender.begin(), b3setGender.end());
    result.insert(result.end(), b3rest.begin(), b3rest.end());
    result.insert(result.end(), b2adj.begin(), b2adj.end());
    result.insert(result.end(), b2rest.begin(), b2rest.end());
    lines = std::move(result);
}

// ========== Layer 1: Rule parsing ==========

SeatRule parseRuleLine(const std::string &line) {
    SeatRule rule;
    rule.isNot = false;

    auto start = std::find_if(line.begin(), line.end(),
                              [](unsigned char ch) { return !std::isspace(ch); });
    std::string trimmed(start, line.end());
    if (trimmed.empty())
        throw NullFunction("rule line is empty");

    if (trimmed.size() > 4 && trimmed.substr(0, 4) == "not ") {
        rule.isNot = true;
        trimmed = trimmed.substr(4);
        auto ns = std::find_if(trimmed.begin(), trimmed.end(),
                               [](unsigned char ch) { return !std::isspace(ch); });
        trimmed = std::string(ns, trimmed.end());
    }

    size_t parenPos = trimmed.find('(');
    if (parenPos == std::string::npos)
        throw NullFunction("missing '(' in rule: " + trimmed);
    rule.functionName = trimmed.substr(0, parenPos);

    size_t closeParen = trimmed.rfind(')');
    if (closeParen == std::string::npos || closeParen <= parenPos)
        throw NullFunction("missing ')' in rule: " + trimmed);

    std::string argsStr =
        trimmed.substr(parenPos + 1, closeParen - parenPos - 1);

    // Split by commas, respecting quotes
    std::vector<std::string> args;
    size_t i = 0;
    bool inQuote = false, inCQuote = false;
    std::string cur;
    while (i < argsStr.size()) {
        char ch = argsStr[i];
        if (!inQuote && !inCQuote && ch == '"') {
            inQuote = true; cur += ch;
        } else if (inQuote && ch == '"') {
            inQuote = false; cur += ch;
        } else if (!inQuote && !inCQuote && isLeftQuote(argsStr, i)) {
            inCQuote = true; cur += argsStr.substr(i, 3); i += 2;
        } else if (inCQuote && isRightQuote(argsStr, i)) {
            inCQuote = false; cur += argsStr.substr(i, 3); i += 2;
        } else if (!inQuote && !inCQuote && ch == ',') {
            args.push_back(cur); cur.clear();
        } else {
            cur += ch;
        }
        ++i;
    }
    if (!cur.empty()) args.push_back(cur);

    for (auto &arg : args) {
        auto a0 = std::find_if(arg.begin(), arg.end(), [](unsigned char c) {
            return !std::isspace(c);
        });
        arg.erase(arg.begin(), a0);
        auto a1 = std::find_if(arg.rbegin(), arg.rend(), [](unsigned char c) {
            return !std::isspace(c);
        });
        arg.erase(a1.base(), arg.end());
    }
    rule.args = std::move(args);
    return rule;
}

std::vector<SeatRule>
parseRuleLines(const std::vector<std::string> &lines) {
    std::vector<SeatRule> rules;
    for (const auto &line : lines) {
        auto first = std::find_if(line.begin(), line.end(),
                                  [](unsigned char ch) { return !std::isspace(ch); });
        if (first == line.end()) continue;
        rules.push_back(parseRuleLine(line));
    }
    return rules;
}

// ========== Layer 2: Student lookup ==========

int findStudentByName(
    const std::string &name,
    const std::vector<std::shared_ptr<Student>> &studentGroup) {
    std::string cleanName = stripQuotes(name);
    for (size_t i = 0; i < studentGroup.size(); ++i)
        if (studentGroup[i]->getName() == cleanName)
            return static_cast<int>(i);
    return -1;
}

static int findStudentOrThrow(
    const std::string &name,
    const std::vector<std::shared_ptr<Student>> &studentGroup) {
    int idx = findStudentByName(name, studentGroup);
    if (idx < 0)
        throw nameNotExistence("student not found: " + stripQuotes(name));
    return idx;
}

// ========== Condition evaluation helpers ==========

enum class CondOp { GE, LE, GT, LT, NE, EQ };

static bool evalSingleCond(CondOp op, int threshold, int value) {
    switch (op) {
    case CondOp::GE: return value >= threshold;
    case CondOp::LE: return value <= threshold;
    case CondOp::GT: return value > threshold;
    case CondOp::LT: return value < threshold;
    case CondOp::NE: return value != threshold;
    case CondOp::EQ: return value == threshold;
    }
    return true;
}

static bool evaluateCondition(const std::string &condStr, int position) {
    if (condStr == "-1") return true;
    std::string s = condStr;
    // 去掉首尾空白
    {
        auto a = std::find_if(s.begin(), s.end(),
                              [](unsigned char c) { return !std::isspace(c); });
        auto b = std::find_if(s.rbegin(), s.rend(),
                              [](unsigned char c) { return !std::isspace(c); });
        if (a == s.end())
            return true;
        s = std::string(a, b.base());
    }
    // 纯数字（如 on(..., 3) 的 "3"）视为「等于该行/列」
    if (!s.empty() &&
        std::all_of(s.begin(), s.end(),
                    [](unsigned char c) { return std::isdigit(c); }))
        return position == std::stoi(s);
    bool result = true, firstCond = true, useAnd = true;
    size_t pos = 0;
    while (pos < s.size()) {
        while (pos < s.size() &&
               std::isspace(static_cast<unsigned char>(s[pos]))) ++pos;
        if (pos >= s.size()) break;
        if (s.substr(pos, 3) == "and") { useAnd = true; pos += 3; continue; }
        if (s.substr(pos, 2) == "or")  { useAnd = false; pos += 2; continue; }
        CondOp op;
        if (s.substr(pos, 2) == ">=")      { op = CondOp::GE; pos += 2; }
        else if (s.substr(pos, 2) == "<=") { op = CondOp::LE; pos += 2; }
        else if (s.substr(pos, 2) == "!=") { op = CondOp::NE; pos += 2; }
        else if (s[pos] == '>')            { op = CondOp::GT; ++pos; }
        else if (s[pos] == '<')            { op = CondOp::LT; ++pos; }
        else if (s[pos] == '=')            { op = CondOp::EQ; ++pos; }
        else { ++pos; continue; }
        while (pos < s.size() &&
               std::isspace(static_cast<unsigned char>(s[pos]))) ++pos;
        std::string numStr;
        while (pos < s.size() &&
               std::isdigit(static_cast<unsigned char>(s[pos]))) {
            numStr += s[pos]; ++pos;
        }
        if (numStr.empty()) continue;
        int threshold = std::stoi(numStr);
        bool condVal = evalSingleCond(op, threshold, position);
        if (firstCond) { result = condVal; firstCond = false; }
        else result = useAnd ? (result && condVal) : (result || condVal);
    }
    return firstCond ? true : result;
}

// ========== Group (小组) helpers ==========

// 总列数 totalColumns 被均分为 groupCount 组，返回第 column 列（0-based）属于第几组（0-based）
int columnGroup(int column, int groupCount, int totalColumns) {
    if (totalColumns <= 0) return 0;
    if (groupCount <= 1) return 0;
    if (groupCount > totalColumns) groupCount = totalColumns;
    return column * groupCount / totalColumns;
}

// 返回第 group（1-based）组的列范围 [startCol, endCol]（0-based 闭区间）
static void groupColumnRange(int group, int groupCount, int totalColumns,
                             int &startCol, int &endCol) {
    if (totalColumns <= 0) { startCol = 0; endCol = -1; return; }
    if (groupCount <= 1) groupCount = 1;
    if (groupCount > totalColumns) groupCount = totalColumns;
    int g = group - 1;
    if (g < 0) g = 0;
    if (g >= groupCount) g = groupCount - 1;
    startCol = (g * totalColumns + groupCount - 1) / groupCount;
    endCol = ((g + 1) * totalColumns + groupCount - 1) / groupCount - 1;
}

// ========== Scope parser for groupCondition ==========

struct ScopeRect {
    int r0 = 0, c0 = 0, r1 = 0, c1 = 0; // r = 列范围(colIdx)，c = 行范围(rowIdx)
};

static ScopeRect parseScope(const std::string &scope, int rows, int columns,
                            int groupCount) {
    // rows = 总列数，columns = 总行数；ScopeRect.r = 列范围(colIdx)，ScopeRect.c = 行范围(rowIdx)
    ScopeRect rect = {0, 0, rows - 1, columns - 1}; // default: entire grid

    auto parseOne = [&](const std::string &token, int &v0, int &v1) {
        char type = token.back();
        int num = std::stoi(token.substr(0, token.size() - 1));
        if (type == 'G' || type == 'g') {
            groupColumnRange(num, groupCount, rows, v0, v1); // rows = 总列数
        } else if (type == 'C' || type == 'c') {
            v0 = num - 1; v1 = num - 1;
        } else if (type == 'R' || type == 'r') {
            v0 = num - 1; v1 = num - 1;
        }
    };

    auto isColumnToken = [](const std::string &token) {
        char t = token.back();
        return t == 'G' || t == 'g' || t == 'C' || t == 'c';
    };

    size_t colon = scope.find(':');
    if (colon == std::string::npos) {
        std::string token = scope;
        bool col = isColumnToken(token);
        int v0 = 0, v1 = 0;
        parseOne(token, v0, v1);
        if (col) { rect.r0 = v0; rect.r1 = v1; }
        else     { rect.c0 = v0; rect.c1 = v1; }
    } else {
        std::string left = scope.substr(0, colon);
        std::string right = scope.substr(colon + 1);
        bool col = isColumnToken(left);
        int lv0 = 0, lv1 = 0, rv0 = 0, rv1 = 0;
        parseOne(left, lv0, lv1);
        parseOne(right, rv0, rv1);
        if (col) { rect.r0 = lv0; rect.r1 = rv1; }
        else     { rect.c0 = lv0; rect.c1 = rv1; }
    }
    // Clamp
    if (rect.r0 < 0) rect.r0 = 0;
    if (rect.c0 < 0) rect.c0 = 0;
    if (rect.r1 >= rows) rect.r1 = rows - 1;
    if (rect.c1 >= columns) rect.c1 = columns - 1;
    return rect;
}

// Gender constraint table
static std::vector<char> gGenderConstraints;

// 学生性别码（'M'/'F'）
static char genderCode(int sidx,
                       const std::vector<std::shared_ptr<Student>> &studentGroup) {
    return studentGroup[sidx]->getSex() == "\xE7\x94\xB7" ? 'M' : 'F';
}

// 判断学生 sidx 能否放入 (colIdx, rowIdx)（遵守性别约束；无约束则放行）
static bool cellGenderOk(
    int sidx, int colIdx, int rowIdx, int columns,
    const std::vector<std::shared_ptr<Student>> &studentGroup) {
    if (gGenderConstraints.empty())
        return true;
    char constraint = gGenderConstraints[colIdx * columns + rowIdx];
    if (constraint != 'M' && constraint != 'F')
        return true;
    return genderCode(sidx, studentGroup) == constraint;
}

// ========== Deskmate placement helper ==========

static bool placeDeskmatePair(unsigned int *seatNumber, int rows, int columns,
                              int groupCount, unsigned int idx1,
                              unsigned int idx2,
                              const std::vector<std::shared_ptr<Student>> &studentGroup) {
    // rows = 总列数，columns = 总行数；网格列主序 seatNumber[colIdx * columns + rowIdx]
    for (int r = 0; r < columns; ++r) {          // r = rowIdx
        for (int c = 0; c < rows - 1; ++c) {     // c = colIdx
            if (columnGroup(c, groupCount, rows) !=
                columnGroup(c + 1, groupCount, rows))
                continue;
            if (seatNumber[c * columns + r] == EMPTY_SEAT &&
                seatNumber[(c + 1) * columns + r] == EMPTY_SEAT &&
                cellGenderOk(idx1, c, r, columns, studentGroup) &&
                cellGenderOk(idx2, c + 1, r, columns, studentGroup)) {
                seatNumber[c * columns + r] = idx1;
                seatNumber[(c + 1) * columns + r] = idx2;
                return true;
            }
        }
    }
    return false;
}

// ========== 模块级状态 ==========

// groupCondition fill_behavior=FALSE 时遗留的空位，constrainedFill 不回填
static std::vector<bool> gLeaveEmpty;
// 文本文件下被关闭的单元格数据规则提示（condition/groupCondition）
static std::string gSkipWarning;
// groupCondition 作用域与合格学生集合（供 adjacent force 判断）
struct GroupCondCtx {
    ScopeRect rect;
    std::vector<int> qualifying;
};
static std::vector<GroupCondCtx> gGroupCondCtx;

// ========== Layer 3: Rule handlers ==========

static void handleSetSeat(const SeatRule &rule, unsigned int *seatNumber,
                          int rows, int columns,
                          std::vector<std::shared_ptr<Student>> &studentGroup) {
    if (rule.args.size() < 3)
        throw functionParameterLacking("setSeat needs 3 args, got " +
                                       std::to_string(rule.args.size()));
    std::string name = stripQuotes(rule.args[0]);
    int x = std::stoi(rule.args[1]) - 1;
    int y = std::stoi(rule.args[2]) - 1;
    if (x < 0 || x >= rows || y < 0 || y >= columns)
        throw meaninglessFunction("setSeat position out of bounds: (" +
                                  std::to_string(x + 1) + "," +
                                  std::to_string(y + 1) + ")");
    int idx = findStudentOrThrow(name, studentGroup);
    for (int i = 0; i < rows * columns; ++i)
        if (seatNumber[i] == static_cast<unsigned int>(idx))
            seatNumber[i] = EMPTY_SEAT;
    seatNumber[x * columns + y] = static_cast<unsigned int>(idx);
}

static void handleAdjacent(const SeatRule &rule, unsigned int *seatNumber,
                           int rows, int columns,
                           std::vector<std::shared_ptr<Student>> &studentGroup,
                           bool isNot) {
    if (rule.args.size() < 3)
        throw functionParameterLacking("adjacent needs 3+ args");
    int radius = std::stoi(rule.args[1]);
    if (radius < 1) radius = 1;
    if (radius > 3) radius = 3;

    std::string centerName = stripQuotes(rule.args[0]);
    int centerIdx = findStudentOrThrow(centerName, studentGroup);
    int cx = -1, cy = -1;
    for (int r = 0; r < rows && cx < 0; ++r)
        for (int c = 0; c < columns && cx < 0; ++c)
            if (seatNumber[r * columns + c] ==
                static_cast<unsigned int>(centerIdx)) {
                cx = r; cy = c;
            }
    // 中心学生尚未就座：先为其找一个空位（遵守性别约束），再以它为中心安排周围学生
    if (cx < 0) {
        for (int r = 0; r < rows && cx < 0; ++r)          // r = colIdx
            for (int c = 0; c < columns && cx < 0; ++c)   // c = rowIdx
                if (seatNumber[r * columns + c] == EMPTY_SEAT &&
                    cellGenderOk(centerIdx, r, c, columns, studentGroup)) {
                    seatNumber[r * columns + c] =
                        static_cast<unsigned int>(centerIdx);
                    cx = r;
                    cy = c;
                }
    }
    if (cx < 0) return;

    if (isNot) {
        // not adjacent：args[2..] 的学生必须远离中心（不落在半径内）。
        // 直接把每个未就座的「周围」学生放到半径之外的空位，而非用 255 永久封锁
        // （永久封锁会导致他人也无法入座，进而漏人）。
        for (size_t i = 2; i < rule.args.size(); ++i) {
            int sidx = findStudentOrThrow(rule.args[i], studentGroup);
            bool seated = false;
            for (int k = 0; k < rows * columns; ++k)
                if (seatNumber[k] == static_cast<unsigned int>(sidx)) {
                    seated = true;
                    break;
                }
            if (seated) continue;
            bool placed = false;
            for (int r = 0; r < rows && !placed; ++r)          // r = colIdx
                for (int c = 0; c < columns && !placed; ++c)   // c = rowIdx
                    if (seatNumber[r * columns + c] == EMPTY_SEAT &&
                        (std::abs(r - cx) > radius ||
                         std::abs(c - cy) > radius) &&
                        cellGenderOk(sidx, r, c, columns, studentGroup)) {
                        seatNumber[r * columns + c] =
                            static_cast<unsigned int>(sidx);
                        placed = true;
                    }
        }
        return;
    }

    // 正向 adjacent：args[2..] 为周围名单，末尾可带布尔参数 force（TRUE/FALSE）
    std::vector<int> neighbors;
    bool force = false;
    for (size_t i = 2; i < rule.args.size(); ++i) {
        std::string a = rule.args[i];
        if (a == "TRUE" || a == "true") { force = true; continue; }
        if (a == "FALSE" || a == "false") { force = false; continue; }
        neighbors.push_back(findStudentOrThrow(a, studentGroup));
    }

    for (int sidx : neighbors) {
        bool seated = false;
        for (int i = 0; i < rows * columns; ++i)
            if (seatNumber[i] == static_cast<unsigned int>(sidx)) {
                seated = true; break;
            }
        if (seated) continue;

        bool placed = false;
        for (int dr = -radius; dr <= radius && !placed; ++dr)
            for (int dc = -radius; dc <= radius && !placed; ++dc) {
                int nr = cx + dr, nc = cy + dc;
                if (nr < 0 || nr >= rows || nc < 0 || nc >= columns)
                    continue;
                unsigned int &cell = seatNumber[nr * columns + nc];
                if (cell != EMPTY_SEAT) continue;

                // 性别约束
                if (!gGenderConstraints.empty()) {
                    char constraint = gGenderConstraints[nr * columns + nc];
                    if (constraint == 'M' || constraint == 'F') {
                        char want = studentGroup[sidx]->getSex() ==
                                            "\xE7\x94\xB7"
                                        ? 'M'
                                        : 'F';
                        if (want != constraint) continue;
                    }
                }
                // force=false 时，不把非合格学生放入 groupCondition 作用域
                if (!force) {
                    bool blocked = false;
                    for (const auto &ctx : gGroupCondCtx) {
                        if (nr >= ctx.rect.r0 && nr <= ctx.rect.r1 &&
                            nc >= ctx.rect.c0 && nc <= ctx.rect.c1) {
                            bool q = std::find(ctx.qualifying.begin(),
                                               ctx.qualifying.end(),
                                               sidx) != ctx.qualifying.end();
                            if (!q) { blocked = true; break; }
                        }
                    }
                    if (blocked) continue;
                }
                cell = static_cast<unsigned int>(sidx);
                placed = true;
            }
    }
}

static void handleOn(const SeatRule &rule, unsigned int *seatNumber,
                     int rows, int columns,
                     std::vector<std::shared_ptr<Student>> &studentGroup,
                     bool isNot) {
    if (rule.args.size() < 3)
        throw functionParameterLacking("on needs 3+ args");

    std::vector<std::string> names;
    std::vector<std::string> conditions;
    for (const auto &arg : rule.args) {
        if (startsWithQuote(arg))
            names.push_back(stripQuotes(arg));
        else
            conditions.push_back(arg);
    }
    if (names.empty())
        throw functionParameterLacking("on needs at least 1 name");

    std::string hCond = conditions.size() > 0 ? conditions[0] : "-1";
    std::string vCond = conditions.size() > 1 ? conditions[1] : "-1";

    std::vector<int> studentIndices;
    for (const auto &name : names)
        studentIndices.push_back(findStudentOrThrow(name, studentGroup));

    for (int sidx : studentIndices) {
        bool placed = false;
        for (int i = 0; i < rows * columns; ++i)
            if (seatNumber[i] == static_cast<unsigned int>(sidx)) {
                placed = true; break;
            }
        if (placed) continue;
        for (int c = 0; c < columns; ++c) {          // 行优先：先横向铺满再下一行
            for (int r = 0; r < rows; ++r) {
                if (seatNumber[r * columns + c] != EMPTY_SEAT) continue;
                if (!cellGenderOk(sidx, r, c, columns, studentGroup)) continue;
                bool hOk = evaluateCondition(hCond, r + 1);
                bool vOk = evaluateCondition(vCond, c + 1);
                bool condOk = isNot ? (!hOk || !vOk) : (hOk && vOk);
                if (condOk) {
                    seatNumber[r * columns + c] = static_cast<unsigned int>(sidx);
                    placed = true; break;
                }
            }
            if (placed) break;
        }
    }
}

static void handleDeskmate(const SeatRule &rule, unsigned int *seatNumber,
                           int rows, int columns, int groupCount,
                           std::vector<std::shared_ptr<Student>> &studentGroup) {
    if (rule.args.size() < 2)
        throw functionParameterLacking("deskmate needs 2 args");
    std::string name1 = stripQuotes(rule.args[0]);
    std::string name2 = stripQuotes(rule.args[1]);
    int idx1 = findStudentOrThrow(name1, studentGroup);
    int idx2 = findStudentOrThrow(name2, studentGroup);

    bool p1 = false, p2 = false;
    for (int i = 0; i < rows * columns; ++i) {
        if (seatNumber[i] == static_cast<unsigned int>(idx1)) p1 = true;
        if (seatNumber[i] == static_cast<unsigned int>(idx2)) p2 = true;
    }
    if (p1 && p2) return;

    if (!placeDeskmatePair(seatNumber, rows, columns, groupCount,
                           static_cast<unsigned int>(idx1),
                           static_cast<unsigned int>(idx2), studentGroup)) {
        // 若其中一个已被安排，尝试把另一个放到其左右相邻列（同一行、同组）
        for (int r = 0; r < columns; ++r) {          // r = rowIdx
            for (int c = 0; c < rows; ++c) {         // c = colIdx
                if (seatNumber[c * columns + r] ==
                    static_cast<unsigned int>(idx1)) {
                    if (c > 0 && seatNumber[(c - 1) * columns + r] == EMPTY_SEAT &&
                        columnGroup(c - 1, groupCount, rows) ==
                            columnGroup(c, groupCount, rows) &&
                        cellGenderOk(idx2, c - 1, r, columns, studentGroup)) {
                        seatNumber[(c - 1) * columns + r] =
                            static_cast<unsigned int>(idx2);
                        return;
                    }
                    if (c + 1 < rows && seatNumber[(c + 1) * columns + r] == EMPTY_SEAT &&
                        columnGroup(c + 1, groupCount, rows) ==
                            columnGroup(c, groupCount, rows) &&
                        cellGenderOk(idx2, c + 1, r, columns, studentGroup)) {
                        seatNumber[(c + 1) * columns + r] =
                            static_cast<unsigned int>(idx2);
                        return;
                    }
                }
                if (seatNumber[c * columns + r] ==
                    static_cast<unsigned int>(idx2)) {
                    if (c > 0 && seatNumber[(c - 1) * columns + r] == EMPTY_SEAT &&
                        columnGroup(c - 1, groupCount, rows) ==
                            columnGroup(c, groupCount, rows) &&
                        cellGenderOk(idx1, c - 1, r, columns, studentGroup)) {
                        seatNumber[(c - 1) * columns + r] =
                            static_cast<unsigned int>(idx1);
                        return;
                    }
                    if (c + 1 < rows && seatNumber[(c + 1) * columns + r] == EMPTY_SEAT &&
                        columnGroup(c + 1, groupCount, rows) ==
                            columnGroup(c, groupCount, rows) &&
                        cellGenderOk(idx1, c + 1, r, columns, studentGroup)) {
                        seatNumber[(c + 1) * columns + r] =
                            static_cast<unsigned int>(idx1);
                        return;
                    }
                }
            }
        }
    }
}

static void handleSetGender(const SeatRule &rule, unsigned int *,
                            int rows, int columns,
                            std::vector<std::shared_ptr<Student>> &) {
    if (rule.args.size() < 2)
        throw functionParameterLacking("setGender needs 2+ args");
    std::string gs = stripQuotes(rule.args[0]);
    char gender = 0;
    if (gs == "male" || gs == "\xE7\x94\xB7") gender = 'M';
    else if (gs == "female" || gs == "\xE5\xA5\xB3") gender = 'F';
    else throw meaninglessFunction("setGender: unknown gender " + gs);

    int row = std::stoi(rule.args[1]);
    int col = -1;
    if (rule.args.size() >= 3) col = std::stoi(rule.args[2]);

    if (gGenderConstraints.empty())
        gGenderConstraints.resize(rows * columns, 0);

    if (row != -1 && row > 0 && row <= rows)
        for (int c = 0; c < columns; ++c)
            gGenderConstraints[(row - 1) * columns + c] = gender;
    if (col != -1 && col > 0 && col <= columns)
        for (int r = 0; r < rows; ++r)
            gGenderConstraints[r * columns + col - 1] = gender;
}

static void handleOverallSituation(const SeatRule &rule, unsigned int *,
                                   int rows, int columns, int groupCount,
                                   std::vector<std::shared_ptr<Student>> &) {
    if (rule.args.empty())
        throw functionParameterLacking("overallSituation needs args");
    if (gGenderConstraints.empty())
        gGenderConstraints.resize(rows * columns, 0);

    int numGroups = groupCount;
    for (int g = 0; g < numGroups; ++g) {
        int startCol, endCol;
        groupColumnRange(g + 1, groupCount, rows, startCol, endCol);
        int width = endCol - startCol + 1;
        for (int p = 0;
             p < width && p < static_cast<int>(rule.args.size()); ++p) {
            char gender = 0;
            std::string gs = stripQuotes(rule.args[p]);
            if (gs == "\xE7\x94\xB7" || gs == "male") gender = 'M';
            else if (gs == "\xE5\xA5\xB3" || gs == "female") gender = 'F';
            if (gender == 0) continue;
            int col = startCol + p;
            for (int r = 0; r < columns; ++r) // r = rowIdx
                gGenderConstraints[col * columns + r] = gender;
        }
    }
}

// 用函数名 + 原始参数重建规则文本（用于向用户提示被关闭的规则行）
static std::string rebuildRuleText(const std::string &fn,
                                   const std::vector<std::string> &args) {
    std::string s = fn + "(";
    for (size_t i = 0; i < args.size(); ++i) {
        if (i) s += ", ";
        s += args[i];
    }
    s += ")";
    return s;
}

// ========== condition handler (full implementation) ==========

static void handleCondition(const SeatRule &rule, unsigned int *seatNumber,
                            int rows, int columns, int groupCount,
                            std::vector<std::shared_ptr<Student>> &studentGroup,
                            int fileType) {
    if (fileType != 1 && fileType != 2) {
        if (!gSkipWarning.empty()) gSkipWarning += "\n";
        gSkipWarning += rebuildRuleText("condition", rule.args);
        return;
    }
    // args: title, greater|less, [lead|cooperate], [min], [max]
    if (rule.args.size() < 2)
        throw functionParameterLacking("condition needs 2+ args (title, greater|less, ...)");
    if (rule.args.size() > 5)
        throw functionAdditionalParameters("condition takes at most 5 args, got " +
                                           std::to_string(rule.args.size()));

    std::string title = stripQuotes(rule.args[0]);
    std::string order = rule.args[1];              // greater / less
    std::string mode =
        rule.args.size() > 2 ? rule.args[2] : "cooperate"; // lead / cooperate
    double minVal = rule.args.size() > 3 ? std::stod(rule.args[3])
                                         : -std::numeric_limits<double>::max();
    double maxVal = rule.args.size() > 4 ? std::stod(rule.args[4])
                                         : std::numeric_limits<double>::max();

    auto *data = getTitleData(title, studentGroup);
    if (!data)
        throw titleNotExistence("title not found: " + title);

    // Extract (name, value, studentIndex) for students not yet placed
    struct Entry {
        std::string name;
        double value;
        int sidx;
    };
    std::vector<Entry> entries;
    for (const auto &[name, value] : *data) {
        if (value <= minVal || value > maxVal) continue;
        int sidx = findStudentByName(name, studentGroup);
        if (sidx < 0) continue;
        // Check not already seated
        bool seated = false;
        for (int i = 0; i < rows * columns; ++i) {
            if (seatNumber[i] == static_cast<unsigned int>(sidx)) {
                seated = true; break;
            }
        }
        if (!seated)
            entries.push_back({name, value, sidx});
    }
    if (entries.empty()) return;

    bool descending = (order == "greater");
    std::sort(entries.begin(), entries.end(),
              [descending](const Entry &a, const Entry &b) {
                  return descending ? a.value > b.value : a.value < b.value;
              });

    // Pair students
    bool leadMode = (mode == "lead");
    size_t n = entries.size();
    std::vector<std::pair<int, int>> pairs;
    if (leadMode) {
        for (size_t i = 0; i < n / 2; ++i)
            pairs.push_back({entries[i].sidx, entries[n - 1 - i].sidx});
        if (n % 2 == 1)
            pairs.push_back({entries[n / 2].sidx, -1});
    } else {
        for (size_t i = 0; i + 1 < n; i += 2)
            pairs.push_back({entries[i].sidx, entries[i + 1].sidx});
        if (n % 2 == 1)
            pairs.push_back({entries.back().sidx, -1});
    }

    for (auto &[a, b] : pairs) {
        if (b < 0) {
            // Single student, place anywhere（遵守性别约束）
            for (int i = 0; i < rows * columns; ++i) {
                if (seatNumber[i] == EMPTY_SEAT &&
                    cellGenderOk(a, i / columns, i % columns, columns, studentGroup)) {
                    seatNumber[i] = static_cast<unsigned int>(a);
                    break;
                }
            }
        } else {
            placeDeskmatePair(seatNumber, rows, columns, groupCount,
                              static_cast<unsigned int>(a),
                              static_cast<unsigned int>(b), studentGroup);
        }
    }
}

// 计算 groupCondition 的合格学生下标集合（greater/less + base_point，数据层面）
static std::vector<int> computeQualifyingStudents(
    const std::string &title, const std::string &order, double basePoint,
    const std::vector<std::shared_ptr<Student>> &studentGroup) {
    auto *data = getTitleData(title, studentGroup);
    if (!data)
        return {};
    bool greaterMode = (order == "greater");
    std::vector<int> out;
    for (const auto &[name, value] : *data) {
        int sidx = findStudentByName(name, studentGroup);
        if (sidx < 0)
            continue;
        bool above = (value > basePoint);
        if (greaterMode ? above : !above)
            out.push_back(sidx);
    }
    return out;
}

// ========== groupCondition handler (full implementation) ==========

static void
handleGroupCondition(const SeatRule &rule, unsigned int *seatNumber, int rows,
                     int columns, int groupCount,
                     std::vector<std::shared_ptr<Student>> &studentGroup,
                     int fileType) {
    if (fileType != 1 && fileType != 2) {
        if (!gSkipWarning.empty()) gSkipWarning += "\n";
        gSkipWarning += rebuildRuleText("groupCondition", rule.args);
        return;
    }
    // args: title, greater|less, scope, base_point, [offset_row],
    //       [offset_column], [fill_behavior]
    if (rule.args.size() < 4)
        throw functionParameterLacking(
            "groupCondition needs 4+ args (title, greater|less, scope, "
            "base_point, ...)");
    if (rule.args.size() > 7)
        throw functionAdditionalParameters(
            "groupCondition takes at most 7 args, got " +
            std::to_string(rule.args.size()));

    std::string title = stripQuotes(rule.args[0]);
    std::string order = rule.args[1];    // greater / less
    std::string scope = rule.args[2];
    double basePoint = std::stod(rule.args[3]);
    // 可选参数：offset_row / offset_column 为整数，fill_behavior 为 TRUE/FALSE；
    // 允许省略偏移直接填 fill_behavior（如 groupCondition(..., 200, TRUE)）。
    int offsetRow = 0;
    int offsetCol = 0;
    bool fillBehavior = true;
    int intCount = 0;
    for (size_t i = 4; i < rule.args.size(); ++i) {
        const std::string &a = rule.args[i];
        if (a == "TRUE" || a == "true") { fillBehavior = true; break; }
        if (a == "FALSE" || a == "false") { fillBehavior = false; break; }
        try {
            int v = std::stoi(a);
            if (intCount == 0) offsetRow = v;
            else if (intCount == 1) offsetCol = v;
            ++intCount;
        } catch (...) {
            break; // 无法识别的尾随参数，忽略
        }
    }

    // Check reserved number
    if (gNextReserved < 250) {
        std::cout << "[hint] groupCondition limit (5) reached, skipped"
                  << std::endl;
        return;
    }
    int reservedNum = gNextReserved--;

    auto *data = getTitleData(title, studentGroup);
    if (!data)
        throw titleNotExistence("title not found: " + title);

    // Parse scope（rect.r = 列范围，rect.c = 行范围）
    ScopeRect rect = parseScope(scope, rows, columns, groupCount);
    // Apply offsets (offset_row = 偏移列，offset_column = 偏移行)
    rect.r0 += offsetRow;  rect.r1 += offsetRow;
    rect.c0 += offsetCol;  rect.c1 += offsetCol;
    // Clamp
    rect.r0 = std::max(0, std::min(rect.r0, rows - 1));
    rect.r1 = std::max(0, std::min(rect.r1, rows - 1));
    rect.c0 = std::max(0, std::min(rect.c0, columns - 1));
    rect.c1 = std::max(0, std::min(rect.c1, columns - 1));

    // Mark scope area with reserved number
    for (int r = rect.r0; r <= rect.r1; ++r)
        for (int c = rect.c0; c <= rect.c1; ++c) {
            unsigned int &cell = seatNumber[r * columns + c];
            // Reserved num smaller = higher priority. Smaller can't be
            // overwritten by larger, unless fillBehavior=TRUE.
            if (cell == EMPTY_SEAT ||
                (fillBehavior && cell > static_cast<unsigned int>(reservedNum)))
                cell = static_cast<unsigned int>(reservedNum);
        }

    // Find students above/below base_point, not yet seated
    std::vector<int> qualifyingIdx =
        computeQualifyingStudents(title, order, basePoint, studentGroup);
    struct StudentVal {
        int sidx;
        double value;
        double dist; // distance from base_point
    };
    std::vector<StudentVal> qualifying, others;
    for (const auto &[name, value] : *data) {
        int sidx = findStudentByName(name, studentGroup);
        if (sidx < 0) continue;
        bool seated = false;
        for (int i = 0; i < rows * columns; ++i) {
            if (seatNumber[i] == static_cast<unsigned int>(sidx)) {
                seated = true; break;
            }
        }
        if (seated) continue;
        StudentVal sv = {sidx, value, std::abs(value - basePoint)};
        bool q = std::find(qualifyingIdx.begin(), qualifyingIdx.end(), sidx) !=
                 qualifyingIdx.end();
        if (q) qualifying.push_back(sv);
        else others.push_back(sv);
    }
    // Sort qualifying by distance from base_point (closest first)
    std::sort(qualifying.begin(), qualifying.end(),
              [](const StudentVal &a, const StudentVal &b) {
                  return a.dist < b.dist;
              });

    // Fill qualifying students into reserved positions（遵守性别约束）
    size_t qi = 0;
    for (int r = rect.r0; r <= rect.r1 && qi < qualifying.size(); ++r)
        for (int c = rect.c0; c <= rect.c1 && qi < qualifying.size(); ++c) {
            if (seatNumber[r * columns + c] !=
                static_cast<unsigned int>(reservedNum))
                continue;
            if (!gGenderConstraints.empty()) {
                char constraint = gGenderConstraints[r * columns + c];
                if (constraint == 'M' || constraint == 'F') {
                    char want = studentGroup[qualifying[qi].sidx]->getSex() ==
                                        "\xE7\x94\xB7"
                                    ? 'M'
                                    : 'F';
                    if (want != constraint) continue;
                }
            }
            seatNumber[r * columns + c] =
                static_cast<unsigned int>(qualifying[qi].sidx);
            ++qi;
        }

    // If fillBehavior, fill remaining reserved positions
    if (fillBehavior) {
        // Merge remaining qualifying + others, sorted by distance
        std::vector<StudentVal> remaining;
        for (size_t i = qi; i < qualifying.size(); ++i)
            remaining.push_back(qualifying[i]);
        std::sort(others.begin(), others.end(),
                  [](const StudentVal &a, const StudentVal &b) {
                      return a.dist < b.dist;
                  });
        remaining.insert(remaining.end(), others.begin(), others.end());

        size_t ri = 0;
        for (int r = rect.r0; r <= rect.r1 && ri < remaining.size(); ++r)
            for (int c = rect.c0; c <= rect.c1 && ri < remaining.size(); ++c) {
                if (seatNumber[r * columns + c] !=
                    static_cast<unsigned int>(reservedNum))
                    continue;
                if (!gGenderConstraints.empty()) {
                    char constraint = gGenderConstraints[r * columns + c];
                    if (constraint == 'M' || constraint == 'F') {
                        char want = studentGroup[remaining[ri].sidx]->getSex() ==
                                            "\xE7\x94\xB7"
                                        ? 'M'
                                        : 'F';
                        if (want != constraint) continue;
                    }
                }
                seatNumber[r * columns + c] =
                    static_cast<unsigned int>(remaining[ri].sidx);
                ++ri;
            }
    }

    // Clear any unreserved markers back to empty；fill_behavior=FALSE 时标记遗留空位
    for (int r = rect.r0; r <= rect.r1; ++r)
        for (int c = rect.c0; c <= rect.c1; ++c)
            if (seatNumber[r * columns + c] ==
                static_cast<unsigned int>(reservedNum)) {
                seatNumber[r * columns + c] = EMPTY_SEAT;
                if (!fillBehavior)
                    gLeaveEmpty[r * columns + c] = true;
            }
}

// ========== executeRules ==========

std::string
executeRules(const std::vector<SeatRule> &rules, unsigned int *seatNumber,
             int rows, int columns, int groupCount,
             std::vector<std::shared_ptr<Student>> &studentGroup,
             const std::string &filePath, int fileType) {
    gGenderConstraints.assign(rows * columns, 0);
    gLeaveEmpty.assign(rows * columns, false);
    gSkipWarning.clear();
    gGroupCondCtx.clear();
    gNextReserved = 254;
    gWorksheetCache = WorksheetCache{};
    gWorksheetCache.filePath = filePath;
    gWorksheetCache.fileType = fileType;

    // 预扫描：收集 groupCondition 的作用域与合格学生集合（供 adjacent force 判断）。
    // 规则已按优先级排序且 b2 内 adjacent 在 groupCondition 之前，故在首次遇到
    // b2 规则时惰性执行（此时 b1/b4/b3 已处理完毕）。
    bool preScanned = false;
    auto runPrescan = [&]() {
        if (preScanned) return;
        preScanned = true;
        if (fileType != 1 && fileType != 2) return;
        for (const auto &r : rules) {
            if (r.functionName != "groupCondition" || r.args.size() < 4)
                continue;
            std::string title = stripQuotes(r.args[0]);
            std::string order = r.args[1];
            std::string scope = r.args[2];
            double basePoint = std::stod(r.args[3]);
            GroupCondCtx ctx;
            ctx.rect = parseScope(scope, rows, columns, groupCount);
            int offsetRow = 0, offsetCol = 0, intCount = 0;
            for (size_t i = 4; i < r.args.size(); ++i) {
                const std::string &a = r.args[i];
                if (a == "TRUE" || a == "true" || a == "FALSE" || a == "false")
                    break;
                try {
                    int v = std::stoi(a);
                    if (intCount == 0) offsetRow = v;
                    else if (intCount == 1) offsetCol = v;
                    ++intCount;
                } catch (...) { break; }
            }
            ctx.rect.r0 += offsetRow; ctx.rect.r1 += offsetRow;
            ctx.rect.c0 += offsetCol; ctx.rect.c1 += offsetCol;
            ctx.rect.r0 = std::max(0, std::min(ctx.rect.r0, rows - 1));
            ctx.rect.r1 = std::max(0, std::min(ctx.rect.r1, rows - 1));
            ctx.rect.c0 = std::max(0, std::min(ctx.rect.c0, columns - 1));
            ctx.rect.c1 = std::max(0, std::min(ctx.rect.c1, columns - 1));
            ctx.qualifying =
                computeQualifyingStudents(title, order, basePoint, studentGroup);
            gGroupCondCtx.push_back(std::move(ctx));
        }
    };

    for (const auto &rule : rules) {
        try {
            if (rule.functionName == "setSeat") {
                handleSetSeat(rule, seatNumber, rows, columns, studentGroup);
            } else if (rule.functionName == "adjacent") {
                runPrescan();
                handleAdjacent(rule, seatNumber, rows, columns, studentGroup,
                               rule.isNot);
            } else if (rule.functionName == "on") {
                handleOn(rule, seatNumber, rows, columns, studentGroup,
                         rule.isNot);
            } else if (rule.functionName == "deskmate") {
                runPrescan();
                handleDeskmate(rule, seatNumber, rows, columns, groupCount,
                               studentGroup);
            } else if (rule.functionName == "setGender") {
                handleSetGender(rule, seatNumber, rows, columns, studentGroup);
            } else if (rule.functionName == "overallSituation") {
                handleOverallSituation(rule, seatNumber, rows, columns,
                                       groupCount, studentGroup);
            } else if (rule.functionName == "condition") {
                handleCondition(rule, seatNumber, rows, columns, groupCount,
                                studentGroup, fileType);
            } else if (rule.functionName == "groupCondition") {
                runPrescan();
                handleGroupCondition(rule, seatNumber, rows, columns, groupCount,
                                     studentGroup, fileType);
            } else {
                throw inexistentFunction("unknown function: " +
                                         rule.functionName);
            }
        } catch (const std::exception &e) {
            std::cerr << "[error] " << e.what() << " (rule: "
                      << (rule.isNot ? "not " : "") << rule.functionName
                      << ")" << std::endl;
        }
    }
    return gSkipWarning;
}

// ========== Layer 4: Random fill ==========

void randomFill(unsigned int *seatNumber, int rows, int columns,
                const std::vector<std::shared_ptr<Student>> &studentGroup) {
    std::vector<int> emptyPositions;
    for (int i = 0; i < rows * columns; ++i)
        if (seatNumber[i] == EMPTY_SEAT) emptyPositions.push_back(i);

    int numStudents = static_cast<int>(studentGroup.size());
    std::vector<bool> placed(numStudents, false);
    for (int i = 0; i < rows * columns; ++i) {
        unsigned int v = seatNumber[i];
        if (v != EMPTY_SEAT && v < 250 && v < static_cast<unsigned int>(numStudents))
            placed[v] = true;
    }
    std::vector<int> unplaced;
    for (int i = 0; i < numStudents; ++i)
        if (!placed[i]) unplaced.push_back(i);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(unplaced.begin(), unplaced.end(), gen);

    size_t fillCount = std::min(emptyPositions.size(), unplaced.size());
    for (size_t i = 0; i < fillCount; ++i)
        seatNumber[emptyPositions[i]] = static_cast<unsigned int>(unplaced[i]);
}

void constrainedFill(unsigned int *seatNumber, int rows, int columns,
                     int /*groupCount*/,
                     const std::vector<std::shared_ptr<Student>> &studentGroup) {
    // Clear any remaining reserved numbers (250-254) to empty
    for (int i = 0; i < rows * columns; ++i)
        if (seatNumber[i] >= 250 && seatNumber[i] <= 254)
            seatNumber[i] = EMPTY_SEAT;

    // 可填充的空位（跳过 groupCondition fill_behavior=FALSE 遗留的 gLeaveEmpty 格）
    std::vector<int> emptyPositions;
    for (int i = 0; i < rows * columns; ++i)
        if (seatNumber[i] == EMPTY_SEAT &&
            (gLeaveEmpty.empty() || !gLeaveEmpty[i]))
            emptyPositions.push_back(i);

    int numStudents = static_cast<int>(studentGroup.size());
    std::vector<bool> placed(numStudents, false);
    for (int i = 0; i < rows * columns; ++i) {
        unsigned int v = seatNumber[i];
        if (v != EMPTY_SEAT && v < 250 && v < static_cast<unsigned int>(numStudents))
            placed[v] = true;
    }
    std::vector<int> unplaced;
    for (int i = 0; i < numStudents; ++i)
        if (!placed[i]) unplaced.push_back(i);

    std::mt19937 gen(42);
    std::shuffle(unplaced.begin(), unplaced.end(), gen);

    std::vector<int> maleStudents, femaleStudents;
    for (int idx : unplaced) {
        if (studentGroup[idx]->getSex() == "\xE7\x94\xB7")
            maleStudents.push_back(idx);
        else
            femaleStudents.push_back(idx);
    }

    auto constraintAt = [&](int pos) -> char {
        if (gGenderConstraints.empty()) return 0;
        return gGenderConstraints[pos];
    };

    size_t mi = 0, fi = 0;

    // 第一轮：性别匹配填充（男→男列、女→女列）
    for (int pos : emptyPositions) {
        char constraint = constraintAt(pos);
        if (constraint == 'M' && mi < maleStudents.size())
            seatNumber[pos] = static_cast<unsigned int>(maleStudents[mi++]);
        else if (constraint == 'F' && fi < femaleStudents.size())
            seatNumber[pos] = static_cast<unsigned int>(femaleStudents[fi++]);
    }
    // 第二轮：无约束格填充剩余学生
    for (int pos : emptyPositions) {
        if (seatNumber[pos] != EMPTY_SEAT) continue;
        char constraint = constraintAt(pos);
        if (constraint == 'M' || constraint == 'F') continue;
        if (mi < maleStudents.size())
            seatNumber[pos] = static_cast<unsigned int>(maleStudents[mi++]);
        else if (fi < femaleStudents.size())
            seatNumber[pos] = static_cast<unsigned int>(femaleStudents[fi++]);
    }
    // 第三轮：溢出回填——性别不均衡时，将剩余学生填到异性列，确保不漏人
    for (int pos : emptyPositions) {
        if (seatNumber[pos] != EMPTY_SEAT) continue;
        if (mi < maleStudents.size())
            seatNumber[pos] = static_cast<unsigned int>(maleStudents[mi++]);
        else if (fi < femaleStudents.size())
            seatNumber[pos] = static_cast<unsigned int>(femaleStudents[fi++]);
    }
}

// ========== Layer 5: Print layout ==========

void printSeatLayout(
    const unsigned int *seatNumber, int rows, int columns,
    const std::vector<std::shared_ptr<Student>> &studentGroup) {
    // 参数约定：rows = 总列数，columns = 总行数；网格列主序 seatNumber[列*columns+行]
    std::cout << "\n========== Seat Layout ==========\n";
    std::cout << "(Facing blackboard, origin at top-left)\n\n";
    std::cout << "Col:\t";
    for (int c = 0; c < rows; ++c)
        std::cout << c + 1 << "\t";
    std::cout << "\n----";
    for (int c = 0; c < rows; ++c)
        std::cout << "--------";
    std::cout << "\n";
    for (int r = 0; r < columns; ++r) {
        std::cout << "Row" << r + 1 << "\t";
        for (int c = 0; c < rows; ++c) {
            unsigned int val = seatNumber[c * columns + r];
            if (val == EMPTY_SEAT) std::cout << "-";
            else if (val == 255) std::cout << "X";
            else if (val < studentGroup.size())
                std::cout << studentGroup[val]->getName();
            else std::cout << "?";
            std::cout << "\t";
        }
        std::cout << "\n";
    }
    std::cout << "\n==================================\n";
}

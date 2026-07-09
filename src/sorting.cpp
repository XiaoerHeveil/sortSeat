#include "sorting.h"
#include "exceptions.h"
#include "fileInput.h"
#include "student.h"
#include <algorithm>
#include <cctype>
#include <cmath>
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
        for (const auto &s : studentGroup) {
            std::ifstream in(gWorksheetCache.filePath);
            std::string valStr = getCellCSV(in, title, s->getName());
            double value = 0.0;
            try { value = std::stod(valStr); }
            catch (...) { value = 0.0; }
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

void sortFunctionsByPriority(std::vector<std::string> &lines) {
    std::vector<std::string> b1, b4, b3, b2;
    for (const auto &line : lines) {
        if (line.empty()) continue;
        switch (parseFunctionPriority(line)) {
        case 1: b1.push_back(line); break;
        case 4: b4.push_back(line); break;
        case 3: b3.push_back(line); break;
        case 2: b2.push_back(line); break;
        default: break;
        }
    }
    std::vector<std::string> result;
    result.reserve(lines.size());
    result.insert(result.end(), b1.begin(), b1.end());
    result.insert(result.end(), b4.begin(), b4.end());
    result.insert(result.end(), b3.begin(), b3.end());
    result.insert(result.end(), b2.begin(), b2.end());
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

// ========== Scope parser for groupCondition ==========

struct ScopeRect {
    int r0 = 0, c0 = 0, r1 = 0, c1 = 0; // inclusive [r0,r1] x [c0,c1]
};

static ScopeRect parseScope(const std::string &scope, int rows, int columns,
                            int groupCols) {
    ScopeRect rect = {0, 0, rows - 1, columns - 1}; // default: entire grid

    auto parseOne = [&](const std::string &token, int &v0, int &v1, bool isRow) {
        char type = token.back();
        int num = std::stoi(token.substr(0, token.size() - 1));
        if (type == 'G' || type == 'g') {
            int s = (num - 1) * groupCols;
            int e = num * groupCols - 1;
            v0 = s; v1 = e;
        } else if (type == 'C' || type == 'c') {
            v0 = num - 1; v1 = num - 1;
        } else if (type == 'R' || type == 'r') {
            v0 = num - 1; v1 = num - 1;
        }
    };

    size_t colon = scope.find(':');
    if (colon == std::string::npos) {
        std::string token = scope;
        char type = token.back();
        bool isRow = (type == 'R' || type == 'r');
        int v0 = 0, v1 = 0;
        parseOne(token, v0, v1, isRow);
        if (isRow) { rect.r0 = v0; rect.r1 = v1; }
        else       { rect.c0 = v0; rect.c1 = v1; }
    } else {
        std::string left = scope.substr(0, colon);
        std::string right = scope.substr(colon + 1);
        char lt = left.back(), rt = right.back();
        bool lr = (lt == 'R' || lt == 'r');
        int lv0 = 0, lv1 = 0, rv0 = 0, rv1 = 0;
        parseOne(left, lv0, lv1, lr);
        parseOne(right, rv0, rv1, lr);
        if (lr) { rect.r0 = lv0; rect.r1 = rv1; }
        else    { rect.c0 = lv0; rect.c1 = rv1; }
    }
    // Clamp
    if (rect.r0 < 0) rect.r0 = 0;
    if (rect.c0 < 0) rect.c0 = 0;
    if (rect.r1 >= rows) rect.r1 = rows - 1;
    if (rect.c1 >= columns) rect.c1 = columns - 1;
    return rect;
}

// ========== Deskmate placement helper ==========

static bool placeDeskmatePair(unsigned int *seatNumber, int rows, int columns,
                              int groupCols, unsigned int idx1,
                              unsigned int idx2) {
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < columns - 1; ++c) {
            if (c / groupCols != (c + 1) / groupCols) continue;
            if (seatNumber[r * columns + c] == 0 &&
                seatNumber[r * columns + c + 1] == 0) {
                seatNumber[r * columns + c] = idx1;
                seatNumber[r * columns + c + 1] = idx2;
                return true;
            }
        }
    }
    return false;
}

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
            seatNumber[i] = 0;
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

    if (isNot) {
        std::string centerName = stripQuotes(rule.args[0]);
        int centerIdx = findStudentOrThrow(centerName, studentGroup);
        int cx = -1, cy = -1;
        for (int r = 0; r < rows && cx < 0; ++r)
            for (int c = 0; c < columns && cx < 0; ++c)
                if (seatNumber[r * columns + c] ==
                    static_cast<unsigned int>(centerIdx)) {
                    cx = r; cy = c;
                }
        if (cx < 0) return;
        for (int dr = -radius; dr <= radius; ++dr)
            for (int dc = -radius; dc <= radius; ++dc) {
                int nr = cx + dr, nc = cy + dc;
                if (nr >= 0 && nr < rows && nc >= 0 && nc < columns &&
                    seatNumber[nr * columns + nc] == 0)
                    seatNumber[nr * columns + nc] = 255;
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
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < columns; ++c) {
                if (seatNumber[r * columns + c] != 0) continue;
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
                           int rows, int columns, int groupCols,
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

    if (!placeDeskmatePair(seatNumber, rows, columns, groupCols,
                           static_cast<unsigned int>(idx1),
                           static_cast<unsigned int>(idx2))) {
        // If already one is placed, try to place the other next to it
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < columns; ++c) {
                if (seatNumber[r * columns + c] == static_cast<unsigned int>(idx1)) {
                    if (c > 0 && seatNumber[r * columns + c - 1] == 0 &&
                        (c - 1) / groupCols == c / groupCols) {
                        seatNumber[r * columns + c - 1] = static_cast<unsigned int>(idx2);
                        return;
                    }
                    if (c + 1 < columns && seatNumber[r * columns + c + 1] == 0 &&
                        (c + 1) / groupCols == c / groupCols) {
                        seatNumber[r * columns + c + 1] = static_cast<unsigned int>(idx2);
                        return;
                    }
                }
                if (seatNumber[r * columns + c] == static_cast<unsigned int>(idx2)) {
                    if (c > 0 && seatNumber[r * columns + c - 1] == 0 &&
                        (c - 1) / groupCols == c / groupCols) {
                        seatNumber[r * columns + c - 1] = static_cast<unsigned int>(idx1);
                        return;
                    }
                    if (c + 1 < columns && seatNumber[r * columns + c + 1] == 0 &&
                        (c + 1) / groupCols == c / groupCols) {
                        seatNumber[r * columns + c + 1] = static_cast<unsigned int>(idx1);
                        return;
                    }
                }
            }
        }
    }
}

// Gender constraint table
static std::vector<char> gGenderConstraints;

static void handleSetGender(const SeatRule &rule, unsigned int *,
                            int rows, int columns,
                            std::vector<std::shared_ptr<Student>> &) {
    if (rule.args.size() < 2)
        throw functionParameterLacking("setGender needs 2+ args");
    std::string gs = rule.args[0];
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
                                   int rows, int columns, int groupCols,
                                   std::vector<std::shared_ptr<Student>> &) {
    if (rule.args.empty())
        throw functionParameterLacking("overallSituation needs args");
    if (gGenderConstraints.empty())
        gGenderConstraints.resize(rows * columns, 0);

    int numGroups = columns / groupCols;
    for (int g = 0; g < numGroups; ++g) {
        for (int gc = 0;
             gc < groupCols && gc < static_cast<int>(rule.args.size()); ++gc) {
            char gender = 0;
            const std::string &gs = rule.args[gc];
            if (gs == "\xE7\x94\xB7" || gs == "male") gender = 'M';
            else if (gs == "\xE5\xA5\xB3" || gs == "female") gender = 'F';
            if (gender == 0) continue;
            int col = g * groupCols + gc;
            for (int r = 0; r < rows; ++r)
                gGenderConstraints[r * columns + col] = gender;
        }
    }
}

// ========== condition handler (full implementation) ==========

static void handleCondition(const SeatRule &rule, unsigned int *seatNumber,
                            int rows, int columns, int groupCols,
                            std::vector<std::shared_ptr<Student>> &studentGroup,
                            int fileType) {
    if (fileType != 1 && fileType != 2) {
        std::cout << "[hint] condition requires CSV or XLSX file, skipped"
                  << std::endl;
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
            // Single student, place anywhere
            for (int i = 0; i < rows * columns; ++i) {
                if (seatNumber[i] == 0) {
                    seatNumber[i] = static_cast<unsigned int>(a);
                    break;
                }
            }
        } else {
            placeDeskmatePair(seatNumber, rows, columns, groupCols,
                              static_cast<unsigned int>(a),
                              static_cast<unsigned int>(b));
        }
    }
}

// ========== groupCondition handler (full implementation) ==========

static void
handleGroupCondition(const SeatRule &rule, unsigned int *seatNumber, int rows,
                     int columns, int groupCols,
                     std::vector<std::shared_ptr<Student>> &studentGroup,
                     int fileType) {
    if (fileType != 1 && fileType != 2) {
        std::cout
            << "[hint] groupCondition requires CSV or XLSX file, skipped"
            << std::endl;
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
    int offsetRow = rule.args.size() > 4 ? std::stoi(rule.args[4]) : 0;
    int offsetCol = rule.args.size() > 5 ? std::stoi(rule.args[5]) : 0;
    bool fillBehavior =
        rule.args.size() > 6 ? (rule.args[6] == "TRUE") : true;

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

    // Parse scope
    ScopeRect rect = parseScope(scope, rows, columns, groupCols);
    // Apply offsets (offset_row = column offset, offset_column = row offset)
    rect.c0 += offsetRow;  rect.c1 += offsetRow;
    rect.r0 += offsetCol;  rect.r1 += offsetCol;
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
            if (cell == 0 ||
                (fillBehavior && cell > static_cast<unsigned int>(reservedNum)))
                cell = static_cast<unsigned int>(reservedNum);
        }

    // Find students above/below base_point, not yet seated
    bool greaterMode = (order == "greater");
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
        bool above = (value > basePoint);
        if (greaterMode ? above : !above)
            qualifying.push_back(sv);
        else
            others.push_back(sv);
    }
    // Sort qualifying by distance from base_point (closest first)
    std::sort(qualifying.begin(), qualifying.end(),
              [](const StudentVal &a, const StudentVal &b) {
                  return a.dist < b.dist;
              });

    // Fill qualifying students into reserved positions
    size_t qi = 0;
    for (int r = rect.r0; r <= rect.r1 && qi < qualifying.size(); ++r)
        for (int c = rect.c0; c <= rect.c1 && qi < qualifying.size(); ++c) {
            if (seatNumber[r * columns + c] ==
                static_cast<unsigned int>(reservedNum)) {
                seatNumber[r * columns + c] =
                    static_cast<unsigned int>(qualifying[qi].sidx);
                ++qi;
            }
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
                if (seatNumber[r * columns + c] ==
                    static_cast<unsigned int>(reservedNum)) {
                    seatNumber[r * columns + c] =
                        static_cast<unsigned int>(remaining[ri].sidx);
                    ++ri;
                }
            }
    }

    // Clear any unreserved markers back to 0
    for (int r = rect.r0; r <= rect.r1; ++r)
        for (int c = rect.c0; c <= rect.c1; ++c)
            if (seatNumber[r * columns + c] ==
                static_cast<unsigned int>(reservedNum))
                seatNumber[r * columns + c] = 0;
}

// ========== executeRules ==========

void executeRules(const std::vector<SeatRule> &rules, unsigned int *seatNumber,
                  int rows, int columns, int groupCols,
                  std::vector<std::shared_ptr<Student>> &studentGroup,
                  const std::string &filePath, int fileType) {
    gGenderConstraints.assign(rows * columns, 0);
    gNextReserved = 254;
    gWorksheetCache = WorksheetCache{};
    gWorksheetCache.filePath = filePath;
    gWorksheetCache.fileType = fileType;

    for (const auto &rule : rules) {
        try {
            if (rule.functionName == "setSeat") {
                handleSetSeat(rule, seatNumber, rows, columns, studentGroup);
            } else if (rule.functionName == "adjacent") {
                handleAdjacent(rule, seatNumber, rows, columns, studentGroup,
                               rule.isNot);
            } else if (rule.functionName == "on") {
                handleOn(rule, seatNumber, rows, columns, studentGroup,
                         rule.isNot);
            } else if (rule.functionName == "deskmate") {
                handleDeskmate(rule, seatNumber, rows, columns, groupCols,
                               studentGroup);
            } else if (rule.functionName == "setGender") {
                handleSetGender(rule, seatNumber, rows, columns, studentGroup);
            } else if (rule.functionName == "overallSituation") {
                handleOverallSituation(rule, seatNumber, rows, columns,
                                       groupCols, studentGroup);
            } else if (rule.functionName == "condition") {
                handleCondition(rule, seatNumber, rows, columns, groupCols,
                                studentGroup, fileType);
            } else if (rule.functionName == "groupCondition") {
                handleGroupCondition(rule, seatNumber, rows, columns, groupCols,
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
}

// ========== Layer 4: Random fill ==========

void randomFill(unsigned int *seatNumber, int rows, int columns,
                const std::vector<std::shared_ptr<Student>> &studentGroup) {
    std::vector<int> emptyPositions;
    for (int i = 0; i < rows * columns; ++i)
        if (seatNumber[i] == 0) emptyPositions.push_back(i);

    int numStudents = static_cast<int>(studentGroup.size());
    std::vector<bool> placed(numStudents, false);
    for (int i = 0; i < rows * columns; ++i) {
        unsigned int v = seatNumber[i];
        if (v > 0 && v < 250 && v < static_cast<unsigned int>(numStudents))
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
                     int /*groupCols*/,
                     const std::vector<std::shared_ptr<Student>> &studentGroup) {
    // Clear any remaining reserved numbers (250-254) to empty
    for (int i = 0; i < rows * columns; ++i)
        if (seatNumber[i] >= 250 && seatNumber[i] <= 254)
            seatNumber[i] = 0;

    std::vector<int> emptyPositions;
    for (int i = 0; i < rows * columns; ++i)
        if (seatNumber[i] == 0) emptyPositions.push_back(i);

    int numStudents = static_cast<int>(studentGroup.size());
    std::vector<bool> placed(numStudents, false);
    for (int i = 0; i < rows * columns; ++i) {
        unsigned int v = seatNumber[i];
        if (v > 0 && v < 250 && v < static_cast<unsigned int>(numStudents))
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

    size_t mi = 0, fi = 0;
    for (int pos : emptyPositions) {
        int r = pos / columns, c = pos % columns;
        char constraint = 0;
        if (!gGenderConstraints.empty())
            constraint = gGenderConstraints[r * columns + c];
        if (constraint == 'M') {
            if (mi < maleStudents.size())
                seatNumber[pos] = static_cast<unsigned int>(maleStudents[mi++]);
            else if (fi < femaleStudents.size())
                seatNumber[pos] = static_cast<unsigned int>(femaleStudents[fi++]);
        } else if (constraint == 'F') {
            if (fi < femaleStudents.size())
                seatNumber[pos] = static_cast<unsigned int>(femaleStudents[fi++]);
            else if (mi < maleStudents.size())
                seatNumber[pos] = static_cast<unsigned int>(maleStudents[mi++]);
        } else {
            if (mi < maleStudents.size())
                seatNumber[pos] = static_cast<unsigned int>(maleStudents[mi++]);
            else if (fi < femaleStudents.size())
                seatNumber[pos] = static_cast<unsigned int>(femaleStudents[fi++]);
        }
    }
}

// ========== Layer 5: Print layout ==========

void printSeatLayout(
    const unsigned int *seatNumber, int rows, int columns,
    const std::vector<std::shared_ptr<Student>> &studentGroup) {
    std::cout << "\n========== Seat Layout ==========\n";
    std::cout << "(Facing blackboard, origin at top-left)\n\n";
    std::cout << "Col:\t";
    for (int c = 0; c < columns; ++c)
        std::cout << c + 1 << "\t";
    std::cout << "\n----";
    for (int c = 0; c < columns; ++c)
        std::cout << "--------";
    std::cout << "\n";
    for (int r = 0; r < rows; ++r) {
        std::cout << "Row" << r + 1 << "\t";
        for (int c = 0; c < columns; ++c) {
            unsigned int val = seatNumber[r * columns + c];
            if (val == 0) std::cout << "-";
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

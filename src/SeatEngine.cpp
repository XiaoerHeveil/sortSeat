#include "SeatEngine.h"

#include "fileInput.h"
#include "sorting.h"
#include "student.h"

#include <OpenXLSX/OpenXLSX.hpp>

#include <algorithm>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace seat {

namespace {

std::string trimCopy(std::string s) {
    auto a = std::find_if(s.begin(), s.end(),
                          [](unsigned char c) { return !std::isspace(c); });
    auto b = std::find_if(s.rbegin(), s.rend(),
                          [](unsigned char c) { return !std::isspace(c); });
    if (a == s.end())
        return "";
    return std::string(a, b.base());
}

std::vector<std::shared_ptr<Student>> loadTxt(const std::string &path) {
    std::ifstream in(std::filesystem::u8path(path));
    if (!in)
        throw std::runtime_error("无法打开学生名单文件: " + path);
    std::vector<std::shared_ptr<Student>> out;
    std::string line;
    int idx = 1;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        std::string t = trimCopy(line);
        if (t.empty())
            continue;
        std::string name = t;
        std::string sex = "男";
        size_t half = t.find(':');
        size_t full = t.find("：");
        size_t pos = std::string::npos;
        if (half != std::string::npos && full != std::string::npos)
            pos = std::min(half, full);
        else if (half != std::string::npos)
            pos = half;
        else
            pos = full;
        if (pos != std::string::npos) {
            name = trimCopy(t.substr(0, pos));
            sex = trimCopy(t.substr(pos + 1));
            if (sex.empty())
                sex = "男";
        }
        out.push_back(std::make_shared<Student>(name, sex, idx++));
    }
    return out;
}

std::vector<std::shared_ptr<Student>> loadCsv(const std::string &path) {
    std::ifstream in(std::filesystem::u8path(path));
    if (!in)
        throw std::runtime_error("无法打开学生名单文件: " + path);
    std::vector<std::shared_ptr<Student>> out;
    std::string line;
    int idx = 1;
    bool first = true;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (first) {
            first = false;
            continue; // 跳过表头
        }
        // 去掉可能的 BOM
        if (line.size() >= 3 && (unsigned char)line[0] == 0xEF &&
            (unsigned char)line[1] == 0xBB && (unsigned char)line[2] == 0xBF)
            line.erase(0, 3);

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
        if (fields.empty())
            continue;
        std::string name = trimCopy(fields[0]);
        std::string sex = fields.size() > 1 ? trimCopy(fields[1]) : "男";
        if (name.empty())
            continue;
        if (sex.empty())
            sex = "男";
        out.push_back(std::make_shared<Student>(name, sex, idx++));
    }
    return out;
}

std::vector<std::shared_ptr<Student>> loadXlsx(const std::string &path) {
    OpenXLSX::XLDocument xlsx;
    // path 已是 UTF-8，OpenXLSX 内部用 nowide::fopen 期望 UTF-8，直接传入即可
    xlsx.open(path);
    auto ws = xlsx.workbook().worksheet("Sheet1");
    std::vector<std::shared_ptr<Student>> out;
    int idx = 1;
    for (int i = 1;; ++i) {
        std::string name;
        std::string sex;
        try {
            name = getNameXLSX(ws, i);
            sex = getSexXLSX(ws, i);
        } catch (const expectationCellEmpty &) {
            break;
        } catch (const expectationCellTypeError &) {
            break;
        }
        out.push_back(std::make_shared<Student>(name, sex, idx++));
    }
    return out;
}

} // namespace

std::vector<std::shared_ptr<Student>> loadStudents(const std::string &path,
                                                   int fileType) {
    switch (fileType) {
    case 1:
        return loadCsv(path);
    case 2:
        return loadXlsx(path);
    default:
        return loadTxt(path);
    }
}

std::vector<std::string> loadRuleLines(const std::string &path) {
    std::vector<std::string> lines;
    if (path.empty())
        return lines;
    std::ifstream in(std::filesystem::u8path(path));
    if (!in)
        return lines;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        auto first = std::find_if(line.begin(), line.end(),
                                  [](unsigned char ch) { return !std::isspace(ch); });
        if (first != line.end())
            lines.push_back(line);
    }
    return lines;
}

std::string buildResultText(
    const unsigned int *seatNumber, int rows, int columns, int groupCount,
    const std::vector<std::shared_ptr<Student>> &studentGroup) {
    // 参数约定：rows = 总列数(x_row)，columns = 总行数(y_column)
    // 网格为列主序存储：seatNumber[列 * columns + 行]
    std::string out;
    for (int r = 0; r < columns; ++r) {   // r = 行
        if (r > 0)
            out += "\r\n";
        auto occupied = [&](int c) {
            if (c < 0 || c >= rows) return false;
            unsigned int v = seatNumber[c * columns + r];
            return v != EMPTY_SEAT && v != 255 &&
                   v < static_cast<unsigned int>(studentGroup.size());
        };
        for (int c = 0; c < rows; ++c) {  // c = 列
            if (c > 0) {
                bool sameGroup =
                    columnGroup(c, groupCount, rows) ==
                    columnGroup(c - 1, groupCount, rows);
                out += sameGroup ? ',' : ' ';
            }
            unsigned int val = seatNumber[c * columns + r];
            if (occupied(c)) {
                out += studentGroup[val]->getName();
            } else if (val == EMPTY_SEAT) {
                // 空座位：与同组相邻座位（同桌）相邻时，标注「（空座位）」
                bool near = false;
                if (c > 0 &&
                    columnGroup(c, groupCount, rows) ==
                        columnGroup(c - 1, groupCount, rows) &&
                    occupied(c - 1))
                    near = true;
                if (!near && c + 1 < rows &&
                    columnGroup(c, groupCount, rows) ==
                        columnGroup(c + 1, groupCount, rows) &&
                    occupied(c + 1))
                    near = true;
                if (near)
                    out += "（空座位）";
            }
        }
    }
    return out;
}

SeatResult compute(const SeatRequest &req) {
    int x_row = req.x_row;
    int groupCount = req.groupCount;
    if (x_row <= 0)
        throw std::runtime_error("座位列数必须为正整数");
    if (groupCount <= 0)
        throw std::runtime_error("小组组数必须为正整数");

    int fileType = fileExtension(req.studentPath);
    if (fileType == 3)
        fileType = 0;

    auto students = loadStudents(req.studentPath, fileType);
    if (students.empty())
        throw std::runtime_error("学生名单为空");

    int peopleNumber = static_cast<int>(students.size());
    int y_column = (peopleNumber + x_row - 1) / x_row;
    if (y_column < 1)
        y_column = 1;

    std::vector<unsigned int> grid(static_cast<size_t>(x_row) * y_column, EMPTY_SEAT);

    auto ruleLines = loadRuleLines(req.rulesPath);
    if (!ruleLines.empty()) {
        sortFunctionsByPriority(ruleLines);
        auto rules = parseRuleLines(ruleLines);
        executeRules(rules, grid.data(), x_row, y_column, groupCount, students,
                     req.studentPath, fileType);
        constrainedFill(grid.data(), x_row, y_column, groupCount, students);
    } else {
        randomFill(grid.data(), x_row, y_column, students);
    }

    SeatResult r;
    r.students = students;
    r.grid = std::move(grid);
    r.rows = x_row;
    r.columns = y_column;
    r.groupCount = groupCount;
    r.text = buildResultText(r.grid.data(), x_row, y_column, groupCount, students);
    return r;
}

std::string computeToTempFile(const SeatRequest &req) {
    SeatResult r = compute(req);
    std::filesystem::path dir = std::filesystem::temp_directory_path();
    auto ts = std::chrono::steady_clock::now().time_since_epoch().count();
    std::filesystem::path file =
        dir / ("sortSeat_result_" + std::to_string(ts) + ".txt");
    std::ofstream out(file, std::ios::binary);
    if (!out)
        throw std::runtime_error("无法写入结果文件");
    out << r.text;
    out.close();
    auto u = std::filesystem::absolute(file).u8string();
    return std::string(u.begin(), u.end());
}

} // namespace seat

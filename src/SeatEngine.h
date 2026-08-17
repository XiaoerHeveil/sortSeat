#pragma once

#include <memory>
#include <string>
#include <vector>

class Student;

// 排序计算引擎：把原 main() 中「读文件 → 解析规则 → 执行 → 填充 → 输出」抽成可复用函数
namespace seat {

struct SeatRequest {
    int x_row = 6;        // 座位列数（前端 columnsNum）
    int groupCount = 4;   // 小组组数（前端 groupNum）
    std::string studentPath; // 学生名单文件路径（txt/csv/xlsx 或 %temp% 规范化文本）
    std::string rulesPath;   // 规则文件路径（空 = 真随机）
};

struct SeatResult {
    std::vector<std::shared_ptr<Student>> students;
    std::vector<unsigned int> grid; // 扁平座位表
    int rows = 0;                   // 第一维（= x_row）
    int columns = 0;                // 第二维（= 行数）
    int groupCount = 0;
    std::string text; // 结果文本（testText 格式）
    std::string warning; // 文本文件下被关闭的单元格数据规则提示（空 = 无）
};

// 读取学生名单（fileType: 0=TXT 1=CSV 2=XLSX，未知按 TXT）
std::vector<std::shared_ptr<Student>> loadStudents(const std::string &path,
                                                   int fileType);

// 读取规则行（去除空行）；文件不存在/为空返回空 vector
std::vector<std::string> loadRuleLines(const std::string &path);

// 生成座位结果文本（逗号=同桌相邻，空格=空一格，\n=换行）
std::string buildResultText(const unsigned int *seatNumber, int rows,
                            int columns, int groupCount,
                            const std::vector<std::shared_ptr<Student>> &studentGroup);

// 计算座位布局（抛异常表示失败）
SeatResult compute(const SeatRequest &req);

// 计算并把结果文本写入 %temp% 文件，返回文件路径（UTF-8）
std::string computeToTempFile(const SeatRequest &req);

} // namespace seat

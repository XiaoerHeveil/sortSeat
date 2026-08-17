#pragma once

#include <memory>
#include <string>
#include <vector>

// 空位哨兵值（UINT_MAX）：学生索引保持 0 基，故不能用 0 表示空位（会与第一个学生冲突）
inline constexpr unsigned int EMPTY_SEAT = ~0u;

class Student;

// DSL规则结构体
struct SeatRule {
    std::string functionName;
    bool isNot;
    std::vector<std::string> args;
};

// 交换两个坐标的座位号
void exchangeSeatNumber(int *seatSheet, int columns, const int &oneRow,
                        const int &oneColumn, const int &twoRow,
                        const int &twoColumn);

// 解析DSL函数调用行的优先级
int parseFunctionPriority(const std::string &line);

// 按照优先级对DSL函数调用行进行稳定排序
void sortFunctionsByPriority(std::vector<std::string> &lines);

// 解析单条规则
SeatRule parseRuleLine(const std::string &line);

// 解析多行规则
std::vector<SeatRule>
parseRuleLines(const std::vector<std::string> &lines);

// 按姓名查找学生
int
findStudentByName(const std::string &name,
                  const std::vector<std::shared_ptr<Student>> &studentGroup);

// 总列数 totalColumns 被均分为 groupCount 组，返回第 column 列（0-based）属于第几组（0-based）
int columnGroup(int column, int groupCount, int totalColumns);

// 执行所有规则，返回文本文件下被关闭的单元格数据规则提示（空 = 无）
std::string
executeRules(const std::vector<SeatRule> &rules, unsigned int *seatNumber,
             int rows, int columns, int groupCount,
             std::vector<std::shared_ptr<Student>> &studentGroup,
             const std::string &filePath, int fileType);

// 真随机填充剩余空位
void randomFill(unsigned int *seatNumber, int rows, int columns,
                const std::vector<std::shared_ptr<Student>> &studentGroup);

// 伪随机填充剩余空位（约束感知）
void constrainedFill(unsigned int *seatNumber, int rows, int columns,
                     int groupCount,
                     const std::vector<std::shared_ptr<Student>> &studentGroup);

// 打印座位布局
void printSeatLayout(const unsigned int *seatNumber, int rows, int columns,
                     const std::vector<std::shared_ptr<Student>> &studentGroup);

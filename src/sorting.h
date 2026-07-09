#pragma once

#include <memory>
#include <string>
#include <vector>

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

// 执行所有规则
void executeRules(const std::vector<SeatRule> &rules, unsigned int *seatNumber,
                  int rows, int columns, int groupCols,
                  std::vector<std::shared_ptr<Student>> &studentGroup,
                  const std::string &filePath, int fileType);

// 真随机填充剩余空位
void randomFill(unsigned int *seatNumber, int rows, int columns,
                const std::vector<std::shared_ptr<Student>> &studentGroup);

// 伪随机填充剩余空位（约束感知）
void constrainedFill(unsigned int *seatNumber, int rows, int columns,
                     int groupCols,
                     const std::vector<std::shared_ptr<Student>> &studentGroup);

// 打印座位布局
void printSeatLayout(const unsigned int *seatNumber, int rows, int columns,
                     const std::vector<std::shared_ptr<Student>> &studentGroup);

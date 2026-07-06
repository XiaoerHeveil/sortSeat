#pragma once
#include <tuple>
#include <fstream>
#include <OpenXLSX/OpenXLSX.hpp>

using NullPathException = std::runtime_error;
using PathIllegalException = std::runtime_error;

using std::ifstream;
using std::string;

// 单元格索引位置
struct cellIndex
{
    int row = 0;
    int column = 0;
};

// 定义非法单元格地址异常
class InvalidCellAddressException : public std::exception {
        string msg;
    public :
        InvalidCellAddressException(const string &addr, const string &reason)
            : msg("Invalid cell address '" + addr + "': " + reason) {}
        const char *what() const noexcept override { return msg.c_str(); }
};

// 实际上这里可以用类进行定义与继承，但我没有弄明白这三个之间的继承关系

// 查找该行单个字符所在的位置
long searchStr(ifstream &, char, int, int);
// 查找该子字符串所在位置
long searchStr(ifstream &, string, int, int);
// 判断是否在第一行
bool isCurrentPositionFirstLine(ifstream &, int, char);

// 文件路径转义
string fileExtensionEscape(string path);
// 判断文件类型
int fileExtension(string path);

// 读取txt文件，返回姓名
string getNameTXT(ifstream &);
// 读取txt文件，返回性别
string getSexTXT(ifstream &);

// 从csv文件里将目标名称提取出来
string telepormpterCSV(string, long, long);
// 读取csv文件，返回标题
string getTitleCSV(ifstream &, const int &);
// 读取csv文件，返回标题所在列号
int getTitleCSV(ifstream &, const string &);
// 读取csv文件，返回姓名
string getNameCSV(ifstream &);
// 读取csv文件，返回性别
string getSexCSV(ifstream &);
// 读取csv文件，查找该名称在某一标题单元格下的单元格(数字需要手动转型)
string getCellCSV(ifstream &, const string &, const string &);
// 读取csv文件，查找该名称在某一标题单元格下的单元格(数字需要手动转型)，以行列进行查找(较快)
string getCellCSV(ifstream &, const int &, const int &);

// 处理单元格地址，字符转索引
cellIndex cellAddress(const string &);
// 处理单元格地址，索引转字符
string cellAddress(const cellIndex &);

// 校验单元格地址的合法性
void validateCellAddress(const string &rawAddress);
// 读取xlsx文件，返回指定单元格内容
std::tuple<OpenXLSX::XLCellValue, int> determineCellType(OpenXLSX::XLWorksheet, const string&, const string&);
std::tuple<OpenXLSX::XLCellValue, int> determineCellType(OpenXLSX::XLWorksheet, const string&, const int &, const int &);
// 读取xlsx文件，返回姓名
string getNameXLSX(OpenXLSX::XLWorksheet workSheet, const int&);
// 读取xlsx文件，返回性别
string getSexXLSX(OpenXLSX::XLWorksheet workSheet, const int&);
// 读取xlsx文件，返回标题
string getTitleXLSX(OpenXLSX::XLWorksheet workSheet, const int&);
// 读取xlsx文件，返回该名称的单元格地址
string getNameAddressXLSX(const string &);
// 读取xlsx文件，查找标题列
string getTitleColumnXLSX(const string &);
// 读取xlsx文件，查找标题列下某一名称行的单元格
string getTitleRowXLSX(const string &, const string &);
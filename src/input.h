#pragma once
#include <fstream>
#include <OpenXLSX/OpenXLSX.hpp>

using std::ifstream;
using std::string;

// 实际上这里可以用模板，但是我不会写

// 文件路径转义
string fileExtensionEscape(string path);
// 判断文件类型
int fileExtension(string path);

// 读取txt文件，返回姓名
string getNameTXT(ifstream &);
// 读取txt文件，返回性别
string getSexTXT(ifstream &);

// 读取csv文件，返回姓名
string getNameCSV(ifstream &);
// 读取csv文件，返回性别
string getSexCSV(ifstream &);
// 读取csv文件，返回标题
string getTitleCSV(ifstream &);
// 读取csv文件，查找标题
string getCellCSV(ifstream &, string title);
// 读取csv文件，查找该名称在某一标题单元格下的单元格(数字需要手动转型)
string getCellCSV(ifstream &, string title, string name);

// 读取xlsx文件，返回姓名
string getNameXLSX(const string &);
// 读取xlsx文件，返回性别
string getSexXLSX(const string &);
// 读取xlsx文件，返回标题
string getTitleXLSX(const string &);
// 读取xlsx文件，查找标题列
string getTitleColumnXLSX(const string &, string title);
// 读取xlsx文件，查找标题列下某一名称行的单元格
string getTitleRowXLSX(const string &, string title, string name);
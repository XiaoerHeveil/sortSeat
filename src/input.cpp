#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include "input.h"

using NullPathException = std::runtime_error;
using PathIllegalException = std::runtime_error;

/**
 * @brief 将Windows平台带来的路径进行转义，将反斜杠替换为正斜杠
 * 
 * @param path 文件路径
 * @return string 返回转义好的路径
 */
string fileExtensionEscape(string path) {
    // 检查是否为空
    if (path.empty()) {
        throw NullPathException("路径不能为空！");
    }
    // 检查路径是否合法
    if (path.find_first_of("\\") == std::string::npos && path.find_last_of(".") == std::string::npos) {
        throw PathIllegalException("路径不合法！");
    }
    // 检查外围是否包裹着双引号，对应复制文件路径
    if (path.find_first_of("\"") != std::string::npos && path.find_last_of("\"") != std::string::npos) {
        // 去掉两头的的双引号
        path.erase(path.length() - 1);
        path.erase(0);
    }
    return path;
}

/**
 * @brief 从末尾检查后缀名
 * .txt返回0
 * .csv返回1
 * .xlsx返回2
 * 未找到返回3
 * 
 * @param path 文件路径
 * @return int 返回状态
 */
int fileExtension(string path) {
    if (path.rfind(".txt") != std::string::npos) {
        return 0;
    } else if (path.rfind(".csv") != std::string::npos) {
        return 1;
    } else if (path.rfind(".xlsx") != std::string::npos) {
        return 2;
    } else {
        return 3;
    }
}

/**
 * @brief 从文件流中读取分割返回名称
 * 
 * @param in 被读取的文件
 * @return string 返回名称
 */
string getNameTXT(ifstream& in) {
    string line;

    // 1.按行读取
    if (std::getline(in, line)) {
        // 2.查找冒号的位置（同时匹配全角冒号与半角冒号）
        size_t pos = line.find_first_of(":：");

        if (pos != string::npos) {
            // 3.提取冒号前面的部分作为姓名
            return line.substr(0, pos);
        }
        // 如果没有提取到冒号，返回整行
        return line;
    }
    // 读取失败到达文件尾，返回空字符串
    return "";
}

string getSexTXT(ifstream &in) {
    string reamining;
    // 继续读取当前行的剩余部分（即冒号后的内容）
    if (std::getline(in, reamining)) {
        // 去除可能存在的前导空格
        auto start = std::find_if(reamining.begin(), reamining.end(), [](unsigned char ch) {
            return !std::isspace(static_cast<unsigned char>(ch));
        });
        return string(start, reamining.end());
    }
    return "";
}
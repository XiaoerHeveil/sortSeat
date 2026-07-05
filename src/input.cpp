#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include "input.h"

using NullPathException = std::runtime_error;
using PathIllegalException = std::runtime_error;

/**
 * @brief 截取一整行作为查找范围
 * 
 * @param in 读取文件流
 * @param str 要查找的子字符串
 * @param offset 与某个位置的偏移量（例如文件开头），默认0
 * @return long 返回该行（或在文件中）的位置
 */
long searchStr(ifstream &in, string str, int offset = 0) {
    // 记下读取流位置
    std::streampos curPos = in.tellg();
    // 截取一整行，并查找子字符串
    string line;
    std::getline(in, line);
    // 判断是否找到了
    if (line.find(str) == string::npos) {
        // 清除可能因读取到 EOF 而设置的 eofbit，否则后续 seekg 可能失效
        in.clear();
        in.seekg(curPos);
        return -1;
    }
    long lineNum = line.find(str);
    // 清除可能因读取到 EOF 而设置的 eofbit，否则后续 seekg 可能失效
    in.clear();
    // 读取流复位
    in.seekg(curPos);
    // 返回找到的位置与偏移量的和
    return lineNum + offset;
}

/**
 * @brief 判断文件读取流是否在第一行
 * 
 * @param in 文件读取流
 * @param curPos 当前读取流位置
 * @param chStr 分隔符（默认'\n'
 * @return true 在第一行
 * @return false 不在第一行
 */
bool isCurrentPositionFirstLine(ifstream &in, int curPos, char chStr = '\n') {
    // 特判：文件开头必然在第一行（包括空文件）
    if (curPos == 0) {
        return true;
    }
    // 回到文件开头，查到分隔符（默认'\n'）的位置
    in.seekg(0, std::ios::beg);
    std::streamoff firstNewlineOffset = -1; // 初始化为-1表示未找到
    char ch;
    std::streamoff count = 0;

    while (in.get(ch)) {
        if (ch == chStr) {
            firstNewlineOffset = count;
            break;
        }
        ++count;
    }
    // 清除可能因读取到 EOF 而设置的 eofbit，否则后续 seekg 可能失效
    in.clear();

    // 判断逻辑
    bool isFirstLine = false;
    if (firstNewlineOffset == -1) {
        // 整个文件没有换行符（即只有一行），那么任何有效位置都在第一行
        isFirstLine = true;
    } else {
        // 当前位置偏移 <= 第一个换行符偏移，即为第一行（等于时表示正好指向换行符本身）
        isFirstLine = (curPos <= firstNewlineOffset);
    }

    // 恢复原读取位置（关键步骤）
    in.seekg(curPos);

    return isFirstLine;
}

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
    // 路径转义，将"\"替换为"/"
    while (path.find_first_of("\\") != string::npos) {
        int pos = path.find_first_of("\\");
        path.replace(pos, 1, "/");
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
    // 记下当前读取流位置
    std::streamoff curPos = in.tellg();
    string line;

    // 1.按行读取
    if (std::getline(in, line)) {
        // 2.查找冒号的位置（同时匹配全角冒号与半角冒号）
        size_t pos = line.find_first_of(":：");

        if (pos != string::npos) {
            in.seekg(curPos);
            // 3.提取冒号前面的部分作为姓名
            return line.substr(0, pos);
        }
        in.seekg(curPos);
        // 如果没有提取到冒号，返回整行
        return line;
    }
    in.seekg(curPos);
    // 读取失败到达文件尾，返回空字符串
    return "";
}

/**
 * @brief 从当前行中提取出性别
 * 
 * @param in 文件读取流
 * @return string 返回性别
 */
string getSexTXT(ifstream &in) {
    // 记下当前读取流位置
    std::streamoff curPos = in.tellg();
    string reamining;
    // 继续读取当前行的剩余部分（即冒号后的内容）
    if (std::getline(in, reamining)) {
        // 去除可能存在的前导空格
        auto start = std::find_if(reamining.begin(), reamining.end(), [](unsigned char ch) {
            return !std::isspace(static_cast<unsigned char>(ch));
        });
        in.seekg(curPos);
        return string(start, reamining.end());
    }
    in.seekg(curPos);
    return "";
}

/**
 * @brief 提词把词语提取出来
 * 
 * @param line 被取词的一行
 * @param asFoPosition 截至位置
 * @param startingPosition 起始位置
 * @return string 返回词语
 */
string telepormpterCSV(string line, long asFoPosition, long startingPosition = 0) {
    // 计算要截取的长度
    long lenght = asFoPosition - startingPosition;

    return line.substr(startingPosition, lenght);
}

/**
 * @brief 从CSV文件里取词出姓名
 * 
 * @param in 文件读取流
 * @return string 返回姓名
 */
string getNameCSV(ifstream &in) {
    // 记下当前读取流的位置
    std::streampos curPos = in.tellg();
    // 判断是否是第一行（默认第一行是标题）
    if (isCurrentPositionFirstLine(in, curPos)) {
        // 查找第二行的位置（第一次'\n'后）
        long curPosTow = searchStr(in, "\n");
        // 跳转至第二行
        in.seekg(curPosTow);
    } else {
        // 重置到记忆位置
        in.seekg(curPos);
    }
    long position = searchStr(in, ",");
    string line;
    std::getline(in, line);
    
    // 复位
    in.seekg(curPos);

    return telepormpterCSV(line, curPos, position);
}
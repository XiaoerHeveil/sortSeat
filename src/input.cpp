#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include "input.h"

using std::getline;
using std::streampos;

using NullPathException = std::runtime_error;
using PathIllegalException = std::runtime_error;

/**
 * @brief 截取一整行作为查找范围，找出该字符所在位置
 * 
 * @param in 读取文件流
 * @param str 要查找的字符
 * @param offset 与某个位置的偏移量（例如文件开头），默认0
 * @param frequencyOccurrence 要查找的字符在第几次出现
 * @return long 返回该行（或在文件中）的位置
 */
long searchStr(ifstream &in, char ch = '\n', int offset = 0, int frequencyOccurrence = 1) {
    // 记下读取流位置
    std::streampos curPos = in.tellg();
    // 截取一整行，并查找子字符串
    int appearNumber = 0;
    string line;
    std::getline(in, line);
    // 判断是否找到了，如果没有找到，返回-1
    if (line.find(ch) == string::npos)
    {

        // 清除可能因读取到 EOF 而设置的 eofbit，否则后续 seekg 可能失效
        in.clear();
        in.seekg(curPos);
        return -1;
    }
    long lineNum = 0;
    for (int i = 0; i < line.length(); ++i) {
        // 判断出现的次数
        if (appearNumber >= frequencyOccurrence) {
            break;
        } else if (line.at(i) == ch) {
            lineNum = i;
            appearNumber += 1;
        }
    }
    // 清除可能因读取到 EOF 而设置的 eofbit，否则后续 seekg 可能失效
    in.clear();
    // 读取流复位
    in.seekg(curPos);
    // 返回找到的位置与偏移量的和
    return lineNum + offset;
}

/**
 * @brief 截取一整行作为查找范围，找出该子字符串所在位置
 *
 * @param in 读取文件流
 * @param str 要查找的字符串
 * @param offset 与某位置的偏移量
 * @param frequencyOccurrence 要查找的字符串在第几次出现
 * @return long 返回该行（或在文件中）的位置
 */
long searchStr(ifstream &in, string str, int offset = 0, int frequencyOccurrence = 1)
{
    // 记下读取流位置
    std::streampos curPos = in.tellg();
    string line;
    std::getline(in, line);
    // 判断是否找到了，如果没有找到，返回-1
    if (line.find(str) == string::npos)
    {

        // 清除可能因读取到 EOF 而设置的 eofbit，否则后续 seekg 可能失效
        in.clear();
        in.seekg(curPos);
        return -1;
    }
    long lineNum = 0;
    for (int i = 0; i < frequencyOccurrence; ++i) {
        lineNum = line.find(str, lineNum);
        if (lineNum == string::npos) {
            in.clear();
            in.seekg(curPos);
            return INT_MAX; // 没找到第 n 次，直接返回 INT_MAX
        }
        lineNum += str.length(); // 移动到匹配位置之后，准备找下一次
    }
    // 清除可能因读取到 EOF 而设置的 eofbit，否则后续 seekg 可能失效
    in.clear();
    // 读取流复位
    in.seekg(curPos);
    // 循环结束后，pos 实际上指向了第 n 次匹配位置的“下一个字符”
    // 所以真正的起始位置需要减去子串的长度
    return lineNum - str.length() + offset;
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

    // 恢复原读取位置
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
    streampos curPos = in.tellg();
    string reamining;
    // 继续读取当前行的剩余部分（即冒号后的内容）
    if (getline(in, reamining)) {
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
    streampos curPos = in.tellg();
    
    long position = searchStr(in, ',');
    string line;
    getline(in, line);
    
    // 复位
    in.seekg(curPos);

    return telepormpterCSV(line, curPos, position);
}

/**
 * @brief 从CSV文件里取词出性别
 * 
 * @param in 文件读取流
 * @param name 同学名字
 * @return string 返回那位同学的性别
 */
string getSexCSV(ifstream &in) {
    // 记下当前读取流位置
    streampos curPos = in.tellg();
    if (searchStr(in, ',', curPos) == -1) {
        return "";
    }
    string line;
    long asFoPosition;
    getline(in, line);
    asFoPosition = searchStr(in, ',', curPos, 2);
    // 复位
    in.seekg(curPos);
    // 返回性别
    return telepormpterCSV(line, asFoPosition, curPos);
}

/**
 * @brief 从CSV文件里的第一行中找出所需要的标题
 * 
 * @param in 文件读取流
 * @param titleNumber 第几个标题
 * @return string 返回标题
 */
string getTitleCSV(ifstream &in, int titleNumber = 0) {
    // 记下当前读取流位置
    streampos curPos = in.tellg();
    long comma = 0;
    long commaOffset;
    if (!isCurrentPositionFirstLine(in, curPos)) {
        in.seekg(0);
        comma = searchStr(in, ',', 0, titleNumber + 2);
        in.seekg(comma);
    }
    commaOffset = searchStr(in, ',', 0, titleNumber + 3);
    string line;
    getline(in, line);
    // 判断commaOffset是否等于-1
    if (commaOffset == -1) {
        in.seekg(curPos);
        return telepormpterCSV(line, line.length() - 1, comma);
    }
    return telepormpterCSV(line, commaOffset, comma);
}

/**
 * @brief 从CSV文件里的第一行找出标题所在位置
 * 
 * @param in 文件读取流
 * @param title 标题名称
 * @return int 标题列号
 */
int getTitleCSV(ifstream &in, string title) {
    streampos curPos = in.tellg();
    if (!isCurrentPositionFirstLine(in, curPos)) {
        in.seekg(0);
    }
    string line;
    getline(in, line);
    return line.find(title) - title.length();
}

/**
 * @brief 根据标题与姓名，查找所交叉的单元格所在的内容
 * 
 * @param in 文件读取流
 * @param title 标题名称
 * @param name 姓名
 * @return string 返回标题列和行所交叉的单元格所在的内容
 */
string getCellCSV(ifstream &in, string title, string name) {
    // 记下位置
    streampos curPos = in.tellg();
    // 跳转至第二行
    in.seekg(0);
    long oneLineBreak = searchStr(in);
    in.seekg(oneLineBreak);
    // 找出标题所在的列号
    int titleColumnNumber = getTitleCSV(in, title);
    long nameLineNumber;
    // 找出名字所在的行号
    for (int i = 0; in.peek() != EOF; i++) {
        // 一般来说，一个班里名字基本是不重复的，所以用不到INT_MAX
        if (searchStr(in, name) != -1) {
            nameLineNumber = searchStr(in, name);
            break;
        } else if (searchStr(in, name) == -1) {
            // 换行
            int a = searchStr(in);
            in.seekg(a);
        }
    }
    if (in.peek() == EOF) {
        in.seekg(curPos);
        return "名称不存在！";
    }
    in.seekg(searchStr(in, '\n', 0, nameLineNumber - 1));
    string line;
    getline(in, line);
    // 复位
    in.seekg(curPos);
    return telepormpterCSV(line, searchStr(in, ',', 0, titleColumnNumber), searchStr(in, ',', 0, titleColumnNumber - 1));
}

/**
 * @brief 接受标题列与姓名行，将词语切出来
 * 
 * @param in 文件读取流
 * @param title 标题列
 * @param name 姓名行
 * @return string 返回切出的词语
 */
string getCellCSV(ifstream &in, int title, int name) {
    // 记忆
    streampos curPos = in.tellg();
    // 跳转至指定行
    long lineBreak = searchStr(in, '\n', 0, name - 1);
    in.seekg(lineBreak);
    // 切词
    string line;
    getline(in, line);
    // 复位
    return telepormpterCSV(line, title, title - 1);
}
#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include "input.h"

using std::getline;
using std::streampos;

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
int getTitleCSV(ifstream &in, const string &title) {
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
string getCellCSV(ifstream &in, const string &title, const string &name) {
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
    in.seekg(curPos);
    return telepormpterCSV(line, title, title - 1);
}

/**
 * @brief 解析并校验单元格地址
 *
 * @param rawAddress 单元格地址字符串
 */
void validateCellAddress(const string &rawAddress)
{
    // 1. 预处理：去除首尾空格，统一转为大写
    string addr = rawAddress;
    addr.erase(0, addr.find_first_not_of(" \t\n\r\f"));
    addr.erase(addr.find_last_not_of(" \t\n\r\f") + 1);
    std::transform(addr.begin(), addr.end(), addr.begin(), ::toupper);

    // 地址为空
    if (addr.empty())
        throw InvalidCellAddressException(rawAddress, "Address is empty");

    // 2. 分离列字母和行数字
    size_t splitPos = 0;
    while (splitPos < addr.size() && std::isalpha(addr[splitPos]))
        ++splitPos;

    // 必须有列字母，且后续必须全是数字
    if (splitPos == 0 || splitPos == addr.size())
    {
        // 必须由字母和数字组成
        throw InvalidCellAddressException(rawAddress, "Must consist of letters followed by digits");
    }
    for (size_t i = splitPos; i < addr.size(); ++i)
    {
        if (!std::isdigit(addr[i]))
        {
            // 包含无效字符
            throw InvalidCellAddressException(rawAddress, "Contains invalid characters");
        }
    }

    string colStr = addr.substr(0, splitPos);
    string rowStr = addr.substr(splitPos);

    // 3. 校验行号 (禁止前导零，且 <= 1048576)
    if (rowStr.size() > 1 && rowStr[0] == '0')
    {
        // 行号不能有前导零
        throw InvalidCellAddressException(rawAddress, "Row number cannot have leading zeros");
    }
    unsigned long long row = std::stoull(rowStr);
    if (row == 0 || row > 1048576ULL)
    {
        // 行超出范围（1-1048576）
        throw InvalidCellAddressException(rawAddress, "Row out of range (1-1048576)");
    }

    // 4. 校验列名 (26进制解码，必须 <= 16384)
    unsigned long colNum = 0;
    for (char c : colStr)
    {
        colNum = colNum * 26 + (c - 'A' + 1);
        // 提前退出：如果超过最大列数，直接报错
        if (colNum > 16384)
        {
            // 列超出范围（A-XFD）
            throw InvalidCellAddressException(rawAddress, "Column out of range (A-XFD)");
        }
    }
    if (colNum == 0)
    {
        // 列格式无效
        throw InvalidCellAddressException(rawAddress, "Invalid column format");
    }
}

/**
 * @brief 将单元格地址字符串转换为行列索引
 *
 * @param cellNumber 单元格地址字符串（如 "A1"）
 * @return cellIndex 返回行列索引结构体
 */
cellIndex cellAddress(const string& cellNumber) {
    validateCellAddress(cellNumber);
    // 分离字母和数字
    string addr;
    size_t splitPos = 0;
    while (splitPos < addr.size() && std::isalpha(addr[splitPos]))
        ++splitPos;

    string colStr = addr.substr(0, splitPos);
    string rowStr = addr.substr(splitPos);

    // 获取行号
    int row = std::stoull(rowStr);
    // 26进制解码
    int colNum = 0;
    for (char c : colStr) {
        colNum = colNum * 26 + (c - 'A' + 1);
    }

    // 返回行号和列号
    cellIndex cellAddr{row, colNum};
    return cellAddr;
}

/**
 * @brief 将行列索引转换为单元格地址字符串
 *
 * @param cell 行列索引结构体
 * @return string 返回单元格地址字符串（如 "A1"）
 */
string cellAddress(const cellIndex& cell) {
    // 校验行列范围
    if (cell.row < 1 || cell.row > 1048576) {
        throw InvalidCellAddressException(cell.row, "Row number out of range (1-1048576)");
    }
    if (cell.column < 1 || cell.column > 16384) {
        throw InvalidCellAddressException(cell.column, "Column number out of range (1-16384)");
    }

    // 将列号转换为字母
    int colNum = cell.column;
    string colStr;
    while (colNum > 0) {
        colNum--; // 调整为0索引
        char letter = 'A' + (colNum % 26);
        colStr = letter + colStr;
        colNum /= 26;
    }

    // 返回组合的单元格地址
    return colStr + std::to_string(cell.row);
}

/**
 * @brief 判断单元格的类型，并返回对应的XLCellValue和类型码
 * 
 * @param workSheet 工作表
 * @param cellPosition 类型码
 * @return std::tuple<OpenXLSX::XLCellValue, int> 
 */
std::tuple<OpenXLSX::XLCellValue, int> determineCellType(
    OpenXLSX::XLWorksheet workSheet, const string &cellPosition) {
        validateCellAddress(cellPosition);
        // 获取单元格对象
        auto cell = workSheet.cell(cellPosition);
        OpenXLSX::XLCellValue cellValue = cell.value();

        // 首先判断是否为空
        if (cellValue.type() == OpenXLSX::XLValueType::Empty) {
            return std::make_tuple(cellValue, 0);
        } else if (cellValue.type() == OpenXLSX::XLValueType::Boolean) {
            return std::make_tuple(cellValue, 1);
        } else if (cellValue.type() == OpenXLSX::XLValueType::Integer) {
            return std::make_tuple(cellValue, 2);
        } else if (cellValue.type() == OpenXLSX::XLValueType::Float) {
            return std::make_tuple(cellValue, 3);
        } else if (cellValue.type() == OpenXLSX::XLValueType::Error) {
            return std::make_tuple(cellValue, 4);
        } else if (cellValue.type() == OpenXLSX::XLValueType::String) {
            // 如果字符串本身是空的，也视为空
            if (cellValue.get<std::string>().empty()) {
                return std::make_tuple(cellValue, 0);
            }
            return std::make_tuple(cellValue, 5);
        } else {
            // 如果遇到未知类型，返回一个默认的 XLCellValue 和类型码 -1
            return std::make_tuple(OpenXLSX::XLCellValue(), -1);
        }
    }

/**
 * @brief 判断单元格的类型，并返回对应的XLCellValue和类型码
 * 
 * @param workSheet 工作表
 * @param rowNumber 行号
 * @param columnNumber 列号
 * @return std::tuple<OpenXLSX::XLCellValue, int> XLCellValue和类型码
 */
std::tuple<OpenXLSX::XLCellValue, int> determineCellType(
    OpenXLSX::XLWorksheet workSheet, const int &rowNumber, const int &columnNumber) {
        // 校验行号与列号的合法性
        if (rowNumber < 1 || rowNumber > 1048576) {
            throw InvalidCellAddressException(rowNumber, "Row number out of range (1-1048576)");
        }
        if (columnNumber < 1 || columnNumber > 16384) {
            throw InvalidCellAddressException(columnNumber, "Column number out of range (1-16384)");
        }
        // 获取指定位置单元格对象
        auto cell = workSheet.cell(rowNumber, columnNumber);
        OpenXLSX::XLCellValue cellValue = cell.value();
        // 根据单元格类型返回相应的值和类型码
        if (cellValue.type() == OpenXLSX::XLValueType::Empty) {
            return std::make_tuple(cellValue, 0);
        } else if (cellValue.type() == OpenXLSX::XLValueType::Boolean) {
            return std::make_tuple(cellValue, 1);
        } else if (cellValue.type() == OpenXLSX::XLValueType::Integer) {
            return std::make_tuple(cellValue, 2);
        } else if (cellValue.type() == OpenXLSX::XLValueType::Float) {
            return std::make_tuple(cellValue, 3);
        } else if (cellValue.type() == OpenXLSX::XLValueType::Error) {
            return std::make_tuple(cellValue, 4);
        } else if (cellValue.type() == OpenXLSX::XLValueType::String) {
            // 如果字符串本身是空的，也视为空
            if (cellValue.get<std::string>().empty()) {
                return std::make_tuple(cellValue, 0);
            }
            return std::make_tuple(cellValue, 5);
        } else {
            // 如果遇到未知类型，返回一个默认的 XLCellValue 和类型码 -1
            return std::make_tuple(OpenXLSX::XLCellValue(), -1);
        }
    }

/**
 * @brief 获取工作表中指定位置的姓名
 * @param workSheet 工作表
 * @param whileNumber 第几次姓名
 * @return std::string 姓名
 */
string getNameXLSX(OpenXLSX::XLWorksheet workSheet, const int &whileNumber = 1) {
    // 从A2(2,1)开始读取姓名
    string cellPosition = "A" + std::to_string(whileNumber + 1);
    auto [cellValue, typeCode] = determineCellType(workSheet, cellPosition);
    return cellValue.get<std::string>();
}

/**
 * @brief 获取工作表中指定位置的性别
 * 
 * @param workSheet 工作表
 * @param whileNumber 第几次性别
 * @return string 性别
 */
string getSexXLSX(OpenXLSX::XLWorksheet workSheet, const int &whileNumber = 1) {
    // 从B2(2,2)开始读取性别
    string cellPosition = "B" + std::to_string(whileNumber + 1);
    auto [cellValue, typeCode] = determineCellType(workSheet, cellPosition);
    return cellValue.get<std::string>();
}

/**
 * @brief 获取工作表中指定列位置的标题
 * 
 * @param workSheet 工作表
 * @param whileNumber 第几次标题
 * @return string 标题名
 */
string getTitleXLSX(OpenXLSX::XLWorksheet workSheet, const int &whileNumber = 1) {
    // 从A3(1,3)这一行开始读取标题
    cellIndex titleCell{whileNumber + 2, 1}; // 行号为 whileNumber + 2，列号为 1（即A列）
    string cellPosition = cellAddress(titleCell);
    auto [cellValue, typeCode] = determineCellType(workSheet, cellPosition);
    return cellValue.get<std::string>();
}

/**
 * @brief 获取工作表中指定姓名所在的行号
 * 
 * @param workSheet 工作表
 * @param name 姓名
 * @return int 行号
 */
int getNameRowXLSX(OpenXLSX::XLWorksheet workSheet, const string &name) {
    // 从A2开始查找姓名
    int row = 2; // 从第2行开始
    while (true) {
        cellIndex nameCell{row, 1}; // A列
        string cellPosition = cellAddress(nameCell);
        auto [cellValue, typeCode] = determineCellType(workSheet, cellPosition);
        if (typeCode == 0) { // 空单元格，说明没有更多姓名
            break;
        }
        if (cellValue.get<std::string>() == name) {
            return row; // 找到姓名，返回行号
        }
        ++row;
    }
    return -1; // 未找到姓名
}

/**
 * @brief 获取工作表中指定标题所在的列号
 * 
 * @param workSheet 工作表
 * @param title 标题
 * @return int 列号
 */
int getTitleColumnXLSX(OpenXLSX::XLWorksheet workSheet, const string &title) {
    // 从C1开始查找标题
    int column = 3; // 从第3列开始
    while (true) {
        cellIndex titleCell{1, column}; // 第1行
        string cellPosition = cellAddress(titleCell);
        auto [cellValue, typeCode] = determineCellType(workSheet, cellPosition);
        if (typeCode == 0) { // 空单元格，说明没有更多标题
            break;
        }
        if (cellValue.get<std::string>() == title) {
            return column; // 找到标题，返回列号
        }
        ++column;
    }
    return -1; // 未找到标题
}
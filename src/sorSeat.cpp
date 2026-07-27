#pragma MSVC diagnostic ignored "-Wdeprecated-declarations"
#define PLATFORM_WINDOWS 1
#define PLATFORM_OTHER 0

#if defined(_WIN32)
	// Windows平台，因为该平台下的换行是/r/n是两个字节，但是在读取的时候只有一个字节，非常坑
	#define TEXT_OFFSET PLATFORM_WINDOWS
#else
	// 其他平台的文件读取在只读模式下不偏移
	#define TEXT_OFFSET PLATFORM_OTHER
#endif

#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <filesystem>
#include <fstream>
#include "student.h"
#include "fileInput.h"
#include "sorting.h"

int main(void) {
	using std::cerr;
	using std::cin;
	using std::cout;
	using std::endl;

	// 创建一个容器，用于存储多个Student对象
	const int peopleNumber = 42;	// 这个人数仅供测试，实际人数不一定为42
	string workSheet = "Sheet1";
	std::vector<std::shared_ptr<Student>> studentGroup;
	while (true) {
		cout << "请输入你要读取文件的路径：\n";
		std::string path;
		cin >> path;
		int fileType = -1; // 文件类型，用于后续condition/groupCondition判断

		// 判断文件类型
		try {
			path = fileExtensionEscape(path);
		} catch (NullPathException) {
			cout << "路径为空！" << endl;
		} catch (PathIllegalException) {
			cout << "非法路径！" << endl;
		}

		try {
			fileType = fileExtension(path);
			switch (fileType) {
				case 0: {
					// 顺着路径打开文件
					ifstream inFile;
					// 遇到权限问题直接抛异常
					inFile.exceptions(ifstream::failbit | ifstream::badbit);
					inFile.open(path);
					// 在外部控制流位置
					for (int i = 1;; ++i) {
						string name = getNameTXT(inFile);
						string sex = getSexTXT(inFile);
						// 查询下一次换行的位置
						long lineBreakPosition = searchStr(inFile, '\n', 0, i);
						if (lineBreakPosition == -1) {
							cout << "找完了" << endl;
							break;
						}
						if (name.empty() && sex.empty()) {
							cout << "找完了" << endl;
							break;
						}
						// 将姓名和性别以及座位号添加进Student类中
						studentGroup.push_back(std::make_shared<Student>(name, sex, i));
						// 将流跳转到下一次换行符后
						inFile.seekg(lineBreakPosition + TEXT_OFFSET);
					}
					// 判断容器是否为空
					if (studentGroup.size() == 0) {
						throw NullFile("TXT文件为空！");
					}

					break;
				}
				case 1: {
					ifstream inFile;
					inFile.exceptions(ifstream::failbit | ifstream::badbit);
					inFile.open(path);
					for (int i = 1;; ++i) {
						string name = getNameCSV(inFile);
						string sex = getSexCSV(inFile);
						long lineBreakPosition = searchStr(inFile, '\n', 0, i);
						if (lineBreakPosition == -1) {
							cout << "找完了" << endl;
							break;
						}
						if (name.empty() && sex.empty()) {
							cout << "找完了" << endl;
							break;
						}
						studentGroup.push_back(std::make_shared<Student>(name, sex, i));
						inFile.seekg(lineBreakPosition + TEXT_OFFSET);
					}
					if (studentGroup.size() == 0) {
						throw NullFile("CSV文件为空！");
					}

					break;
				}
				case 2: {
					// 打开XLSX文件
					OpenXLSX::XLDocument xlsx;
					xlsx.open(path);
					
					auto wb = xlsx.workbook();
					OpenXLSX::XLWorksheet ws;
					try {
						ws = wb.worksheet(workSheet);
					} catch (const OpenXLSX::XLSheetError &e) {
						cout << "这可能是您更改了工作表名导致没有找到您期望的工作表，请输入此工作表名：" << endl;
						cin >> workSheet;
						// 重新读取
						ws = wb.worksheet(workSheet);
					}
					// OpenXLSX 这里不需要手动触发公式计算，直接读取工作表内容即可
					// 在每一行读取
					try {
						for (int i = 1;; ++i) {
							string name = getNameXLSX(ws, i);
							string sex = getSexXLSX(ws, i);
							studentGroup.push_back(std::make_shared<Student>(name, sex, i));
						}
					} catch (expectationCellEmpty) {
						if (studentGroup.size() == 0)
							throw NullFile("XLSX文件为空！");
					}

						break;
				}
				default :
					cout << "不支持的文件！" << endl;
			}
		} catch (const std::ios_base::failure& e) {
			cerr << "访问被拒！" << endl;
			cerr << "错误：" << e.what() << endl;
		} catch (NullFile) {
			cerr << "空文件！" << endl;
		} catch (const OpenXLSX::XLException &e) {
			cerr << "访问被拒！" << e.what() << endl;
		} catch (expectationCellTypeError) {
			cerr << "期望的单元格类型不对！这可能是您没有按照格式来" << endl;
		}
		cout << "请输入你想分配的座位列：\n";
		int x_row;
		cin >> x_row;
		cout << "每组有几列？\n";
		int groupRow;
		cin >> groupRow;

		int y_column = peopleNumber / x_row + 1;
		// 使用 vector 动态创建二维数组，所有元素初始化为 0
		std::vector<std::vector<unsigned int>> seatNumber(x_row, std::vector<unsigned int>(y_column, 0));

		// 编号会被保存在这个数组中，所有排序都是基于编号进行映射（动号不动人）
		// 因为需要修改顺序，因此拷贝内置整形比拷贝整个对象的开销要小很多
		// 保留数字：0->空座位（或者这里还没有排座）；255->无法分配；254,253,252,251,250->条件排序（目前最多5个条件，后续增添）

		// 读取DSL规则文件并排序
		std::string dslPath = "test/ABCD.txt";
		std::ifstream dslFile(dslPath);
		std::vector<std::string> ruleLines;
		if (dslFile.is_open()) {
			std::string ruleLine;
			while (std::getline(dslFile, ruleLine)) {
				auto first = std::find_if(ruleLine.begin(), ruleLine.end(),
				    [](unsigned char ch) { return !std::isspace(ch); });
				if (first != ruleLine.end())
					ruleLines.push_back(ruleLine);
			}
			dslFile.close();

			sortFunctionsByPriority(ruleLines);

			// 写入临时文件
			std::filesystem::path tempDir =
			    std::filesystem::temp_directory_path();
			std::filesystem::path tempFile =
			    tempDir / "seat_rules_sorted.txt";
			std::ofstream outFile(tempFile);
			if (outFile.is_open()) {
				for (const auto &r : ruleLines)
					outFile << r << '\n';
				outFile.close();
				cout << "已排序的规则已写入: " << tempFile.string()
				     << endl;
			}
		} else {
			cout << "未找到DSL规则文件: " << dslPath
			     << "，使用真随机模式" << endl;
		}

		// 解析并执行规则
		if (!ruleLines.empty()) {
			auto rules = parseRuleLines(ruleLines);
			executeRules(rules, &seatNumber[0][0], x_row, y_column,
			             groupRow, studentGroup, path, fileType);
			cout << "已执行 " << rules.size() << " 条规则" << endl;
			constrainedFill(&seatNumber[0][0], x_row, y_column,
			                groupRow, studentGroup);
		} else {
			randomFill(&seatNumber[0][0], x_row, y_column,
			           studentGroup);
		}

		// 打印座位布局
		printSeatLayout(&seatNumber[0][0], x_row, y_column,
		                studentGroup);

		break;
	}
	
	return 0;
}
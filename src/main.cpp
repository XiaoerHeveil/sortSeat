#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include "student.h"
#include "input.h"

int main(void) {
	using std::cout;
	using std::cin;
	using std::endl;
	
	const int peopleNumber = 42;
	cout << "请输入你要读取文件的路径：\n";
	std::string path;
	cin >> path;

	// 创建一个容器，用于存储多个Student对象
	std::vector<std::shared_ptr<Student>> studentGroup;
	// 判断文件类型
	try {
		path = fileExtensionEscape(path);
	} catch (NullPathException) {
		cout << "路径为空！" << endl;
	} catch (PathIllegalException) {
		cout << "非法路径！" << endl;
	}
	switch (fileExtension(path)) {
		case 0: {
			
			break;
		}
		case 1: {
			break;
		}
		case 2: {
			break;
		}
		default :
	}

	/* while (true) {
		cout << "请输入你想分配的座位列：\n";
		int x_number;
		cin >> x_number;
		int y_number = peopleNumber / x_number + 1;
		unsigned int seatNumber[x_number][y_number] = {0};
		// 编号会被保存在这个数组中，所有排序都是基于编号进行映射（动号不动人）
		// 保留数字：0->空座位；255->无法分配；254,253,252,251,250->条件排序（目前最多5个条件，后续增添）
		printInformation(*student);
	} */
	
	return 0;
}
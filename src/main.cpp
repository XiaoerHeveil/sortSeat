#include <iostream>
#include <string>
#include <memory>
#include "student.h"

int main(void) {
	using std::cout;
	using std::cin;
	using std::endl;
	
	const int peopleNumber = 42;
	cout << "请输入你要读取文件的路径：\n";
	std::string path;
	cin >> path;

	while (true) {
	cout << "输入一个名字(按Q退出)：";
	std::string name;
	cin >> name;
	cout << "输入性别：";
	std::string sex;
	cin >> sex;
	cout << "请输入你想分配的座位列：\n";
	int x_number;
	cin >> x_number;
	int y_number = peopleNumber/x_number + 1;
	unsigned int seatNumber[x_number][y_number] = {0};
	// 编号会被保存在这个数组中，所有排序都是基于编号进行映射（动号不动人）
	// 保留数字：0->空座位；255->无法分配；254,253,252,251,250->条件排序（目前最多5个条件，后续增添）

	auto student = std::make_shared<Student>(name, sex, 1);
	printInformation(*student);
	}
	
	return 0;
}
/* and &&
 * or ||
 * not !
 */
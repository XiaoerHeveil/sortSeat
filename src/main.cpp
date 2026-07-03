#include <iostream>
#include <string>
#include <memory>
#include "definition.h"

int main(void) {
	using std::cout;
	using std::cin;
	using std::endl;
	
	const int peopleNumber = 42;
	
	whele (true) {
	cout << "输入一个名字(按Q退出)：";
	cin >> std::string name;
	if (name == 'q' || name == 'Q') {
		break;
	}
	cout << "\n输入ta所在的班级：";
	cin >> std::string classes;
	cout << "\n输入ta的年龄：";
	cin >> int age;
	cout << "\n身高：";
	cin >> double height;
	cout << "\n体重：";
	cin >> double weight;
	
	cout << "请输入你想分配的座位列：\n";
	cin >> byte y_number;
	x_number = peopleNumber/y_number + 1;
	unsigned byte seatNumber[x_number][y_number] {0};
	// 编号会被保存在这个数组中，所有排序都是基于编号进行映射（动号不动人）
	// 保留数字：0->空座位；255->无法分配；254,253,252,251,250->条件排序（目前最多5个条件，后续增添）
	
	// Student zhangSan = new Student(name, classes, age, height, weight, 1);
	std::shared_ptr<Student> student(new Student(name, classes, age, height, weight, 1))
	printInformation(student);
	}
	
	return 0;
}
/* and &&
 * or ||
 * not !
 */
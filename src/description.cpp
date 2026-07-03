#include <iostream>
#include <string>
#include "definition.h"

std::string Student::getName() {
	return name;
}

int Student::getAge() {
	return age;
}

double Student::getHeight() {
	return height;
}

double Student::getWeight() {
	return weight;
}

int Student::getSeatNumber() {
	return seatNumber;
}

// 打印Student类的成员信息
void printInformation(const Student &stu) {
	std::cout << "你的信息如下：\n"
		 << "姓名：" + stu.getName() << '\n'
		 << "年龄：" + s.getAge() << '\n'
		 << "身高：" + s.getHeight() << '\n'
		 << "体重：" + s.getWeight() << '\n'
		 << "座位号：" + s.getSeatNumber() << std::endl;
}
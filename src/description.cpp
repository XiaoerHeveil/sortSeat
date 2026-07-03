#include <iostream>
#include <string>
#include "definition.h"

std::string Student::getName() {
	return name;
}

std::string Student::getClasses() {
	return classes;
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
void printInformation(const Student &s) {
	std::cout << "你的信息如下：\n"
		 << "姓名：" + s.getName() << '\n'
		 << "班级：" + s.getClasses() << '\n'
		 << "年龄：" + s.getAge() << '\n'
		 << "身高：" + s.getHeight() << '\n'
		 << "体重：" + s.getWeight() << '\n'
		 << "座位号：" + s.getSeatNumber() << std::endl;
}
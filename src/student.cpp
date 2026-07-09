#include <iostream>
#include <string>
#include "student.h"

Student::Student() {
	name = "";
	sex = "男";
	seatNumber = 1;
}

Student::Student(std::string name, std::string sex, int seatNumber)
{
	this->name = name;
	this->sex = sex;
	this->seatNumber = seatNumber;
}

std::string Student::getName() const {
	return name;
}

std::string Student::getSex() const {
	return sex;
}

int Student::getSeatNumber() const {
	return seatNumber;
}

void Student::setSeatNumber(int a) {
	if (a >= 0 && a < 256) {
		seatNumber = a;
	} else if (a < 0) {
		std::cout << "您输入的数字太小了！\n";
	} else {
		std::cout << "您输入的数字太大了！\n";
	}
}

Student::~Student() {}
void printInformation(const Student &s) {
	std::cout << "你的信息如下：\n"
		 << "姓名：" << s.getName() << '\n'
		 << "年龄：" << s.getSex() << '\n'
		 << "座位号：" << s.getSeatNumber() << std::endl;
}
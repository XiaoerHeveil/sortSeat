#pragma once
#include <iostream>
// 学生信息表
class Student {
	private:
		std::string name = "";
		int age = 0;
		double height = 0.0;
		double weight = 0.0;
		unsigned int seatNumber = 0;
	public:
		Student();	// （无参）构造器
		// Student(name, age, height = 0.0, weight = 0.0, SeatNumber = 1);	// （有参）构造器
		~Student();	// 析构函数
		std::string getName();
		int getAge();
		double getHeight();
		double getWeight();
		int getSeatNumber();
};

void printInformation(const Student&);
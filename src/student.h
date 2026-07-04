#pragma once
#include <iostream>
// 学生信息表
class Student {
	private:
		std::string name = "";
		std::string sex = "男";
		unsigned int seatNumber = 0;
	public:
		Student();	// （无参）构造器
		Student(std::string name, std::string sex, int SeatNumber = 1);	// （有参）构造器
		~Student();	// 析构函数
		std::string getName() const;
		std::string getSex() const;
		int getSeatNumber() const;
		void setSeatNumber(int);
};

void printInformation(const Student &);
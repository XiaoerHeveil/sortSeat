#include <iostream>
#include <OpenXLSX.hpp>

int main()
{
    // 创建一个名为 "example.xlsx" 的 Excel 文件
    OpenXLSX::XLDocument doc;
    doc.create("\\Excel\\AAA.xlsx", OpenXLSX::XLForceOverwrite);

    // 获取默认工作表 "Sheet1" 并向单元格写入数据
    auto wks = doc.workbook().worksheet("Sheet1");
    wks.cell("A1").value() = "Hello, OpenXLSX!";
    wks.cell("B1").value() = 42;

    // 保存并关闭文件
    doc.save();
    doc.close();

    std::cout << "Excel文件创建成功！" << std::endl;
    return 0;
}
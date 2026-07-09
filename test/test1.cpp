#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <OpenXLSX/OpenXLSX.hpp>
#include <iostream>

int main()
{
    try
    {
        OpenXLSX::XLDocument doc;
        doc.open("D:/Temp/Excel/AAA.xlsx");
        auto wks = doc.workbook().worksheet("Sheet1");
        wks.cell("A1").value() = "Hello, OpenXLSX!";
        wks.cell("B1").value() = 40;

        auto cell = wks.cell("G1");
        const auto &value = cell.value();

        if (value.type() == OpenXLSX::XLValueType::String) {
            std::cout << value.get<std::string>() << std::endl;
        } else if (value.type() == OpenXLSX::XLValueType::Integer) {
            std::cout << value.get<int>() << std::endl;
        } else if (value.type() == OpenXLSX::XLValueType::Float) {
            std::cout << value.get<double>() << std::endl;
        } else {
            std::cout << "Cell G1 is empty or has a different type." << std::endl;
        }

        doc.save();
        doc.close();
        std::cout << "File created successfully." << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
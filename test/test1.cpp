#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <OpenXLSX/OpenXLSX.hpp>
#include <filesystem>
namespace fs = std::filesystem;

int main()
{
    // fs::create_directories("./Excel"); // 确保目录存在

    try
    {
        OpenXLSX::XLDocument doc;
        doc.open("D:/Temp/Excel/AAA.xlsx");
        auto wks = doc.workbook().worksheet("Sheet1");
        wks.cell("A1").value() = "Hello, OpenXLSX!";
        wks.cell("B1").value() = 40;
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
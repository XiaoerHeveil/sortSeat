set(CMAKE_TOOLCHAIN_FILE "D:/Code/Cpp/project/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

cmake_minimum_required(VERSION 3.15)
project(MyMiniGame)

# 1. 设置VCPKG_TARGET_TRIPLET，指定MinGW目标三元组
set(VCPKG_TARGET_TRIPLET "x64-mingw-dynamic" CACHE STRING "Vcpkg triplet")

# 2. 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 3. 添加第三方库
find_package(OpenXLSX CONFIG REQUIRED)
message(STATUS "OpenXLSX_FOUND = ${OpenXLSX_FOUND}")
message(STATUS "OpenXLSX_INCLUDE_DIRS = ${OpenXLSX_INCLUDE_DIRS}")
get_target_property(inc_dirs OpenXLSX::OpenXLSX INTERFACE_INCLUDE_DIRECTORIES)
if(inc_dirs MATCHES "/include/OpenXLSX$")
    string(REGEX REPLACE "/OpenXLSX$" "" corrected_inc "${inc_dirs}")
    set_target_properties(OpenXLSX::OpenXLSX PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${corrected_inc}")
    message(STATUS "Corrected INTERFACE_INCLUDE_DIRECTORIES = ${corrected_inc}")
endif()

# 4. 搜集源文件
add_executable(MyMiniGame "test/test1.cpp")

# 5. 链接库
target_link_libraries(MyMiniGame PRIVATE OpenXLSX::OpenXLSX)
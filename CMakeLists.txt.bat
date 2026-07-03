cmake_minimum_required(VERSION 3.15)
project(MyMiniGame VERSION 1.0.0)

#set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 1. 设置CMAKE_TOOLCHAIN_FILE，指定vcpkg工具链 (请将路径替换成你的vcpkg实际路径)
set(CMAKE_TOOLCHAIN_FILE "E:/Code/Cpp/project/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

# 2. 设置VCPKG_TARGET_TRIPLET，指定MinGW目标三元组
set(VCPKG_TARGET_TRIPLET "x64-mingw-static" CACHE STRING "Vcpkg triplet")

# 3. 设置C++标准 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 4. 添加第三方库 
# list(APPEND CMAKE_PREFIX_PATH "E:/Code/Cpp/project/vcpkg/installed/x64-mingw-static/share/openxlsx")
find_package(OpenXLSX CONFIG REQUIRED)

# 5. 搜集源文件（假设你的源文件在src目录下）
file(GLOB SOURCES "test/test1.cpp")
add_executable(${PROJECT_NAME} ${SOURCES})

# 6. 链接库 (注意target名称已修正)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenXLSX::OpenXLSX)
cmake_minimum_required(VERSION 3.15)
project(MyMiniGame VERSION 1.0.0)

# 1. 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. 添加第三方库
find_package(OpenXLSX CONFIG REQUIRED)

# 3. 搜集源文件
file(GLOB SOURCES "test/test1.cpp") # 请确保路径正确
add_executable(${PROJECT_NAME} ${SOURCES})

# 4. 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE OpenXLSX::OpenXLSX)

# 5. 强烈建议删除之前手动设置的任何 CMAKE_FIND_ROOT_PATH_MODE_* 和 CMAKE_TOOLCHAIN_FILE 相关代码
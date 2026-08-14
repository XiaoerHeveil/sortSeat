set(CMAKE_TOOLCHAIN_FILE "D:/Code/Cpp/project/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

cmake_minimum_required(VERSION 3.15)
project(sortSeat LANGUAGES CXX)

# 1. 设置VCPKG_TARGET_TRIPLET，指定MinGW目标三元组
# set(VCPKG_TARGET_TRIPLET "x64-windows-dynamic" CACHE STRING "Vcpkg triplet")

# 指定UTF-8编码
if(MSVC)
    add_compile_options(/utf-8)
endif()

# 2. 设置C++标准
set(CMAKE_CXX_STANDARD 20)
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

# wxWidgets（GUI 进程使用）
find_package(wxWidgets REQUIRED COMPONENTS core base)

# 4. 业务与 GUI 共用的核心代码库
add_library(sorSeatCore STATIC
    src/fileInput.cpp
    src/sorting.cpp
    src/student.cpp
    sorSeatUI/MainFrame.cpp
    sorSeatUI/SeatPanel.cpp
    sorSeatUI/SequenceButton.cpp
    sorSeatUI/sorButton.cpp
)

target_include_directories(sorSeatCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/sorSeatUI
)

target_link_libraries(sorSeatCore PUBLIC OpenXLSX::OpenXLSX ${wxWidgets_LIBRARIES})
target_compile_definitions(sorSeatCore PRIVATE ${wxWidgets_DEFINITIONS})

# 5. 业务进程（控制台程序，后续用于 IPC 双向存活检测）
add_executable(sortSeat src/sorSeat.cpp)
target_link_libraries(sortSeat PRIVATE sorSeatCore)

# 6. GUI 进程（wxWidgets 窗口程序）
add_executable(sorSeatUI WIN32
    sorSeatUI/sorSeatApp.cpp
    sorSeatUI/sorSeatApp.h
)
target_link_libraries(sorSeatUI PRIVATE sorSeatCore)
target_compile_definitions(sorSeatUI PRIVATE ${wxWidgets_DEFINITIONS})

# 将 UI 资源复制到可执行文件旁边，保证 ./resource 相对路径可用
function(sorSeat_copy_resources target)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/sorSeatUI/resource
        $<TARGET_FILE_DIR:${target}>/resource
    )
endfunction()

sorSeat_copy_resources(sortSeat)
sorSeat_copy_resources(sorSeatUI)

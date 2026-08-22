# 优先使用环境变量 VCPKG_ROOT 定位 vcpkg 工具链，避免硬编码绝对路径
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

cmake_minimum_required(VERSION 3.21)
project(sortSeat LANGUAGES CXX)

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

# spdlog（异步日志）
find_package(spdlog CONFIG REQUIRED)

# 4. 业务与 GUI 共用的核心代码库
add_library(sorSeatCore STATIC
    src/fileInput.cpp
    src/sorting.cpp
    src/student.cpp
    src/Log.cpp
    src/NamedPipe.cpp
    src/Validate.cpp
    src/SeatEngine.cpp
    sorSeatUI/MainFrame.cpp
    sorSeatUI/IpcClient.cpp
    sorSeatUI/SeatPanel.cpp
    sorSeatUI/SequenceButton.cpp
    sorSeatUI/sorButton.cpp
    sorSeatUI/addRult.cpp
)

target_include_directories(sorSeatCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/sorSeatUI
)

target_link_libraries(sorSeatCore PUBLIC OpenXLSX::OpenXLSX ${wxWidgets_LIBRARIES} spdlog::spdlog)
target_compile_definitions(sorSeatCore PRIVATE ${wxWidgets_DEFINITIONS})

# 5. 业务进程（控制台程序，后续用于 IPC 双向存活检测）
add_executable(sortSeat src/sorSeat.cpp)
target_link_libraries(sortSeat PRIVATE sorSeatCore)

# 6. GUI 进程（wxWidgets 窗口程序）
add_executable(sorSeatUI WIN32
    sorSeatUI/sorSeatApp.cpp
    sorSeatUI/sorSeatApp.h
    sorSeatUI/sorSeatUI.rc
    sorSeatUI/addRult.h
    sorSeatUI/addRult.cpp
    sorSeatUI/presetsRult.h
    sorSeatUI/presetsRult.cpp
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

# ============ 打包（CPack） ============
# 两个 exe 必须安装到同一目录（前端 IpcClient 在同目录查找 sortSeat.exe）
install(TARGETS sortSeat sorSeatUI RUNTIME DESTINATION .)

# 运行时 DLL：vcpkg 的 applocal 部署已把全部第三方 DLL 复制到 exe 目录，
# 这里在安装时按当前构建配置（Release/Debug）收集 *.dll 一并安装。
install(CODE "
    file(GLOB _sortseat_dlls \"${CMAKE_BINARY_DIR}/\${CMAKE_INSTALL_CONFIG_NAME}/*.dll\")
    foreach(_d IN LISTS _sortseat_dlls)
        file(INSTALL \"\${_d}\" DESTINATION \"\${CMAKE_INSTALL_PREFIX}\")
    endforeach()
")

# UI 资源目录
install(DIRECTORY sorSeatUI/resource/ DESTINATION resource)

# 协议：安装后复制到安装目录 Licence 文件夹，供用户查看细分协议
install(DIRECTORY License/ DESTINATION Licence)

set(CPACK_PACKAGE_NAME "sortSeat")
set(CPACK_PACKAGE_VERSION "0.1.6")
set(CPACK_PACKAGE_VENDOR "sortSeat")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "sortSeat")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "排座位工具")
# 安装包文件名（main 分支为 sortSeatInstaller；win7-gbk 分支为 sortSeatInstaller_Win7）
set(CPACK_PACKAGE_FILE_NAME "sortSeatInstaller")
set(CPACK_GENERATOR "NSIS")
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/sorSeatUI/resource/Logo.ico")
set(CPACK_NSIS_INSTALLED_ICON_NAME "sorSeatUI.exe")
# 桌面快捷方式：安装后在桌面创建「排个座.lnk」指向 sorSeatUI.exe
set(CPACK_NSIS_CREATE_ICONS_EXTRA
    "CreateShortCut \\\"$DESKTOP\\\\排个座.lnk\\\" \\\"$INSTDIR\\\\sorSeatUI.exe\\\"")
set(CPACK_NSIS_DELETE_ICONS_EXTRA
    "Delete \\\"$DESKTOP\\\\排个座.lnk\\\"")
# EULA：安装前强制展示，不同意无法继续（UTF-8 BOM 编码）
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/Licence.txt")
include(CPack)

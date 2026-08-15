#include "IpcCommon.h"
#include "Log.h"
#include "NamedPipe.h"
#include "SeatEngine.h"
#include "Validate.h"
#include "sorting.h"
#include "student.h"

#include <OpenXLSX/OpenXLSX.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace ipc;

namespace {

// 将原生窄字符路径（Windows 下为 ANSI/GBK）转为 UTF-8，用于 --test 命令行参数
std::string toUtf8(const char *native) {
#ifdef _WIN32
    try {
        return pathToUtf8(std::filesystem::path(native));
    } catch (...) {
        return native;
    }
#else
    return native;
#endif
}

// 解析结果文本为二维网格（逗号=相邻单元格，空格=空单元格）
std::vector<std::vector<std::string>> parseResultGrid(const std::string &text) {
    std::vector<std::vector<std::string>> grid;
    size_t pos = 0;
    while (pos <= text.size()) {
        size_t nl = text.find('\n', pos);
        std::string line = (nl == std::string::npos)
                               ? text.substr(pos)
                               : text.substr(pos, nl - pos);
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        pos = (nl == std::string::npos) ? text.size() + 1 : nl + 1;

        // 与前端 ParseResultText 一致：逗号=相邻，空格=空出一个单元格
        std::vector<std::string> row;
        std::string cur;
        for (char ch : line) {
            if (ch == ',') {
                row.push_back(cur);
                cur.clear();
            } else if (ch == ' ') {
                row.push_back(cur);
                cur.clear();
                row.push_back("");
            } else {
                cur += ch;
            }
        }
        row.push_back(cur);
        grid.push_back(std::move(row));
    }
    return grid;
}

// 复制日志目录下所有文件到目标目录
bool exportLogs(const std::string &destDirUtf8) {
#ifdef _WIN32
    std::filesystem::path dest = std::filesystem::u8path(destDirUtf8);
    wchar_t exePath[MAX_PATH] = {0};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::filesystem::path logDir =
        std::filesystem::path(exePath).parent_path() / L"log";
    if (!std::filesystem::exists(logDir))
        return false;
    std::filesystem::create_directories(dest);
    int copied = 0;
    for (const auto &entry : std::filesystem::directory_iterator(logDir)) {
        if (entry.is_regular_file()) {
            std::filesystem::copy_file(entry.path(),
                                       dest / entry.path().filename(),
                                       std::filesystem::copy_options::overwrite_existing);
            ++copied;
        }
    }
    return copied > 0;
#else
    (void)destDirUtf8;
    return false;
#endif
}

} // namespace

// CLI 测试模式：sortSeat --test <学生文件> <规则文件> <列数> <小组组数>
static int runTestMode(int argc, char **argv) {
    if (argc < 6) {
        std::cerr << "用法: sortSeat --test <学生文件> <规则文件> <列数> <小组组数>\n";
        return 1;
    }
    seat::SeatRequest req;
    req.studentPath = toUtf8(argv[2]);
    req.rulesPath = toUtf8(argv[3]);
    req.x_row = std::stoi(argv[4]);
    req.groupCount = std::stoi(argv[5]);
    try {
        auto r = seat::compute(req);
        printSeatLayout(r.grid.data(), r.rows, r.columns, r.students);
        std::cout << "\n===== 结果文本（Result Text） =====\n"
                  << r.text << "\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "排序失败: " << e.what() << "\n";
        return 1;
    }
}

int main(int argc, char *argv[]) {
    Log::init("sortSeat");

    if (argc >= 2 && std::string(argv[1]) == "--test") {
        int rc = runTestMode(argc, argv);
        Log::shutdown();
        return rc;
    }

    SORLOG_INFO("sortSeat 后端启动，等待前端连接...");

    std::string lastResultPath;
    bool running = true;
    while (running) {
        PipeServer server;
        if (!server.createAndWait()) {
            Sleep(500);
            continue;
        }
        SORLOG_INFO("前端已连接");

        while (running) {
            Message msg;
            if (!server.receive(msg)) {
                SORLOG_INFO("前端断开连接");
                break;
            }
            Message reply;
            switch (msg.op) {
            case Op::HELLO:
                reply.op = Op::ACK;
                server.send(reply);
                break;

            case Op::PING:
                reply.op = Op::PONG;
                server.send(reply);
                break;

            case Op::START: {
                try {
                    auto fields = unpackFields(msg.payload);
                    if (fields.size() < 4)
                        throw std::runtime_error("START 参数不足");
                    seat::SeatRequest req;
                    req.x_row = std::stoi(fields[0]);
                    req.groupCount = std::stoi(fields[1]);
                    req.studentPath = fields[2];
                    req.rulesPath = fields[3];
                    lastResultPath = seat::computeToTempFile(req);
                    reply.op = Op::RESULT;
                    reply.payload = lastResultPath;
                    SORLOG_INFO("排序完成，结果文件: {}", lastResultPath);
                } catch (const std::exception &e) {
                    SORLOG_ERROR("START 处理失败: {}", e.what());
                    reply.op = Op::ERR;
                    reply.payload = e.what();
                }
                server.send(reply);
                break;
            }

            case Op::EXPORT_TXT: {
                try {
                    if (lastResultPath.empty())
                        throw std::runtime_error("尚未生成排序结果");
                    std::filesystem::path dest =
                        std::filesystem::u8path(msg.payload);
                    std::filesystem::create_directories(dest);
                    std::filesystem::copy_file(
                        std::filesystem::u8path(lastResultPath),
                        dest / L"seat_result.txt",
                        std::filesystem::copy_options::overwrite_existing);
                    reply.op = Op::ACK;
                } catch (const std::exception &e) {
                    reply.op = Op::ERR;
                    reply.payload = e.what();
                }
                server.send(reply);
                break;
            }

            case Op::EXPORT_EXCEL: {
                try {
                    if (lastResultPath.empty())
                        throw std::runtime_error("尚未生成排序结果");
                    std::filesystem::path dest =
                        std::filesystem::u8path(msg.payload);
                    std::filesystem::create_directories(dest);
                    std::ifstream in(std::filesystem::u8path(lastResultPath));
                    std::string text((std::istreambuf_iterator<char>(in)),
                                     std::istreambuf_iterator<char>());
                    auto grid = parseResultGrid(text);

                    std::filesystem::path outPath = dest / L"seat_result.xlsx";
                    OpenXLSX::XLDocument doc;
                    doc.create(pathToUtf8(outPath));
                    auto ws = doc.workbook().worksheet("Sheet1");
                    for (size_t r = 0; r < grid.size(); ++r)
                        for (size_t c = 0; c < grid[r].size(); ++c)
                            if (!grid[r][c].empty())
                                ws.cell(r + 1, c + 1).value() = grid[r][c];
                    doc.save();
                    doc.close();
                    reply.op = Op::ACK;
                } catch (const std::exception &e) {
                    reply.op = Op::ERR;
                    reply.payload = e.what();
                }
                server.send(reply);
                break;
            }

            case Op::EXPORT_LOG: {
                if (exportLogs(msg.payload))
                    reply.op = Op::ACK;
                else {
                    reply.op = Op::ERR;
                    reply.payload = "日志导出失败（可能无日志文件）";
                }
                server.send(reply);
                break;
            }

            case Op::SHUTDOWN:
                running = false;
                break;

            default:
                reply.op = Op::ERR;
                reply.payload = "未知操作码";
                server.send(reply);
                break;
            }
        }
    }

    Log::shutdown();
    return 0;
}

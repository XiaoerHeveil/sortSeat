#include "Log.h"

#include <spdlog/async.h>
#include <spdlog/sinks/daily_file_sink.h>

#include <chrono>
#include <filesystem>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace Log
{
    static std::shared_ptr<spdlog::logger> g_logger;

    // 解析可执行文件所在目录，日志统一写入该目录下的 log/ 文件夹
    static std::filesystem::path executableDirectory()
    {
#if defined(_WIN32)
        wchar_t buffer[MAX_PATH] = {0};
        DWORD len = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
        if (len > 0 && len < MAX_PATH)
            return std::filesystem::path(buffer).parent_path();
#endif
        return std::filesystem::current_path();
    }

    void init(const std::string &processName)
    {
        if (g_logger)
            return;
        try
        {
            std::filesystem::path logDir = executableDirectory() / "log";
            std::filesystem::create_directories(logDir);
            std::string logPath = (logDir / (processName + ".log")).string();

            spdlog::init_thread_pool(8192, 1);
            // 按天滚动，max_files=0 表示不限制数量，由 cleanupOldLogs 负责 30 天保留
            auto sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
                logPath, 0, 0, false, 0);
            auto logger = std::make_shared<spdlog::async_logger>(
                processName, sink, spdlog::thread_pool(),
                spdlog::async_overflow_policy::block);
            logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v");
            logger->set_level(spdlog::level::debug);
            logger->flush_on(spdlog::level::info);
            spdlog::register_logger(logger);
            g_logger = logger;

            cleanupOldLogs();
            logger->info("日志系统初始化完成，日志目录：{}", logDir.string());
        }
        catch (const std::exception &)
        {
            // 日志初始化失败不能影响主流程
        }
    }

    void shutdown()
    {
        if (g_logger)
        {
            g_logger->flush();
            g_logger.reset();
        }
        spdlog::shutdown();
    }

    void cleanupOldLogs()
    {
        try
        {
            std::filesystem::path logDir = executableDirectory() / "log";
            if (!std::filesystem::exists(logDir))
                return;
            auto now = std::filesystem::file_time_type::clock::now();
            for (const auto &entry :
                 std::filesystem::directory_iterator(logDir))
            {
                if (!entry.is_regular_file())
                    continue;
                auto age = now - entry.last_write_time();
                auto days = std::chrono::duration_cast<std::chrono::hours>(age)
                                .count() /
                            24;
                if (days > LogRecordDays)
                {
                    std::filesystem::remove(entry.path());
                    if (g_logger)
                        g_logger->info("已删除过期日志文件：{}",
                                       entry.path().string());
                }
            }
        }
        catch (...)
        {
        }
    }

    std::shared_ptr<spdlog::logger> logger()
    {
        return g_logger;
    }
}

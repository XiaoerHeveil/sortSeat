#pragma once

#include <spdlog/spdlog.h>
#include <string>

namespace Log
{
    // 日志保留天数，超出时长的日志文件将被清理
    static constexpr int LogRecordDays = 30;

    // 初始化日志系统；processName 用于区分进程（sortSeat / sorSeatUI）
    void init(const std::string &processName);
    // 关闭并刷新日志
    void shutdown();
    // 清理超过 LogRecordDays 的日志文件
    void cleanupOldLogs();
    // 获取当前日志器（未初始化时返回 nullptr）
    std::shared_ptr<spdlog::logger> logger();
}

#define SORLOG_TRACE(...)                                                      \
    do                                                                         \
    {                                                                          \
        auto _l = ::Log::logger();                                             \
        if (_l)                                                                \
            _l->trace(__VA_ARGS__);                                            \
    } while (0)

#define SORLOG_DEBUG(...)                                                      \
    do                                                                         \
    {                                                                          \
        auto _l = ::Log::logger();                                             \
        if (_l)                                                                \
            _l->debug(__VA_ARGS__);                                            \
    } while (0)

#define SORLOG_INFO(...)                                                       \
    do                                                                         \
    {                                                                          \
        auto _l = ::Log::logger();                                             \
        if (_l)                                                                \
            _l->info(__VA_ARGS__);                                             \
    } while (0)

#define SORLOG_WARN(...)                                                       \
    do                                                                         \
    {                                                                          \
        auto _l = ::Log::logger();                                             \
        if (_l)                                                                \
            _l->warn(__VA_ARGS__);                                             \
    } while (0)

#define SORLOG_ERROR(...)                                                      \
    do                                                                         \
    {                                                                          \
        auto _l = ::Log::logger();                                             \
        if (_l)                                                                \
            _l->error(__VA_ARGS__);                                            \
    } while (0)

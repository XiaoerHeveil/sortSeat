#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

// 业务进程与 GUI 进程共用的命名管道协议定义（头文件内联实现）
namespace ipc {

#ifdef _WIN32
inline constexpr const wchar_t *PipeName = L"\\\\.\\pipe\\sortSeatIpc";
#endif

// 将文件系统路径转为 UTF-8 字符串（管道传输统一使用 UTF-8）
inline std::string pathToUtf8(const std::filesystem::path &p) {
    std::u8string u = p.u8string();
    return std::string(u.begin(), u.end());
}

// 消息操作码（注意：不能命名为 ERROR，会与 Windows 的 ERROR 宏冲突）
enum class Op : std::uint32_t {
    HELLO = 1,       // 握手请求
    ACK,             // 握手应答
    PING,            // 心跳探测
    PONG,            // 心跳应答
    START,           // 前端 -> 后端：开始排序（负载：打包字段）
    RESULT,          // 后端 -> 前端：结果文本文件路径
    EXPORT_TXT,      // 前端 -> 后端：导出 TXT（负载：目录路径）
    EXPORT_EXCEL,    // 前端 -> 后端：导出 Excel（负载：目录路径）
    EXPORT_PNG,      // 前端 -> 后端：导出 PNG（暂未实现）
    EXPORT_LOG,      // 前端 -> 后端：导出日志（负载：目录路径）
    ERR,             // 错误信息
    SHUTDOWN,        // 关闭
};

// 时间常量（毫秒），对齐参考文件
inline constexpr int StartupPollMs = 3000;             // 启动检测频率
inline constexpr int StartupTimeoutMs = 15000;         // 启动超时
inline constexpr int HeartbeatMs = 7000;               // 存活检测频率（握手后降低）
inline constexpr int HeartbeatTimeoutMs = 3 * HeartbeatMs + 3000; // 存活超时（预留抖动）
inline constexpr int MaxRestart = 3;                   // 最大重启次数

// 单条消息最大字节数（大字符串走 %temp% 文件，消息保持轻量）
inline constexpr std::uint32_t MaxMessageSize = 1u << 20; // 1 MiB

// 消息结构
struct Message {
    Op op = Op::ERR;
    std::string payload;
};

// ============ 小端序序列化辅助 ============
inline void appendU32(std::string &out, std::uint32_t v) {
    out.push_back(static_cast<char>(v & 0xff));
    out.push_back(static_cast<char>((v >> 8) & 0xff));
    out.push_back(static_cast<char>((v >> 16) & 0xff));
    out.push_back(static_cast<char>((v >> 24) & 0xff));
}

inline std::uint32_t readU32(const char *p) {
    return static_cast<std::uint32_t>(static_cast<unsigned char>(p[0])) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(p[1])) << 8) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(p[2])) << 16) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(p[3])) << 24);
}

// 字段序列化：按 [uint32 len][bytes] 拼接多个字符串字段
inline std::string packFields(const std::vector<std::string> &fields) {
    std::string out;
    for (const auto &f : fields) {
        appendU32(out, static_cast<std::uint32_t>(f.size()));
        out += f;
    }
    return out;
}

// 字段反序列化
inline std::vector<std::string> unpackFields(const std::string &payload) {
    std::vector<std::string> fields;
    size_t pos = 0;
    while (pos + 4 <= payload.size()) {
        std::uint32_t len = readU32(payload.data() + pos);
        pos += 4;
        if (pos + len > payload.size())
            break;
        fields.emplace_back(payload.substr(pos, len));
        pos += len;
    }
    return fields;
}

// 将 Message 编码为字节流：[uint32 opcode][uint32 len][payload]
inline std::string encode(const Message &msg) {
    std::string out;
    appendU32(out, static_cast<std::uint32_t>(msg.op));
    appendU32(out, static_cast<std::uint32_t>(msg.payload.size()));
    out += msg.payload;
    return out;
}

// 从字节流解码（返回 false 表示数据不完整/非法）
inline bool decode(const std::string &data, Message &msg) {
    if (data.size() < 8)
        return false;
    msg.op = static_cast<Op>(readU32(data.data()));
    std::uint32_t len = readU32(data.data() + 4);
    if (8 + len > data.size())
        return false;
    msg.payload.assign(data.data() + 8, len);
    return true;
}

} // namespace ipc

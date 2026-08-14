#pragma once

#include "IpcCommon.h"
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

// 命名管道封装：后端作为服务端，前端作为客户端（均为全双工 message-mode 管道）
namespace ipc {

class NamedPipe {
public:
    NamedPipe() = default;
    virtual ~NamedPipe();
    NamedPipe(const NamedPipe &) = delete;
    NamedPipe &operator=(const NamedPipe &) = delete;

    bool isOpen() const;
    void close();

    bool send(const Message &msg);       // 发送一条消息
    bool receive(Message &msg);          // 阻塞接收一条消息
    bool tryReceive(Message &msg);       // 非阻塞尝试接收（PeekNamedPipe）

protected:
#ifdef _WIN32
    HANDLE m_handle = INVALID_HANDLE_VALUE;
#endif
};

// 服务端（后端 sortSeat 使用）
class PipeServer : public NamedPipe {
public:
    bool createAndWait(); // 创建命名管道并等待客户端连接
};

// 客户端（前端 sorSeatUI 使用）
class PipeClient : public NamedPipe {
public:
    bool connect(int timeoutMs); // 连接命名管道（先等待其可用）
};

} // namespace ipc

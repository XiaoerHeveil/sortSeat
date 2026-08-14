#pragma once

#include "IpcCommon.h"
#include "NamedPipe.h"

#include <string>

// 前端进程的后端客户端：负责拉起 sortSeat.exe、连接管道、收发消息
namespace ipc {

class BackendClient {
public:
    static BackendClient &instance();

    BackendClient(const BackendClient &) = delete;
    BackendClient &operator=(const BackendClient &) = delete;

    // 拉起同目录下的 sortSeat.exe（无窗口）
    bool launchBackend();

    // 连接后端（WaitNamedPipe + CreateFile）
    bool connect(int timeoutMs);

    bool isOpen() const;
    void close();

    bool send(const Message &msg);
    bool receive(Message &msg);                // 阻塞接收
    bool receive(Message &msg, int timeoutMs); // 超时接收（轮询 PeekNamedPipe）

    // 握手：发送 HELLO，等待 ACK
    bool handshake();
    // 心跳：发送 PING，等待 PONG
    bool ping();

private:
    BackendClient() = default;
    PipeClient m_pipe;
};

} // namespace ipc

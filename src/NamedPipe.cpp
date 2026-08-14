#include "NamedPipe.h"

namespace ipc {

NamedPipe::~NamedPipe() {
    close();
}

bool NamedPipe::isOpen() const {
#ifdef _WIN32
    return m_handle != INVALID_HANDLE_VALUE;
#else
    return false;
#endif
}

void NamedPipe::close() {
#ifdef _WIN32
    if (m_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
    }
#endif
}

bool NamedPipe::send(const Message &msg) {
#ifdef _WIN32
    if (m_handle == INVALID_HANDLE_VALUE)
        return false;
    std::string data = encode(msg);
    DWORD written = 0;
    BOOL ok = WriteFile(m_handle, data.data(),
                        static_cast<DWORD>(data.size()), &written, nullptr);
    return ok && written == data.size();
#else
    return false;
#endif
}

bool NamedPipe::receive(Message &msg) {
#ifdef _WIN32
    if (m_handle == INVALID_HANDLE_VALUE)
        return false;
    std::string buf(MaxMessageSize, '\0');
    DWORD read = 0;
    BOOL ok = ReadFile(m_handle, buf.data(), static_cast<DWORD>(buf.size()),
                       &read, nullptr);
    if (!ok || read < 8)
        return false;
    buf.resize(read);
    return decode(buf, msg);
#else
    return false;
#endif
}

bool NamedPipe::tryReceive(Message &msg) {
#ifdef _WIN32
    if (m_handle == INVALID_HANDLE_VALUE)
        return false;
    std::string header(8, '\0');
    DWORD headerAvail = 0;
    DWORD totalAvail = 0;
    if (!PeekNamedPipe(m_handle, header.data(), 8, &headerAvail, &totalAvail,
                       nullptr))
        return false;
    if (headerAvail < 8)
        return false;
    std::uint32_t len = readU32(header.data() + 4);
    if (totalAvail < 8 + len)
        return false; // 完整消息尚未到齐
    return receive(msg);
#else
    return false;
#endif
}

bool PipeServer::createAndWait() {
#ifdef _WIN32
    m_handle = CreateNamedPipeW(
        PipeName,
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,                 // 实例数
        MaxMessageSize,    // 输出缓冲
        MaxMessageSize,    // 输入缓冲
        0,                 // 默认超时
        nullptr);          // 默认安全属性
    if (m_handle == INVALID_HANDLE_VALUE)
        return false;
    BOOL ok = ConnectNamedPipe(m_handle, nullptr);
    if (!ok && GetLastError() != ERROR_PIPE_CONNECTED) {
        close();
        return false;
    }
    return true;
#else
    return false;
#endif
}

bool PipeClient::connect(int timeoutMs) {
#ifdef _WIN32
    if (!WaitNamedPipeW(PipeName, static_cast<DWORD>(timeoutMs)))
        return false;
    m_handle = CreateFileW(PipeName, GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                           OPEN_EXISTING, 0, nullptr);
    return m_handle != INVALID_HANDLE_VALUE;
#else
    return false;
#endif
}

} // namespace ipc

#include "IpcClient.h"

#include <chrono>
#include <filesystem>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#endif

namespace ipc {

BackendClient &BackendClient::instance() {
    static BackendClient inst;
    return inst;
}

bool BackendClient::launchBackend() {
#ifdef _WIN32
    wchar_t exePath[MAX_PATH] = {0};
    DWORD len = GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    if (len == 0 || len >= MAX_PATH)
        return false;
    std::filesystem::path dir = std::filesystem::path(exePath).parent_path();
    std::filesystem::path backend = dir / L"sortSeat.exe";
    if (!std::filesystem::exists(backend))
        return false;

    STARTUPINFOW si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));
    if (!CreateProcessW(backend.c_str(), nullptr, nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
        return false;
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
#else
    return false;
#endif
}

bool BackendClient::connect(int timeoutMs) {
    m_pipe.close();
    return m_pipe.connect(timeoutMs);
}

bool BackendClient::isOpen() const { return m_pipe.isOpen(); }

void BackendClient::close() { m_pipe.close(); }

bool BackendClient::send(const Message &msg) { return m_pipe.send(msg); }

bool BackendClient::receive(Message &msg) { return m_pipe.receive(msg); }

bool BackendClient::receive(Message &msg, int timeoutMs) {
    using namespace std::chrono;
    auto deadline = steady_clock::now() + milliseconds(timeoutMs);
    while (steady_clock::now() < deadline) {
        if (m_pipe.tryReceive(msg))
            return true;
        if (!m_pipe.isOpen())
            return false;
        std::this_thread::sleep_for(milliseconds(50));
    }
    return false;
}

bool BackendClient::handshake() {
    Message hello;
    hello.op = Op::HELLO;
    if (!send(hello))
        return false;
    Message ack;
    if (!receive(ack, 3000))
        return false;
    return ack.op == Op::ACK;
}

bool BackendClient::ping() {
    Message p;
    p.op = Op::PING;
    if (!send(p))
        return false;
    Message pong;
    if (!receive(pong, 3000))
        return false;
    return pong.op == Op::PONG;
}

} // namespace ipc

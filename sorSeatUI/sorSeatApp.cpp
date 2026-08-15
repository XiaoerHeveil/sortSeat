#include "sorSeatApp.h"
#include "MainFrame.h"
#include "Log.h"
#include "IpcClient.h"

wxIMPLEMENT_APP(sorSeatApp);

bool sorSeatApp::OnInit()
{
    Log::init("sorSeatUI");
    MainFrame *mainFrame = new MainFrame(L"排个座");
    // 统一使用 Logo.ico（资源名 sortSeatIcon），覆盖任务栏/任务管理器/标题栏默认图标
    mainFrame->SetIcon(wxICON(sortSeatIcon));
    mainFrame->SetClientSize(1280, 720);
    mainFrame->SetSizeHints(wxSize(1280, 720), wxDefaultSize);
    mainFrame->Center();
    mainFrame->Show();

    // 拉起后端并进入启动检测
    auto &backend = ipc::BackendClient::instance();
    backend.launchBackend();

    m_startupTimer.SetOwner(this);
    m_heartbeatTimer.SetOwner(this);
    Bind(wxEVT_TIMER, &sorSeatApp::OnStartupTick, this, m_startupTimer.GetId());
    Bind(wxEVT_TIMER, &sorSeatApp::OnHeartbeatTick, this, m_heartbeatTimer.GetId());
    m_startupTimer.Start(ipc::StartupPollMs);

    return true;
}

int sorSeatApp::OnExit()
{
    auto &backend = ipc::BackendClient::instance();
    if (backend.isOpen())
    {
        ipc::Message msg;
        msg.op = ipc::Op::SHUTDOWN;
        backend.send(msg);
    }
    backend.close();
    Log::shutdown();
    return 0;
}

void sorSeatApp::OnStartupTick(wxTimerEvent &)
{
    auto &backend = ipc::BackendClient::instance();
    ++m_startupTicks;

    if (!backend.isOpen())
        backend.connect(1500);
    if (backend.isOpen() && backend.handshake())
    {
        m_connected = true;
        m_startupTimer.Stop();
        StartHeartbeat();
        SORLOG_INFO("与后端握手成功");
        return;
    }

    if (m_startupTicks * ipc::StartupPollMs >= ipc::StartupTimeoutMs)
    {
        if (m_restartCount < ipc::MaxRestart)
        {
            ++m_restartCount;
            m_startupTicks = 0;
            backend.close();
            backend.launchBackend();
            SORLOG_WARN("后端未响应，第 {} 次重启", m_restartCount);
        }
        else
        {
            m_startupTimer.Stop();
            FailHardware();
        }
    }
}

void sorSeatApp::StartHeartbeat()
{
    m_missedPongs = 0;
    m_heartbeatTimer.Start(ipc::HeartbeatMs);
}

void sorSeatApp::OnHeartbeatTick(wxTimerEvent &)
{
    auto &backend = ipc::BackendClient::instance();
    if (!backend.isOpen() || !backend.ping())
    {
        ++m_missedPongs;
        if (m_missedPongs * ipc::HeartbeatMs >= ipc::HeartbeatTimeoutMs)
        {
            m_heartbeatTimer.Stop();
            wxMessageBox(L"主进程意外退出", L"错误", wxOK | wxICON_ERROR);
            ExitMainLoop();
        }
    }
    else
    {
        m_missedPongs = 0;
    }
}

void sorSeatApp::FailHardware()
{
    wxMessageBox(L"你的计算机硬件配置过低或此程序与你的计算机不兼容！",
                 L"错误", wxOK | wxICON_ERROR);
    ExitMainLoop();
}

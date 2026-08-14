#pragma once
#include <wx/wx.h>

class sorSeatApp : public wxApp
{
public:
    bool OnInit() override;
    int OnExit() override;

private:
    void OnStartupTick(wxTimerEvent &evt);
    void OnHeartbeatTick(wxTimerEvent &evt);
    void StartHeartbeat();
    void FailHardware();

    wxTimer m_startupTimer;
    wxTimer m_heartbeatTimer;
    int m_startupTicks = 0; // 当前重启周期内已检测次数
    int m_restartCount = 0; // 已重启次数
    int m_missedPongs = 0;  // 心跳未响应次数
    bool m_connected = false;
};

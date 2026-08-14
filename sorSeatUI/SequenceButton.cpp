#include "SequenceButton.h"

SequenceButton::SequenceButton(wxWindow *parent, const std::vector<wxBitmap> &frames)
    : wxStaticBitmap(parent, wxID_ANY, frames.empty() ? wxNullBitmap : frames[0]),
      m_frames(frames), m_currentFrame(0),
      m_isPlayingForward(false), m_isPlayingBackward(false),
      m_timer(nullptr)
{ // 初始化为空
    Bind(wxEVT_LEFT_DOWN, &SequenceButton::OnMouseDown, this);
    // 定时器事件绑定可以推迟，或者用 Bind 绑定到 this 的定时器事件（所有定时器事件都发往 this）
    Bind(wxEVT_TIMER, &SequenceButton::OnTimer, this);
}

SequenceButton::~SequenceButton()
{
    StopTimer();
    delete m_timer; // 安全删除
}

void SequenceButton::SetFrame(int idx)
{
    if (idx >= 0 && idx < (int)m_frames.size())
    {
        SetBitmap(m_frames[idx]);
        Refresh();
    }
}

void SequenceButton::StartTimer()
{
    StopTimer(); // 确保先停止旧的
    // 延迟创建定时器
    if (m_timer == nullptr)
    {
        m_timer = new wxTimer(this); // 此时 wxApp 已完全初始化
    }
    int delay = 10; // 你可以根据帧索引动态调整
    m_timer->Start(delay, wxTIMER_ONE_SHOT);
}

void SequenceButton::StopTimer()
{
    if (m_timer && m_timer->IsRunning())
    {
        m_timer->Stop();
    }
}

void SequenceButton::OnMouseDown(wxMouseEvent &evt)
{
    StopTimer(); // 停止任何正在进行的动画

    if (!m_isPlayingForward && !m_isPlayingBackward)
    {
        m_isPlayingForward = true;
        m_currentFrame = 0;
        StartTimer(); // 首次启动定时器
    }
    else if (m_isPlayingForward)
    {
        m_isPlayingForward = false;
        m_isPlayingBackward = true;
        StartTimer();
    }
    else if (m_isPlayingBackward)
    {
        m_isPlayingBackward = false;
        m_isPlayingForward = true;
        StartTimer();
    }
    evt.Skip();
}

void SequenceButton::OnTimer(wxTimerEvent &evt)
{
    int total = (int)m_frames.size();
    if (total == 0)
        return;

    if (m_isPlayingForward)
    {
        m_currentFrame++;
        if (m_currentFrame >= total)
        {
            m_currentFrame = total - 1;
            SetFrame(m_currentFrame);
            m_isPlayingForward = false;
            // 动画结束，不重设定时器
            return;
        }
    }
    else if (m_isPlayingBackward)
    {
        m_currentFrame--;
        if (m_currentFrame < 0)
        {
            m_currentFrame = 0;
            SetFrame(m_currentFrame);
            m_isPlayingBackward = false;
            return;
        }
    }

    SetFrame(m_currentFrame);
    // 继续下一帧
    int delay = 50; // 可改为根据帧序号动态计算
    if (m_timer)
        m_timer->Start(delay, wxTIMER_ONE_SHOT);
}
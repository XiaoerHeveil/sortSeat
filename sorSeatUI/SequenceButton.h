#pragma once
#include <wx/wx.h>
#include <wx/timer.h>
#include <wx/statbmp.h>
#include <vector>

class SequenceButton : public wxStaticBitmap
{
public:
    SequenceButton(wxWindow *parent, const std::vector<wxBitmap> &frames);
    virtual ~SequenceButton();

private:
    std::vector<wxBitmap> m_frames;
    int m_currentFrame;
    bool m_isPlayingForward;
    bool m_isPlayingBackward;
    wxTimer *m_timer;
    static constexpr int FRAME_DELAY = 50;

    void SetFrame(int idx);
    void OnMouseDown(wxMouseEvent &evt);
    void StartTimer();
    void StopTimer();
    void OnTimer(wxTimerEvent &evt);
};
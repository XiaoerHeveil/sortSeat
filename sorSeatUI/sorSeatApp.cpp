#include "sorSeatApp.h"
#include "MainFrame.h"
#include "Log.h"

wxIMPLEMENT_APP(sorSeatApp);

bool sorSeatApp::OnInit()
{
    Log::init("sorSeatUI");
    MainFrame *mainFrame = new MainFrame(L"排个座");
    mainFrame->SetClientSize(1280, 720);
    mainFrame->SetSizeHints(wxSize(1280, 720), wxDefaultSize);
    mainFrame->Center();
    mainFrame->Show();
    return true;
}

int sorSeatApp::OnExit()
{
    Log::shutdown();
    return 0;
}

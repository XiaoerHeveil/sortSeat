#include "sorSeatApp.h"
#include "MainFrame.h"

wxIMPLEMENT_APP(sorSeatApp);

bool sorSeatApp::OnInit()
{
    MainFrame *mainFrame = new MainFrame("排个座");
    mainFrame->SetClientSize(1280, 720);
    mainFrame->SetSizeHints(wxSize(1280, 720), wxDefaultSize);
    mainFrame->Center();
    mainFrame->Show();
    return true;
}

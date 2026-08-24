#include <LWS/Cursor.hpp>
#include <LWS/Platform.hpp>
#include <LWS/Window.hpp>
#include <LLUtils/StringDefs.h>
#include <LLUtils/Colors.h>

int main()
{
    using namespace LWS;

    if (Platform::init() != Result::Success)
    {
        return 1;
    }
    Window win;
    WindowConfig config;
    config.title = LLUTILS_TEXT("Hello LWS");
    config.size = {800, 600};
    config.styles = WindowStyle::Caption | WindowStyle::CloseButton | WindowStyle::MaximizeButton |
                    WindowStyle::MinimizeButton | WindowStyle::ResizableBorder;
    config.visible = true;

    if (win.Create(config) != Result::Success)
    {
        Platform::shutdown();
        return 1;
    }
    win.SetWindowStyles(WindowStyle::ResizableBorder, true);
    win.SetBackgroundColor(LLUtils::Colors::Red);
    win.SetEraseBackground(true);

    Platform::runMessageLoop();
    Platform::shutdown();

    return 0;
}

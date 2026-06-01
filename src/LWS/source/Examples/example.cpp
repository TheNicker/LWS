#include <LWS/Cursor.hpp>
#include <LWS/Platform.hpp>
#include <LWS/Window.hpp>
#include <LLUtils/StringDefs.h>

#ifdef LWS_PLATFORM_WIN32
#include <LWS/Win32/CursorWin32.hpp>
#include <LWS/Win32/WindowWin32.hpp>
#endif

int main()
{
    using namespace LWS;

    Platform::init();

#ifdef LWS_PLATFORM_WIN32
    Win32::WindowWin32 win;
    CursorWin32 cur;
    cur.setVisible(true);
    win.SetMenuChar(false);
#else
    Window win;
    Cursor cur;
#endif

    WindowConfig config;
    config.title = LLUTILS_TEXT("Hello LWS");
    config.size = { 800, 600 };
    config.styles = WindowStyle::Caption | WindowStyle::CloseButton |
        WindowStyle::MaximizeButton | WindowStyle::MinimizeButton |
        WindowStyle::ResizableBorder;
    config.visible = true;

    win.Create(config);
    win.SetMouseCursor(&cur);

    Platform::runMessageLoop();
    Platform::shutdown();

    return 0;
}
#include <LWS/facade/Window.hpp>
#include <LWS/facade/Cursor.hpp>
#include <LWS/Win32/WindowWin32.hpp>
#include <LWS/Win32/CursorWin32.hpp>
#include <LWS/facade/Window.hpp>
#include <LWS/facade/Platform.hpp>
#include <LWS/X11/WindowX11.hpp>

//#if LWS_SUPPORTED_PLATFORM(LWS_PLATFORM_WIN32)
// #endif

int main() 
{
 using namespace LWS;
 WindowWin32 win;
 
 CursorWin32 cur;
//  Window win2 = std::move(WindowWin32());

 win.setTitle("Hello Window");
 cur.setVisible(true);
 win.setCursor(&cur);
 
 static_cast<WindowWin32&>(win).win32SpecialCall();
 win.show();

 WindowX11 winX11;
 winX11.setTitle("My X11 Window");

 using namespace LWS;

 Window window = std::move(winx11);

 window.Create();
 window.SetWindowStyles(WindowStyle::Caption | WindowStyle::CloseButton | WindowStyle::MaximizeButton |
                            WindowStyle::MinimizeButton | WindowStyle::ResizableBorder,
                        true);
 window.SetVisible(true);



 MessageLoop();

 return 0;
}

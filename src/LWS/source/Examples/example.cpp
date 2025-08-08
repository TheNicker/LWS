#include <LWS/facade/Window.hpp>
#include <LWS/facade/Cursor.hpp>
#include <LWS/Win32/WindowWin32.hpp>
#include <LWS/Win32/CursorWin32.hpp>
#include <LWS/facade/Window.hpp>
#include <LWS/X11/WindowX11.hpp>

int main() 
{
 using namespace LWS;
  Window win = std::move(WindowWin32());
  win.setTitle("My Win32 Window");
  Cursor cur = std::move(CursorWin32());
  Window win2 = std::move(WindowWin32());
  win.setTitle("Hello Window");
  cur.setVisible(true);
  win.setCursor(&cur);
  static_cast<WindowWin32&>(win).win32SpecialCall();
  win.show();

  WindowX11 winX11;
  winX11.setTitle("My X11 Window");
  

  return 0;
}

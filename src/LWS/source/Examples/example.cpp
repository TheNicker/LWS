#include <LWS/facade/Window.h>
#include <LWS/facade/Cursor.h>
#include <LWS/Win32/WindowsWin32.hpp>
#include <LWS/facade/Window.h>

int main() 
{
 using namespace LWS;
  Window win = std::move(Win32Window());
  win.setTitle("My Win32 Window");
  Cursor cur = std::move(Win32Cursor());
  Window win2 = std::move(Win32Window());
  win.setTitle("Hello Window");
  cur.setVisible(true);
  win.setCursor(cur);
  static_cast<Win32Window&>(win).win32SpecialCall();
  win.show();
  return 0;
}

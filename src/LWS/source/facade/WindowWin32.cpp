#include <LWS/Win32/WindowWin32.hpp>
#include <LWS/Win32/WindowBackendWin32.hpp>
namespace LWS
{

  WindowWin32::WindowWin32() :Window::Window(std::make_shared<WindowBackendWin32>())
{

}


void WindowWin32::win32SpecialCall() 
{
auto *win32 = static_cast<WindowBackendWin32 *>(impl_.get());
win32->specialFeature();
}
}
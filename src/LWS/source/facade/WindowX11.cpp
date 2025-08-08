#include <LWS/X11/WindowX11.hpp>
#include <LWS/X11/WindowBackendX11.hpp>
namespace LWS
{
WindowX11::WindowX11() :Window::Window(std::make_shared<WindowBackendX11>())
{
  
}

void WindowX11::x11SpecialCall()
{
  auto *x11Windowbackend = static_cast<WindowBackendX11*>(impl_.get());
  x11Windowbackend->getServerAddress();
}

}
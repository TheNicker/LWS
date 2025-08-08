#include <iostream>
#include <LWS/interfaces/backends.hpp>
#include <LWS/facade/Window.hpp>
#include <LWS/facade/Cursor.hpp>
#include <LWS/X11/WindowBackendX11.hpp>

namespace LWS
{
    void WindowBackendX11::show() 
    {
    std::cout << "X11: Show window\n"; 
    }

    void WindowBackendX11::setTitle(std::string_view title) 
    {
    std::cout << "X11: Set title to " << title << "\n";
    }

    void WindowBackendX11::setCursor(std::shared_ptr<ICursorBackend> cursor) 
    {
    std::cout << "X11: Set cursor\n";
    }

    BackendId WindowBackendX11::backend() const { return BackendId::Win32; }

    void WindowBackendX11::getServerAddress() 
    {
    std::cout << "X11: Special get X11 serveral address feature\n";
    }
}
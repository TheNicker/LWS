#include <iostream>
#include <LWS/interfaces/backends.hpp>
#include <LWS/facade/Window.hpp>
#include <LWS/facade/Cursor.hpp>
#include <LWS/Win32/WindowBackendWin32.hpp>

namespace LWS
{
    void WindowBackendWin32::show() 
    {
    std::cout << "Win32: Show window\n"; 
    }

    void WindowBackendWin32::setTitle(std::string_view title) 
    {
    std::cout << "Win32: Set title to " << title << "\n";
    }

    void WindowBackendWin32::setCursor(std::shared_ptr<ICursorBackend> cursor) 
    {
    std::cout << "Win32: Set cursor\n";
    }

    BackendId WindowBackendWin32::backend() const { return BackendId::Win32; }

    void WindowBackendWin32::specialFeature() 
    {
    std::cout << "Win32: Special backend-specific feature\n";
    }
}
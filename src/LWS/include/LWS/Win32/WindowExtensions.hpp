#pragma once
#ifdef LWS_PLATFORM_WIN32

    #include <LWS/Result.hpp>
    #include <LWS/Win32/EventWin32.hpp>

namespace LWS
{
    class Window;
}

namespace LWS::Win32
{
    [[nodiscard]] Result SetPlatformCallback(Window& window, PlatformCallback callback);
    [[nodiscard]] Result SetMenuChar(Window& window, bool suppress);
}  // namespace LWS::Win32

#endif  // LWS_PLATFORM_WIN32

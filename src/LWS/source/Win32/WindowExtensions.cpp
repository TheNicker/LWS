#ifdef LWS_PLATFORM_WIN32

    #include <LWS/Win32/WindowBackendWin32.hpp>
    #include <LWS/Win32/WindowExtensions.hpp>

    #include "../internal/WindowBackendAccess.hpp"

    #include <utility>

namespace LWS::Win32
{
    namespace
    {
        WindowBackendWin32* GetBackend(Window& window)
        {
            IWindowBackend* backend = internal::WindowBackendAccess::Get(window);
            if (backend == nullptr || backend->backend() != BackendId::Win32)
                return nullptr;

            // Built-in BackendId values identify their concrete backend type.
            return static_cast<WindowBackendWin32*>(backend);
        }
    }  // namespace

    Result SetPlatformCallback(Window& window, PlatformCallback callback)
    {
        WindowBackendWin32* backend = GetBackend(window);
        if (backend == nullptr)
            return Result::NotSupported;

        backend->setPlatformCallback(std::move(callback));
        return Result::Success;
    }

    Result SetMenuChar(Window& window, bool suppress)
    {
        WindowBackendWin32* backend = GetBackend(window);
        if (backend == nullptr)
            return Result::NotSupported;

        backend->setMenuChar(suppress);
        return Result::Success;
    }
}  // namespace LWS::Win32

#endif  // LWS_PLATFORM_WIN32

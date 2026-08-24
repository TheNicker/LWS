// Wayland-specific WindowWayland extension methods.
#include <LWS/Wayland/WindowWayland.hpp>
#include <LWS/Wayland/WindowBackendWayland.hpp>

#ifdef LWS_PLATFORM_WAYLAND

namespace LWS::Wayland
{
    void WindowWayland::SetAppId(const std::string& appId)
    {
        fAppId = appId;
        if (auto* backend = getBackendAs<WindowBackendWayland>(); backend != nullptr)
        {
            backend->setAppId(appId);
        }
    }

    std::string WindowWayland::GetAppId() const
    {
        return fAppId;
    }
}  // namespace LWS::Wayland

#endif  // LWS_PLATFORM_WAYLAND

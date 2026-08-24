#ifdef LWS_PLATFORM_WAYLAND
    #include <LWS/Platform.hpp>
    #include "internal/PlatformState.hpp"

namespace LWS::internal::platform_backend
{
    using Platform::Feature;
    using Platform::MonitorDesc;
    Result init()
    {
        return WaylandPlatformState::current().initialize();
    }
    void shutdown()
    {
        WaylandPlatformState::current().shutdown();
    }
    bool isInitialized()
    {
        return WaylandPlatformState::current().isInitialized();
    }
    void runMessageLoop()
    {
        WaylandPlatformState::current().runMessageLoop();
    }
    bool processMessages()
    {
        return WaylandPlatformState::current().processMessages();
    }
    void requestQuit()
    {
        WaylandPlatformState::current().requestQuit();
    }
    bool supports(Feature feature)
    {
        return feature == Feature::ServerSideDecorations &&
               WaylandPlatformState::current().decorationManager() != nullptr;
    }
    bool isKeyPressed(KeyCode key)
    {
        return WaylandPlatformState::current().isKeyPressed(key);
    }
    bool isKeyToggled(KeyCode)
    {
        return false;
    }
    Point getMousePosition()
    {
        return WaylandPlatformState::current().pointerPosition();
    }
    void moveMouse(Point) {}
    void browseToFile(const std::filesystem::path&) {}
    void refreshMonitors()
    {
        if (auto* display = WaylandPlatformState::current().display(); display != nullptr)
        {
            std::ignore = wl_display_roundtrip(display);
        }
    }
    MonitorDesc getMonitorInfo(Handle handle, bool allowRefresh)
    {
        if (allowRefresh)
            refreshMonitors();
        return WaylandPlatformState::current().monitorInfo(handle);
    }
    MonitorDesc getPrimaryMonitor(bool allowRefresh)
    {
        if (allowRefresh)
            refreshMonitors();
        return WaylandPlatformState::current().primaryMonitor();
    }
    Rect getBoundingMonitorArea()
    {
        return WaylandPlatformState::current().boundingMonitorArea();
    }
}  // namespace LWS::internal::platform_backend
#endif

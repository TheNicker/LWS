#include <LWS/Platform.hpp>
#include "internal/PlatformBackend.hpp"

namespace LWS::Platform
{
    Result init() { return internal::platform_backend::init(); }
    void shutdown() { internal::platform_backend::shutdown(); }
    bool isInitialized() { return internal::platform_backend::isInitialized(); }
    void runMessageLoop() { internal::platform_backend::runMessageLoop(); }
    bool processMessages() { return internal::platform_backend::processMessages(); }
    void requestQuit() { internal::platform_backend::requestQuit(); }
    bool supports(Feature feature) { return internal::platform_backend::supports(feature); }
    bool isKeyPressed(KeyCode key) { return internal::platform_backend::isKeyPressed(key); }
    bool isKeyToggled(KeyCode key) { return internal::platform_backend::isKeyToggled(key); }
    Point getMousePosition() { return internal::platform_backend::getMousePosition(); }
    void moveMouse(Point delta) { internal::platform_backend::moveMouse(delta); }
    void browseToFile(const std::filesystem::path& path) { internal::platform_backend::browseToFile(path); }
    void refreshMonitors() { internal::platform_backend::refreshMonitors(); }
    MonitorDesc getMonitorInfo(Handle handle, bool allowRefresh)
    {
        return internal::platform_backend::getMonitorInfo(handle, allowRefresh);
    }
    MonitorDesc getPrimaryMonitor(bool allowRefresh)
    {
        return internal::platform_backend::getPrimaryMonitor(allowRefresh);
    }
    Rect getBoundingMonitorArea() { return internal::platform_backend::getBoundingMonitorArea(); }

    Session::Session() : fResult(init()) {}

    Session::~Session()
    {
        if (fResult == Result::Success)
            shutdown();
    }
}

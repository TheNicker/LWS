#pragma once

#include <LWS/Platform.hpp>

namespace LWS::internal::platform_backend
{
    Result init();
    void shutdown();
    [[nodiscard]] bool isInitialized();

    void runMessageLoop();
    [[nodiscard]] bool processMessages();
    void requestQuit();

    [[nodiscard]] bool supports(Platform::Feature feature);
    [[nodiscard]] bool isKeyPressed(KeyCode key);
    [[nodiscard]] bool isKeyToggled(KeyCode key);
    [[nodiscard]] Point getMousePosition();
    void moveMouse(Point delta);

    void browseToFile(const std::filesystem::path& path);

    void refreshMonitors();
    [[nodiscard]] Platform::MonitorDesc getMonitorInfo(Handle monitorHandle, bool allowRefresh);
    [[nodiscard]] Platform::MonitorDesc getPrimaryMonitor(bool allowRefresh);
    [[nodiscard]] Rect getBoundingMonitorArea();
}

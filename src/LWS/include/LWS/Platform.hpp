#pragma once
#include <LWS/Export.hpp>
#include <LWS/interfaces/backends.hpp>
#include <filesystem>

namespace LWS
{
    namespace Platform
    {
        LWS_API void init();
        LWS_API void shutdown();
        LWS_API void runMessageLoop();
        LWS_API bool processMessages();

        LWS_API bool isKeyPressed(KeyCode key);
        LWS_API bool isKeyToggled(KeyCode key);
        LWS_API Point getMousePosition();
        LWS_API void moveMouse(Point delta);

        LWS_API void browseToFile(const std::filesystem::path& path);
    }
}

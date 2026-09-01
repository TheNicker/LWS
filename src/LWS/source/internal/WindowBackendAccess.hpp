#pragma once

#include <LWS/Window.hpp>

namespace LWS::internal
{
    class WindowBackendAccess
    {
      public:

        [[nodiscard]] static IWindowBackend* Get(Window& window) { return window.impl_.get(); }
    };
}  // namespace LWS::internal

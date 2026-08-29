#pragma once

#include <functional>

struct wl_display;

namespace LWS::Wayland
{
    [[nodiscard]] wl_display* GetDisplay();
    void PostTask(std::move_only_function<void()> task);
}  // namespace LWS::Wayland

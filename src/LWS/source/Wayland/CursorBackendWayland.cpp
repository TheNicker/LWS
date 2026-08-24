#include <LWS/Wayland/CursorBackendWayland.hpp>

#ifdef LWS_PLATFORM_WAYLAND

    #include "internal/PlatformState.hpp"

    #include <wayland-client.h>

    #ifdef LWS_HAS_WAYLAND_CURSOR
        #include <wayland-cursor.h>
    #endif

namespace
{
    const char* cursorName(LWS::CursorShape shape)
    {
        using LWS::CursorShape;
        switch (shape)
        {
            case CursorShape::Arrow:
                return "left_ptr";
            case CursorShape::Hand:
                return "pointer";
            case CursorShape::IBeam:
                return "text";
            case CursorShape::SizeNS:
                return "ns-resize";
            case CursorShape::SizeEW:
                return "ew-resize";
            case CursorShape::SizeNWSE:
                return "nwse-resize";
            case CursorShape::SizeNESW:
                return "nesw-resize";
            case CursorShape::SizeAll:
                return "move";
            case CursorShape::No:
                return "not-allowed";
            case CursorShape::Wait:
                return "wait";
            case CursorShape::AppStarting:
                return "progress";
        }
        return "left_ptr";
    }
}  // namespace

namespace LWS
{
    class CursorBackendWayland::NativeState
    {
      public:

        ~NativeState()
        {
            if (surface != nullptr)
                wl_surface_destroy(surface);
    #ifdef LWS_HAS_WAYLAND_CURSOR
            if (theme != nullptr)
                wl_cursor_theme_destroy(theme);
    #endif
        }

        wl_surface* surface = nullptr;
    #ifdef LWS_HAS_WAYLAND_CURSOR
        wl_cursor_theme* theme = nullptr;
        wl_cursor* cursor = nullptr;
    #endif
    };

    CursorBackendWayland::CursorBackendWayland() : fNativeState(std::make_unique<NativeState>())
    {
        auto& platform = internal::WaylandPlatformState::current();
        if (platform.isInitialized())
        {
            fNativeState->surface = wl_compositor_create_surface(platform.compositor());
    #ifdef LWS_HAS_WAYLAND_CURSOR
            fNativeState->theme = wl_cursor_theme_load(nullptr, 24, platform.sharedMemory());
    #endif
        }
    }

    CursorBackendWayland::~CursorBackendWayland() = default;

    void CursorBackendWayland::setVisible(bool visible)
    {
        fVisible = visible;
        apply();
    }

    void CursorBackendWayland::setCursorShape(CursorShape shape)
    {
        fShape = shape;
    #ifdef LWS_HAS_WAYLAND_CURSOR
        if (fNativeState->theme != nullptr)
        {
            fNativeState->cursor = wl_cursor_theme_get_cursor(fNativeState->theme, cursorName(shape));
        }
    #endif
        apply();
    }

    void CursorBackendWayland::setCustomCursor(const BitmapBuffer&)
    {
        // Custom cursor buffers require explicit hotspot and lifetime information, which
        // BitmapBuffer does not currently carry. Keep the current named cursor intact.
    }

    void CursorBackendWayland::apply()
    {
        auto& platform = internal::WaylandPlatformState::current();
        wl_pointer* pointer = platform.pointer();
        if (pointer == nullptr || platform.pointerSerial() == 0)
        {
            return;
        }
        if (!fVisible)
        {
            wl_pointer_set_cursor(pointer, platform.pointerSerial(), nullptr, 0, 0);
            return;
        }

    #ifdef LWS_HAS_WAYLAND_CURSOR
        if (fNativeState->theme == nullptr || fNativeState->surface == nullptr)
        {
            return;
        }
        fNativeState->cursor = wl_cursor_theme_get_cursor(fNativeState->theme, cursorName(fShape));
        if (fNativeState->cursor == nullptr || fNativeState->cursor->image_count == 0)
        {
            return;
        }

        wl_cursor_image* image = fNativeState->cursor->images[0];
        wl_buffer* buffer = wl_cursor_image_get_buffer(image);
        if (buffer != nullptr)
        {
            wl_pointer_set_cursor(pointer, platform.pointerSerial(), fNativeState->surface,
                                  static_cast<int32_t>(image->hotspot_x), static_cast<int32_t>(image->hotspot_y));
            wl_surface_attach(fNativeState->surface, buffer, 0, 0);
            wl_surface_damage(fNativeState->surface, 0, 0, static_cast<int32_t>(image->width),
                              static_cast<int32_t>(image->height));
            wl_surface_commit(fNativeState->surface);
        }
    #endif
    }
}  // namespace LWS

#endif

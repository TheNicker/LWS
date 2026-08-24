#include <LWS/Wayland/WindowBackendWayland.hpp>
#include <LWS/Wayland/CursorBackendWayland.hpp>

#ifdef LWS_PLATFORM_WAYLAND

    #include "internal/PlatformState.hpp"

    #include <algorithm>
    #include <cerrno>
    #include <cstdint>
    #include <limits>
    #include <sys/mman.h>
    #include <sys/syscall.h>
    #include <unistd.h>

    #include <wayland-client.h>
    #include <xdg-shell-client-protocol.h>

namespace
{
    constexpr int32_t captionHeight = 32;
    constexpr int32_t closeButtonWidth = 46;

    int createAnonymousFile()
    {
    #ifdef SYS_memfd_create
        return static_cast<int>(syscall(SYS_memfd_create, "lws-wayland-buffer", 0x0001U));
    #else
        errno = ENOSYS;
        return -1;
    #endif
    }
}  // namespace

namespace LWS
{
    class WindowBackendWayland::NativeState
    {
      public:

        explicit NativeState(WindowBackendWayland& window) : owner(window) {}

        ~NativeState() { releaseBuffer(); }

        void releaseBuffer()
        {
            if (buffer != nullptr)
            {
                wl_buffer_destroy(buffer);
                buffer = nullptr;
            }
            if (mapping != MAP_FAILED)
            {
                munmap(mapping, mappingSize);
                mapping = MAP_FAILED;
                mappingSize = 0;
            }
            bufferReleased = true;
        }

        static void surfaceConfigure(void* data, xdg_surface* surface, uint32_t serial)
        {
            auto& state = *static_cast<NativeState*>(data);
            xdg_surface_ack_configure(surface, serial);
            state.configured = true;
            state.owner.paintBackground();
        }

        static void toplevelConfigure(void* data, xdg_toplevel*, int32_t width, int32_t height, wl_array* states)
        {
            auto& state = *static_cast<NativeState*>(data);
            auto& owner = state.owner;
            const Size previousSize = owner.fSize;
            if (width > 0 && height > 0)
            {
                owner.fSize = {width, height - (owner.usesClientSideDecorations() ? captionHeight : 0)};
                owner.fSize.y = std::max(owner.fSize.y, 1);
            }

            bool maximized = false;
            bool fullscreen = false;
            const auto* item = static_cast<const uint32_t*>(states->data);
            const auto* end = item + states->size / sizeof(uint32_t);
            for (; item != end; ++item)
            {
                maximized |= *item == XDG_TOPLEVEL_STATE_MAXIMIZED;
                fullscreen |= *item == XDG_TOPLEVEL_STATE_FULLSCREEN;
            }

            const WindowDisplayState displayState = maximized ? WindowDisplayState::Maximized
                                                              : WindowDisplayState::Restored;
            if (displayState != owner.fDisplayState)
            {
                owner.fDisplayState = displayState;
                owner.dispatchEvent(EventDisplayStateChanged{displayState});
            }
            owner.fFullScreen = fullscreen;
            if (fullscreen)
            {
                owner.fFullScreenState = FullScreenState::SingleScreen;
            }
            else if (owner.fFullScreenState != FullScreenState::None)
            {
                owner.fFullScreenState = FullScreenState::Windowed;
            }

            if (owner.fSize != previousSize)
            {
                owner.dispatchEvent(EventResize{owner.fSize});
            }
        }

        static void toplevelClose(void* data, xdg_toplevel*)
        {
            auto& owner = static_cast<NativeState*>(data)->owner;
            if (!owner.dispatchEvent(EventClose{}))
            {
                owner.destroy();
            }
        }

        static void toplevelConfigureBounds(void*, xdg_toplevel*, int32_t, int32_t) {}
        static void toplevelWmCapabilities(void*, xdg_toplevel*, wl_array*) {}
        static void decorationConfigure(void*, zxdg_toplevel_decoration_v1*, uint32_t) {}

        static void bufferRelease(void* data, wl_buffer*)
        {
            auto& state = *static_cast<NativeState*>(data);
            state.bufferReleased = true;
            if (state.repaintPending)
            {
                state.repaintPending = false;
                state.owner.paintBackground();
            }
        }

        WindowBackendWayland& owner;
        wl_surface* surface = nullptr;
        xdg_surface* shellSurface = nullptr;
        xdg_toplevel* toplevel = nullptr;
        zxdg_toplevel_decoration_v1* decoration = nullptr;
        wl_buffer* buffer = nullptr;
        void* mapping = MAP_FAILED;
        size_t mappingSize = 0;
        bool configured = false;
        bool bufferReleased = true;
        bool repaintPending = false;
    };

    WindowBackendWayland::WindowBackendWayland() = default;

    WindowBackendWayland::~WindowBackendWayland()
    {
        destroy();
    }

    Result WindowBackendWayland::create(const WindowConfig& config)
    {
        if (fNativeState != nullptr)
        {
            return Result::AlreadyCreated;
        }

        auto& platform = internal::WaylandPlatformState::current();
        if (!platform.isInitialized())
        {
            return Result::PlatformNotInitialized;
        }

        fTitle = config.title;
        fSize = config.size;
        fMinSize = config.minSize;
        fMaxSize = config.maxSize;
        fVisible = config.visible;
        fAlwaysOnTop = config.alwaysOnTop;
        fTransparent = config.transparent;
        fEraseBackground = config.eraseBackground;
        fWindowStyles = config.styles;
        fDisplayState = config.displayState;

        auto native = std::make_unique<NativeState>(*this);
        native->surface = wl_compositor_create_surface(platform.compositor());
        if (native->surface == nullptr)
        {
            return Result::Failure;
        }

        native->shellSurface = xdg_wm_base_get_xdg_surface(platform.shell(), native->surface);
        native->toplevel = native->shellSurface != nullptr ? xdg_surface_get_toplevel(native->shellSurface) : nullptr;
        if (native->shellSurface == nullptr || native->toplevel == nullptr)
        {
            if (native->toplevel != nullptr)
                xdg_toplevel_destroy(native->toplevel);
            if (native->shellSurface != nullptr)
                xdg_surface_destroy(native->shellSurface);
            wl_surface_destroy(native->surface);
            native->surface = nullptr;
            return Result::Failure;
        }

        static constexpr xdg_surface_listener surfaceListener{.configure = NativeState::surfaceConfigure};
        static constexpr xdg_toplevel_listener toplevelListener{
            .configure = NativeState::toplevelConfigure,
            .close = NativeState::toplevelClose,
            .configure_bounds = NativeState::toplevelConfigureBounds,
            .wm_capabilities = NativeState::toplevelWmCapabilities,
        };
        xdg_surface_add_listener(native->shellSurface, &surfaceListener, native.get());
        xdg_toplevel_add_listener(native->toplevel, &toplevelListener, native.get());

        fWlSurface = native->surface;
        fXdgSurface = native->shellSurface;
        fXdgToplevel = native->toplevel;
        fNativeState = std::move(native);
        platform.registerWindow(static_cast<wl_surface*>(fWlSurface), *this);

        if (platform.decorationManager() != nullptr)
        {
            fNativeState->decoration = zxdg_decoration_manager_v1_get_toplevel_decoration(platform.decorationManager(),
                                                                                          fNativeState->toplevel);
            static constexpr zxdg_toplevel_decoration_v1_listener decorationListener{
                .configure = NativeState::decorationConfigure,
            };
            zxdg_toplevel_decoration_v1_add_listener(fNativeState->decoration, &decorationListener, fNativeState.get());
            setWindowStyles(WindowStyle::NoStyle, true);
        }

        setTitle(fTitle);
        setMinMaxSize(fMinSize, fMaxSize);
        setDisplayState(fDisplayState);
        wl_surface_commit(static_cast<wl_surface*>(fWlSurface));
        return Result::Success;
    }

    void WindowBackendWayland::destroy()
    {
        if (fNativeState == nullptr)
        {
            return;
        }

        auto native = std::move(fNativeState);
        internal::WaylandPlatformState::current().unregisterWindow(native->surface);
        native->releaseBuffer();
        if (native->decoration != nullptr)
            zxdg_toplevel_decoration_v1_destroy(native->decoration);
        xdg_toplevel_destroy(native->toplevel);
        xdg_surface_destroy(native->shellSurface);
        wl_surface_destroy(native->surface);
        fXdgToplevel = nullptr;
        fXdgSurface = nullptr;
        fWlSurface = nullptr;
        fVisible = false;
        dispatchEvent(EventWindowDestroyed{});
    }

    void WindowBackendWayland::show()
    {
        if (fNativeState != nullptr && !fVisible)
        {
            fVisible = true;
            if (fNativeState->configured)
            {
                paintBackground();
            }
            else
            {
                wl_surface_commit(fNativeState->surface);
            }
        }
    }

    void WindowBackendWayland::hide()
    {
        if (fNativeState != nullptr && fVisible)
        {
            fVisible = false;
            wl_surface_attach(fNativeState->surface, nullptr, 0, 0);
            wl_surface_commit(fNativeState->surface);
            fNativeState->configured = false;
        }
    }

    bool WindowBackendWayland::getVisible() const
    {
        return fVisible;
    }

    void WindowBackendWayland::setDisplayState(WindowDisplayState state)
    {
        fDisplayState = state;
        if (fNativeState == nullptr)
        {
            return;
        }

        switch (state)
        {
            case WindowDisplayState::Minimized:
                xdg_toplevel_set_minimized(fNativeState->toplevel);
                break;
            case WindowDisplayState::Maximized:
                xdg_toplevel_set_maximized(fNativeState->toplevel);
                break;
            case WindowDisplayState::Restored:
                xdg_toplevel_unset_maximized(fNativeState->toplevel);
                break;
        }
    }

    WindowDisplayState WindowBackendWayland::getDisplayState() const
    {
        return fDisplayState;
    }

    void WindowBackendWayland::setTitle(const LWS::string_type& title)
    {
        fTitle = title;
        if (fNativeState != nullptr)
        {
            xdg_toplevel_set_title(fNativeState->toplevel, fTitle.c_str());
        }
    }

    LWS::string_type WindowBackendWayland::getTitle() const
    {
        return fTitle;
    }
    void WindowBackendWayland::setWindowIcon(const std::filesystem::path&) {}
    void WindowBackendWayland::setPosition(Point) {}
    Point WindowBackendWayland::getPosition() const
    {
        return {};
    }

    void WindowBackendWayland::setSize(Size size)
    {
        if (size.x > 0 && size.y > 0 && size != fSize)
        {
            fSize = size;
            paintBackground();
        }
    }

    Size WindowBackendWayland::getClientSize() const
    {
        return fSize;
    }
    Rect WindowBackendWayland::getClientRect() const
    {
        return {{0, 0}, fSize};
    }
    Size WindowBackendWayland::getWindowSize() const
    {
        return fSize;
    }

    void WindowBackendWayland::setPlacement(const WindowPlacement& placement)
    {
        setSize(placement.size);
        setDisplayState(placement.displayState);
    }

    WindowPlacement WindowBackendWayland::getPlacement() const
    {
        return {{}, fSize, fDisplayState};
    }

    void WindowBackendWayland::setMinMaxSize(Size minSize, Size maxSize)
    {
        fMinSize = minSize;
        fMaxSize = maxSize;
        if (fNativeState != nullptr)
        {
            xdg_toplevel_set_min_size(fNativeState->toplevel, std::max(minSize.x, 0), std::max(minSize.y, 0));
            xdg_toplevel_set_max_size(fNativeState->toplevel, std::max(maxSize.x, 0), std::max(maxSize.y, 0));
        }
    }

    Size WindowBackendWayland::getMinSize() const
    {
        return fMinSize;
    }
    Size WindowBackendWayland::getMaxSize() const
    {
        return fMaxSize;
    }

    void WindowBackendWayland::setWindowStyles(WindowStyle styles, bool enable)
    {
        const auto current = std::to_underlying(fWindowStyles);
        fWindowStyles = static_cast<WindowStyle>(enable ? current | std::to_underlying(styles)
                                                        : current & ~std::to_underlying(styles));
        if (fNativeState != nullptr && fNativeState->decoration != nullptr)
        {
            const bool wantsCaption = (std::to_underlying(fWindowStyles) & std::to_underlying(WindowStyle::Caption)) !=
                                      0;
            if (wantsCaption)
            {
                zxdg_toplevel_decoration_v1_set_mode(fNativeState->decoration,
                                                     ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
            }
            else
            {
                zxdg_toplevel_decoration_v1_unset_mode(fNativeState->decoration);
            }
        }
    }

    WindowStyle WindowBackendWayland::getWindowStyles() const
    {
        return fWindowStyles;
    }
    void WindowBackendWayland::setForeground() {}
    void WindowBackendWayland::setFocus() {}
    bool WindowBackendWayland::isInFocus() const
    {
        return fFocused;
    }
    void WindowBackendWayland::setAlwaysOnTop(bool onTop)
    {
        fAlwaysOnTop = onTop;
    }
    bool WindowBackendWayland::getAlwaysOnTop() const
    {
        return fAlwaysOnTop;
    }

    void WindowBackendWayland::setTransparent(bool transparent)
    {
        if (fTransparent != transparent)
        {
            fTransparent = transparent;
            paintBackground();
        }
    }

    bool WindowBackendWayland::getTransparent() const
    {
        return fTransparent;
    }

    void WindowBackendWayland::setBackgroundColor(LLUtils::Color color)
    {
        fBackgroundColor = color;
        paintBackground();
    }

    LLUtils::Color WindowBackendWayland::getBackgroundColor() const
    {
        return fBackgroundColor;
    }
    void WindowBackendWayland::setEraseBackground(bool erase)
    {
        fEraseBackground = erase;
    }
    bool WindowBackendWayland::getEraseBackground() const
    {
        return fEraseBackground;
    }

    void WindowBackendWayland::setFullScreenState(FullScreenState state)
    {
        fFullScreenState = state;
        fFullScreen = state != FullScreenState::None && state != FullScreenState::Windowed;
        if (fNativeState != nullptr)
        {
            if (fFullScreen)
            {
                xdg_toplevel_set_fullscreen(fNativeState->toplevel, nullptr);
            }
            else
            {
                xdg_toplevel_unset_fullscreen(fNativeState->toplevel);
            }
        }
    }

    FullScreenState WindowBackendWayland::getFullScreenState() const
    {
        return fFullScreenState;
    }
    bool WindowBackendWayland::isFullScreen() const
    {
        return fFullScreen;
    }

    void WindowBackendWayland::toggleFullScreen(bool)
    {
        setFullScreenState(fFullScreen ? FullScreenState::Windowed : FullScreenState::SingleScreen);
    }

    bool WindowBackendWayland::isMouseInClientRect() const
    {
        return fMouseInside;
    }
    bool WindowBackendWayland::isUnderMouseCursor() const
    {
        return fMouseInside;
    }
    Point WindowBackendWayland::getMousePosition() const
    {
        return fMousePosition;
    }
    void WindowBackendWayland::setLockMouseToWindowMode(LockMouseToWindowMode mode)
    {
        fLockMode = mode;
    }
    LockMouseToWindowMode WindowBackendWayland::getLockMouseToWindowMode() const
    {
        return fLockMode;
    }
    void WindowBackendWayland::setDoubleClickMode(DoubleClickMode mode)
    {
        fDoubleClickMode = mode;
    }
    DoubleClickMode WindowBackendWayland::getDoubleClickMode() const
    {
        return fDoubleClickMode;
    }
    void WindowBackendWayland::setCursor(std::shared_ptr<ICursorBackend> cursor)
    {
        fCursor = std::move(cursor);
        if (auto* waylandCursor = dynamic_cast<CursorBackendWayland*>(fCursor.get()); waylandCursor != nullptr)
        {
            waylandCursor->apply();
        }
    }

    void WindowBackendWayland::setParent(IWindowBackend* parent)
    {
        auto* waylandParent = dynamic_cast<WindowBackendWayland*>(parent);
        if (fNativeState != nullptr)
        {
            xdg_toplevel_set_parent(fNativeState->toplevel,
                                    waylandParent != nullptr && waylandParent->fNativeState != nullptr
                                        ? waylandParent->fNativeState->toplevel
                                        : nullptr);
        }
    }

    Result WindowBackendWayland::enableDragAndDrop(bool)
    {
        return Result::NotSupported;
    }

    EventListenerToken WindowBackendWayland::addListener(EventCallback callback)
    {
        const EventListenerToken token = fNextListenerToken++;
        fListeners.emplace_back(token, std::move(callback));
        return token;
    }

    void WindowBackendWayland::removeListener(EventListenerToken token)
    {
        std::erase_if(fListeners, [token](const auto& listener) { return listener.first == token; });
    }

    void WindowBackendWayland::injectRawEvent(void* platformEvent)
    {
        dispatchEvent(EventRawPlatform{std::to_underlying(BackendId::Wayland), platformEvent});
    }

    Handle WindowBackendWayland::getHandle() const
    {
        return reinterpret_cast<Handle>(fWlSurface);
    }

    void WindowBackendWayland::handlePointerEnter(Point position)
    {
        fMouseInside = true;
        fMousePosition = position;
        if (auto* waylandCursor = dynamic_cast<CursorBackendWayland*>(fCursor.get()); waylandCursor != nullptr)
        {
            waylandCursor->apply();
        }
    }

    void WindowBackendWayland::handlePointerLeave()
    {
        fMouseInside = false;
    }

    void WindowBackendWayland::handlePointerMotion(Point position, Point delta)
    {
        if (usesClientSideDecorations() && position.y < captionHeight)
        {
            fMousePosition = position;
            return;
        }
        if (usesClientSideDecorations())
        {
            position.y -= captionHeight;
        }
        fMousePosition = position;
        dispatchEvent(EventMouseMove{position, delta});
    }

    void WindowBackendWayland::handlePointerButton(MouseButton button, bool pressed, Point position)
    {
        if (usesClientSideDecorations() && button == MouseButton::Left && pressed && position.y < captionHeight)
        {
            if (isCloseButton(position))
            {
                if (!dispatchEvent(EventClose{}))
                    destroy();
            }
            else if (fNativeState != nullptr)
            {
                auto& platform = internal::WaylandPlatformState::current();
                xdg_toplevel_move(fNativeState->toplevel, platform.seat(), platform.pointerSerial());
            }
            return;
        }
        if (usesClientSideDecorations())
        {
            position.y -= captionHeight;
        }
        fMousePosition = position;
        dispatchEvent(EventMouseButton{button, pressed, position});
    }

    void WindowBackendWayland::handlePointerWheel(int32_t delta, Point position)
    {
        dispatchEvent(EventMouseWheel{delta, position});
    }

    void WindowBackendWayland::handleKeyboardFocus(bool focused)
    {
        if (fFocused != focused)
        {
            fFocused = focused;
            dispatchEvent(focused ? AnyEvent{EventFocusGained{}} : AnyEvent{EventFocusLost{}});
        }
    }

    void WindowBackendWayland::handleKey(KeyCode key, bool pressed)
    {
        if (key != KeyCode::Unknown)
        {
            dispatchEvent(pressed ? AnyEvent{EventKeyDown{key}} : AnyEvent{EventKeyUp{key}});
        }
    }

    void WindowBackendWayland::setAppId(const std::string& appId)
    {
        if (fNativeState != nullptr)
        {
            xdg_toplevel_set_app_id(fNativeState->toplevel, appId.c_str());
        }
    }

    bool WindowBackendWayland::dispatchEvent(const AnyEvent& event)
    {
        std::vector<EventListenerToken> tokens;
        tokens.reserve(fListeners.size());
        for (const auto& [token, callback] : fListeners)
        {
            tokens.push_back(token);
        }

        bool handled = false;
        for (EventListenerToken token : tokens)
        {
            const auto item = std::ranges::find(fListeners, token, &decltype(fListeners)::value_type::first);
            if (item != fListeners.end() && item->second(event))
            {
                handled = true;
                break;
            }
        }
        return handled;
    }

    bool WindowBackendWayland::usesClientSideDecorations() const
    {
        return internal::WaylandPlatformState::current().decorationManager() == nullptr &&
               (std::to_underlying(fWindowStyles) & std::to_underlying(WindowStyle::Caption)) != 0;
    }

    bool WindowBackendWayland::isCloseButton(Point position) const
    {
        return (std::to_underlying(fWindowStyles) & std::to_underlying(WindowStyle::CloseButton)) != 0 &&
               position.x >= fSize.x - closeButtonWidth && position.x < fSize.x && position.y >= 0 &&
               position.y < captionHeight;
    }

    void WindowBackendWayland::paintBackground()
    {
        if (fNativeState == nullptr || !fVisible || !fNativeState->configured || !fEraseBackground || fSize.x <= 0 ||
            fSize.y <= 0)
        {
            return;
        }
        if (!fNativeState->bufferReleased)
        {
            fNativeState->repaintPending = true;
            return;
        }

        fNativeState->releaseBuffer();
        constexpr size_t bytesPerPixel = 4;
        const size_t width = static_cast<size_t>(fSize.x);
        const int32_t bufferHeight = fSize.y + (usesClientSideDecorations() ? captionHeight : 0);
        const size_t height = static_cast<size_t>(bufferHeight);
        if (width > std::numeric_limits<size_t>::max() / bytesPerPixel / height)
        {
            return;
        }
        const size_t stride = width * bytesPerPixel;
        const size_t bufferSize = stride * height;
        if (bufferSize > static_cast<size_t>(std::numeric_limits<off_t>::max()))
        {
            return;
        }

        const int fd = createAnonymousFile();
        if (fd < 0 || ftruncate(fd, static_cast<off_t>(bufferSize)) != 0)
        {
            if (fd >= 0)
                close(fd);
            return;
        }

        void* mapping = mmap(nullptr, bufferSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (mapping == MAP_FAILED)
        {
            close(fd);
            return;
        }

        wl_shm_pool* pool = wl_shm_create_pool(internal::WaylandPlatformState::current().sharedMemory(), fd,
                                               static_cast<int32_t>(bufferSize));
        wl_buffer* buffer = pool != nullptr
                                ? wl_shm_pool_create_buffer(pool, 0, fSize.x, bufferHeight,
                                                            static_cast<int32_t>(stride), WL_SHM_FORMAT_ARGB8888)
                                : nullptr;
        if (pool != nullptr)
            wl_shm_pool_destroy(pool);
        close(fd);
        if (buffer == nullptr)
        {
            munmap(mapping, bufferSize);
            return;
        }

        const uint32_t alpha = fTransparent ? fBackgroundColor.A() : 0xffU;
        const uint32_t pixel = alpha << 24U | static_cast<uint32_t>(fBackgroundColor.R()) << 16U |
                               static_cast<uint32_t>(fBackgroundColor.G()) << 8U | fBackgroundColor.B();
        std::fill_n(static_cast<uint32_t*>(mapping), width * height, pixel);
        if (usesClientSideDecorations())
        {
            auto* pixels = static_cast<uint32_t*>(mapping);
            constexpr uint32_t captionColor = 0xff303030U;
            constexpr uint32_t closeColor = 0xffc42b1cU;
            constexpr uint32_t glyphColor = 0xffffffffU;
            std::fill_n(pixels, width * captionHeight, captionColor);
            if ((std::to_underlying(fWindowStyles) & std::to_underlying(WindowStyle::CloseButton)) != 0)
            {
                const int32_t closeLeft = std::max(fSize.x - closeButtonWidth, 0);
                for (int32_t y = 0; y < captionHeight; ++y)
                    std::fill_n(pixels + static_cast<size_t>(y) * width + closeLeft, fSize.x - closeLeft, closeColor);
                const int32_t centerX = closeLeft + (fSize.x - closeLeft) / 2;
                const int32_t centerY = captionHeight / 2;
                for (int32_t offset = -5; offset <= 5; ++offset)
                {
                    pixels[static_cast<size_t>(centerY + offset) * width + centerX + offset] = glyphColor;
                    pixels[static_cast<size_t>(centerY + offset) * width + centerX - offset] = glyphColor;
                }
            }
            xdg_surface_set_window_geometry(fNativeState->shellSurface, 0, 0, fSize.x, bufferHeight);
        }

        fNativeState->buffer = buffer;
        fNativeState->mapping = mapping;
        fNativeState->mappingSize = bufferSize;
        fNativeState->bufferReleased = false;
        static constexpr wl_buffer_listener bufferListener{.release = NativeState::bufferRelease};
        wl_buffer_add_listener(buffer, &bufferListener, fNativeState.get());
        wl_surface_attach(fNativeState->surface, buffer, 0, 0);
        if (wl_surface_get_version(fNativeState->surface) >= WL_SURFACE_DAMAGE_BUFFER_SINCE_VERSION)
        {
            wl_surface_damage_buffer(fNativeState->surface, 0, 0, fSize.x, bufferHeight);
        }
        else
        {
            wl_surface_damage(fNativeState->surface, 0, 0, fSize.x, bufferHeight);
        }
        wl_surface_commit(fNativeState->surface);
        dispatchEvent(EventPaint{});
    }
}  // namespace LWS

#endif

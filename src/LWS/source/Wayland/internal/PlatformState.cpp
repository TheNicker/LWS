#ifdef LWS_PLATFORM_WAYLAND

    #include "PlatformState.hpp"

    #include <LWS/Wayland/WindowBackendWayland.hpp>

    #include <algorithm>
    #include <cerrno>
    #include <cstring>
    #include <linux/input-event-codes.h>
    #include <poll.h>
    #include <ranges>
    #include <sys/eventfd.h>
    #include <tuple>
    #include <unistd.h>

namespace
{
    LWS::KeyCode keyCodeFromLinux(uint32_t key)
    {
        using LWS::KeyCode;
        if (key >= KEY_A && key <= KEY_Z)
        {
            return static_cast<KeyCode>(std::to_underlying(KeyCode::A) + key - KEY_A);
        }
        if (key >= KEY_1 && key <= KEY_9)
        {
            return static_cast<KeyCode>(std::to_underlying(KeyCode::Digit1) + key - KEY_1);
        }
        if (key >= KEY_F1 && key <= KEY_F12)
        {
            return static_cast<KeyCode>(std::to_underlying(KeyCode::F1) + key - KEY_F1);
        }
        if (key >= KEY_KP0 && key <= KEY_KP9)
        {
            return static_cast<KeyCode>(std::to_underlying(KeyCode::Numpad0) + key - KEY_KP0);
        }

        switch (key)
        {
            case KEY_0:
                return KeyCode::Digit0;
            case KEY_LEFT:
                return KeyCode::Left;
            case KEY_RIGHT:
                return KeyCode::Right;
            case KEY_UP:
                return KeyCode::Up;
            case KEY_DOWN:
                return KeyCode::Down;
            case KEY_HOME:
                return KeyCode::Home;
            case KEY_END:
                return KeyCode::End;
            case KEY_PAGEUP:
                return KeyCode::PageUp;
            case KEY_PAGEDOWN:
                return KeyCode::PageDown;
            case KEY_INSERT:
                return KeyCode::Insert;
            case KEY_DELETE:
                return KeyCode::Delete;
            case KEY_ENTER:
                return KeyCode::Enter;
            case KEY_ESC:
                return KeyCode::Escape;
            case KEY_TAB:
                return KeyCode::Tab;
            case KEY_BACKSPACE:
                return KeyCode::Backspace;
            case KEY_SPACE:
                return KeyCode::Space;
            case KEY_LEFTSHIFT:
                return KeyCode::LShift;
            case KEY_RIGHTSHIFT:
                return KeyCode::RShift;
            case KEY_LEFTCTRL:
                return KeyCode::LControl;
            case KEY_RIGHTCTRL:
                return KeyCode::RControl;
            case KEY_LEFTALT:
                return KeyCode::LAlt;
            case KEY_RIGHTALT:
                return KeyCode::RAlt;
            case KEY_LEFTMETA:
            case KEY_RIGHTMETA:
                return KeyCode::Win;
            case KEY_CAPSLOCK:
                return KeyCode::CapsLock;
            case KEY_NUMLOCK:
                return KeyCode::NumLock;
            case KEY_SCROLLLOCK:
                return KeyCode::ScrollLock;
            case KEY_KPPLUS:
                return KeyCode::NumpadAdd;
            case KEY_KPMINUS:
                return KeyCode::NumpadSubtract;
            case KEY_KPASTERISK:
                return KeyCode::NumpadMultiply;
            case KEY_KPSLASH:
                return KeyCode::NumpadDivide;
            case KEY_KPDOT:
                return KeyCode::NumpadDecimal;
            case KEY_KPENTER:
                return KeyCode::NumpadEnter;
            case KEY_COMMA:
                return KeyCode::Comma;
            case KEY_DOT:
                return KeyCode::Period;
            case KEY_SLASH:
                return KeyCode::Slash;
            case KEY_SEMICOLON:
                return KeyCode::Semicolon;
            case KEY_APOSTROPHE:
                return KeyCode::Quote;
            case KEY_LEFTBRACE:
                return KeyCode::LeftBracket;
            case KEY_RIGHTBRACE:
                return KeyCode::RightBracket;
            case KEY_BACKSLASH:
                return KeyCode::Backslash;
            case KEY_MINUS:
                return KeyCode::Minus;
            case KEY_EQUAL:
                return KeyCode::Equals;
            case KEY_GRAVE:
                return KeyCode::Tilde;
            case KEY_SYSRQ:
                return KeyCode::PrintScreen;
            case KEY_PAUSE:
                return KeyCode::Pause;
            default:
                return KeyCode::Unknown;
        }
    }

    LWS::MouseButton mouseButtonFromLinux(uint32_t button)
    {
        using LWS::MouseButton;
        switch (button)
        {
            case BTN_LEFT:
                return MouseButton::Left;
            case BTN_MIDDLE:
                return MouseButton::Middle;
            case BTN_RIGHT:
                return MouseButton::Right;
            case BTN_SIDE:
                return MouseButton::X1;
            case BTN_EXTRA:
                return MouseButton::X2;
            default:
                return MouseButton::Left;
        }
    }
}  // namespace

namespace LWS::internal
{
    WaylandPlatformState& WaylandPlatformState::current()
    {
        thread_local WaylandPlatformState state;
        return state;
    }

    Result WaylandPlatformState::initialize()
    {
        if (fInitCount != 0)
        {
            ++fInitCount;
            return Result::Success;
        }

        fDisplay = wl_display_connect(nullptr);
        if (fDisplay == nullptr)
        {
            return Result::Failure;
        }

        fWakeDescriptor = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
        if (fWakeDescriptor < 0)
        {
            releaseObjects();
            return Result::Failure;
        }

        fRegistry = wl_display_get_registry(fDisplay);
        static constexpr wl_registry_listener registryListener{
            .global = registryGlobal,
            .global_remove = registryGlobalRemove,
        };
        wl_registry_add_listener(fRegistry, &registryListener, this);

        const bool registryReady = wl_display_roundtrip(fDisplay) >= 0 && wl_display_roundtrip(fDisplay) >= 0;
        if (!registryReady || fCompositor == nullptr || fSharedMemory == nullptr || fShell == nullptr)
        {
            releaseObjects();
            return Result::NotSupported;
        }

        fQuitRequested = false;
        fInitCount = 1;
        return Result::Success;
    }

    void WaylandPlatformState::shutdown()
    {
        if (fInitCount != 0)
        {
            --fInitCount;
            if (fInitCount == 0)
            {
                while (!fWindows.empty())
                {
                    fWindows.begin()->second->destroy();
                }
                releaseObjects();
            }
        }
    }

    bool WaylandPlatformState::isInitialized() const
    {
        return fInitCount != 0;
    }

    void WaylandPlatformState::runMessageLoop()
    {
        while (!fQuitRequested && fDisplay != nullptr)
        {
            dispatchOnce(-1);
        }
    }

    bool WaylandPlatformState::processMessages()
    {
        dispatchOnce(0);
        return fQuitRequested;
    }

    void WaylandPlatformState::requestQuit()
    {
        fQuitRequested = true;
        if (fWakeDescriptor >= 0)
        {
            const uint64_t value = 1;
            std::ignore = write(fWakeDescriptor, &value, sizeof(value));
        }
    }

    void WaylandPlatformState::postTask(std::move_only_function<void()> task)
    {
        {
            const std::scoped_lock lock(fTaskMutex);
            fTasks.push_back(std::move(task));
        }
        if (fWakeDescriptor >= 0)
        {
            const uint64_t value = 1;
            std::ignore = write(fWakeDescriptor, &value, sizeof(value));
        }
    }

    void WaylandPlatformState::registerWindow(wl_surface* surface, WindowBackendWayland& window)
    {
        fWindows.emplace(surface, &window);
    }

    void WaylandPlatformState::unregisterWindow(wl_surface* surface)
    {
        WindowBackendWayland* window = findWindow(surface);
        if (fPointerWindow == window)
            fPointerWindow = nullptr;
        if (fKeyboardWindow == window)
            fKeyboardWindow = nullptr;
        fWindows.erase(surface);
        if (fWindows.empty())
        {
            requestQuit();
        }
    }

    WindowBackendWayland* WaylandPlatformState::findWindow(wl_surface* surface) const
    {
        const auto it = fWindows.find(surface);
        return it != fWindows.end() ? it->second : nullptr;
    }

    bool WaylandPlatformState::isKeyPressed(KeyCode key) const
    {
        if (key == KeyCode::Shift)
        {
            return isKeyPressed(KeyCode::LShift) || isKeyPressed(KeyCode::RShift);
        }
        if (key == KeyCode::Control)
        {
            return isKeyPressed(KeyCode::LControl) || isKeyPressed(KeyCode::RControl);
        }
        if (key == KeyCode::Alt)
        {
            return isKeyPressed(KeyCode::LAlt) || isKeyPressed(KeyCode::RAlt);
        }
        return fPressedKeys.contains(key);
    }

    Platform::MonitorDesc WaylandPlatformState::monitorInfo(Handle handle) const
    {
        const auto it = std::ranges::find_if(fOutputs, [handle](const Output& output)
                                             { return output.description.handle == handle; });
        return it != fOutputs.end() ? it->description : Platform::MonitorDesc{};
    }

    Platform::MonitorDesc WaylandPlatformState::primaryMonitor() const
    {
        return fOutputs.empty() ? Platform::MonitorDesc{} : fOutputs.front().description;
    }

    Rect WaylandPlatformState::boundingMonitorArea() const
    {
        if (fOutputs.empty())
        {
            return {};
        }

        Point topLeft = fOutputs.front().description.monitorRect.GetCorner(LLUtils::TopLeft);
        Point bottomRight = fOutputs.front().description.monitorRect.GetCorner(LLUtils::BottomRight);
        for (const Output& output : fOutputs | std::views::drop(1))
        {
            const Point outputTopLeft = output.description.monitorRect.GetCorner(LLUtils::TopLeft);
            const Point outputBottomRight = output.description.monitorRect.GetCorner(LLUtils::BottomRight);
            topLeft.x = std::min(topLeft.x, outputTopLeft.x);
            topLeft.y = std::min(topLeft.y, outputTopLeft.y);
            bottomRight.x = std::max(bottomRight.x, outputBottomRight.x);
            bottomRight.y = std::max(bottomRight.y, outputBottomRight.y);
        }
        return {topLeft, bottomRight};
    }

    void WaylandPlatformState::registryGlobal(void* data, wl_registry* registry, uint32_t name, const char* interface,
                                              uint32_t version)
    {
        auto& state = *static_cast<WaylandPlatformState*>(data);
        if (std::strcmp(interface, wl_compositor_interface.name) == 0)
        {
            state.fCompositor = static_cast<wl_compositor*>(
                wl_registry_bind(registry, name, &wl_compositor_interface, std::min(version, 6U)));
        }
        else if (std::strcmp(interface, wl_shm_interface.name) == 0)
        {
            state.fSharedMemory = static_cast<wl_shm*>(
                wl_registry_bind(registry, name, &wl_shm_interface, std::min(version, 1U)));
        }
        else if (std::strcmp(interface, xdg_wm_base_interface.name) == 0)
        {
            state.fShell = static_cast<xdg_wm_base*>(
                wl_registry_bind(registry, name, &xdg_wm_base_interface, std::min(version, 6U)));
            static constexpr xdg_wm_base_listener shellListener{.ping = shellPing};
            xdg_wm_base_add_listener(state.fShell, &shellListener, &state);
        }
        else if (std::strcmp(interface, zxdg_decoration_manager_v1_interface.name) == 0)
        {
            state.fDecorationManager = static_cast<zxdg_decoration_manager_v1*>(
                wl_registry_bind(registry, name, &zxdg_decoration_manager_v1_interface, std::min(version, 1U)));
        }
        else if (std::strcmp(interface, wl_seat_interface.name) == 0 && state.fSeat == nullptr)
        {
            state.fSeat = static_cast<wl_seat*>(
                wl_registry_bind(registry, name, &wl_seat_interface, std::min(version, 9U)));
            static constexpr wl_seat_listener seatListener{
                .capabilities = seatCapabilities,
                .name = seatName,
            };
            wl_seat_add_listener(state.fSeat, &seatListener, &state);
        }
        else if (std::strcmp(interface, wl_output_interface.name) == 0)
        {
            Output output;
            output.registryName = name;
            output.object = static_cast<wl_output*>(
                wl_registry_bind(registry, name, &wl_output_interface, std::min(version, 4U)));
            output.description.handle = reinterpret_cast<Handle>(output.object);
            output.description.primary = state.fOutputs.empty();
            state.fOutputs.push_back(std::move(output));
            static constexpr wl_output_listener outputListener{
                .geometry = outputGeometry,
                .mode = outputMode,
                .done = outputDone,
                .scale = outputScale,
                .name = outputName,
                .description = outputDescription,
            };
            wl_output_add_listener(state.fOutputs.back().object, &outputListener, &state);
        }
    }

    void WaylandPlatformState::registryGlobalRemove(void* data, wl_registry*, uint32_t name)
    {
        auto& state = *static_cast<WaylandPlatformState*>(data);
        const auto it = std::ranges::find(state.fOutputs, name, &Output::registryName);
        if (it != state.fOutputs.end())
        {
            wl_output_destroy(it->object);
            state.fOutputs.erase(it);
            if (!state.fOutputs.empty())
            {
                state.fOutputs.front().description.primary = true;
            }
        }
    }

    void WaylandPlatformState::shellPing(void*, xdg_wm_base* shell, uint32_t serial)
    {
        xdg_wm_base_pong(shell, serial);
    }

    void WaylandPlatformState::seatCapabilities(void* data, wl_seat* seat, uint32_t capabilities)
    {
        auto& state = *static_cast<WaylandPlatformState*>(data);
        if ((capabilities & WL_SEAT_CAPABILITY_POINTER) != 0 && state.fPointer == nullptr)
        {
            state.fPointer = wl_seat_get_pointer(seat);
            static constexpr wl_pointer_listener pointerListener{
                .enter = pointerEnter,
                .leave = pointerLeave,
                .motion = pointerMotion,
                .button = pointerButton,
                .axis = pointerAxis,
                .frame = pointerFrame,
                .axis_source = pointerAxisSource,
                .axis_stop = pointerAxisStop,
                .axis_discrete = pointerAxisDiscrete,
                .axis_value120 = pointerAxisValue120,
                .axis_relative_direction = pointerAxisRelativeDirection,
    #ifdef WL_POINTER_WARP_SINCE_VERSION
                .warp = pointerWarp,
    #endif
            };
            wl_pointer_add_listener(state.fPointer, &pointerListener, &state);
        }
        else if ((capabilities & WL_SEAT_CAPABILITY_POINTER) == 0 && state.fPointer != nullptr)
        {
            wl_pointer_release(state.fPointer);
            state.fPointer = nullptr;
            state.fPointerWindow = nullptr;
        }

        if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) != 0 && state.fKeyboard == nullptr)
        {
            state.fKeyboard = wl_seat_get_keyboard(seat);
            static constexpr wl_keyboard_listener keyboardListener{
                .keymap = keyboardKeymap,
                .enter = keyboardEnter,
                .leave = keyboardLeave,
                .key = keyboardKey,
                .modifiers = keyboardModifiers,
                .repeat_info = keyboardRepeatInfo,
            };
            wl_keyboard_add_listener(state.fKeyboard, &keyboardListener, &state);
        }
        else if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) == 0 && state.fKeyboard != nullptr)
        {
            wl_keyboard_release(state.fKeyboard);
            state.fKeyboard = nullptr;
            state.fKeyboardWindow = nullptr;
            state.fPressedKeys.clear();
        }
    }

    void WaylandPlatformState::seatName(void*, wl_seat*, const char*) {}

    void WaylandPlatformState::pointerEnter(void* data, wl_pointer*, uint32_t serial, wl_surface* surface, wl_fixed_t x,
                                            wl_fixed_t y)
    {
        auto& state = *static_cast<WaylandPlatformState*>(data);
        state.fPointerSerial = serial;
        state.fPointerPosition = {wl_fixed_to_int(x), wl_fixed_to_int(y)};
        state.fPointerWindow = state.findWindow(surface);
        if (state.fPointerWindow != nullptr)
        {
            state.fPointerWindow->handlePointerEnter(state.fPointerPosition);
        }
    }

    void WaylandPlatformState::pointerLeave(void* data, wl_pointer*, uint32_t, wl_surface*)
    {
        auto& state = *static_cast<WaylandPlatformState*>(data);
        if (state.fPointerWindow != nullptr)
        {
            state.fPointerWindow->handlePointerLeave();
            state.fPointerWindow = nullptr;
        }
    }

    void WaylandPlatformState::pointerMotion(void* data, wl_pointer*, uint32_t, wl_fixed_t x, wl_fixed_t y)
    {
        auto& state = *static_cast<WaylandPlatformState*>(data);
        const Point position{wl_fixed_to_int(x), wl_fixed_to_int(y)};
        const Point delta{position.x - state.fPointerPosition.x, position.y - state.fPointerPosition.y};
        state.fPointerPosition = position;
        if (state.fPointerWindow != nullptr)
        {
            state.fPointerWindow->handlePointerMotion(position, delta);
        }
    }

    void WaylandPlatformState::pointerButton(void* data, wl_pointer*, uint32_t serial, uint32_t, uint32_t button,
                                             uint32_t buttonState)
    {
        auto& state = *static_cast<WaylandPlatformState*>(data);
        state.fPointerSerial = serial;
        if (state.fPointerWindow != nullptr)
        {
            state.fPointerWindow->handlePointerButton(mouseButtonFromLinux(button),
                                                      buttonState == WL_POINTER_BUTTON_STATE_PRESSED,
                                                      state.fPointerPosition);
        }
    }

    void WaylandPlatformState::pointerAxis(void* data, wl_pointer*, uint32_t, uint32_t axis, wl_fixed_t value)
    {
        auto& state = *static_cast<WaylandPlatformState*>(data);
        if (state.fPointerWindow != nullptr && axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
        {
            state.fPointerWindow->handlePointerWheel(-wl_fixed_to_int(value) * 12, state.fPointerPosition);
        }
    }

    void WaylandPlatformState::pointerFrame(void*, wl_pointer*) {}
    void WaylandPlatformState::pointerAxisSource(void*, wl_pointer*, uint32_t) {}
    void WaylandPlatformState::pointerAxisStop(void*, wl_pointer*, uint32_t, uint32_t) {}
    void WaylandPlatformState::pointerAxisDiscrete(void*, wl_pointer*, uint32_t, int32_t) {}
    void WaylandPlatformState::pointerAxisValue120(void*, wl_pointer*, uint32_t, int32_t) {}
    void WaylandPlatformState::pointerAxisRelativeDirection(void*, wl_pointer*, uint32_t, uint32_t) {}
    #ifdef WL_POINTER_WARP_SINCE_VERSION
    void WaylandPlatformState::pointerWarp(void* data, wl_pointer*, wl_fixed_t x, wl_fixed_t y)
    {
        pointerMotion(data, nullptr, 0, x, y);
    }
    #endif

    void WaylandPlatformState::keyboardKeymap(void*, wl_keyboard*, uint32_t, int32_t fd, uint32_t)
    {
        close(fd);
    }

    void WaylandPlatformState::keyboardEnter(void* data, wl_keyboard*, uint32_t, wl_surface* surface, wl_array*)
    {
        auto& state = *static_cast<WaylandPlatformState*>(data);
        state.fKeyboardWindow = state.findWindow(surface);
        if (state.fKeyboardWindow != nullptr)
        {
            state.fKeyboardWindow->handleKeyboardFocus(true);
        }
    }

    void WaylandPlatformState::keyboardLeave(void* data, wl_keyboard*, uint32_t, wl_surface*)
    {
        auto& state = *static_cast<WaylandPlatformState*>(data);
        if (state.fKeyboardWindow != nullptr)
        {
            state.fKeyboardWindow->handleKeyboardFocus(false);
            state.fKeyboardWindow = nullptr;
        }
        state.fPressedKeys.clear();
    }

    void WaylandPlatformState::keyboardKey(void* data, wl_keyboard*, uint32_t, uint32_t, uint32_t key,
                                           uint32_t keyState)
    {
        auto& state = *static_cast<WaylandPlatformState*>(data);
        const KeyCode translated = keyCodeFromLinux(key);
        const bool pressed = keyState == WL_KEYBOARD_KEY_STATE_PRESSED;
        if (translated != KeyCode::Unknown)
        {
            if (pressed)
            {
                state.fPressedKeys.insert(translated);
            }
            else
            {
                state.fPressedKeys.erase(translated);
            }
        }
        if (state.fKeyboardWindow != nullptr)
        {
            state.fKeyboardWindow->handleKey(translated, pressed);
        }
    }

    void WaylandPlatformState::keyboardModifiers(void*, wl_keyboard*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)
    {
    }
    void WaylandPlatformState::keyboardRepeatInfo(void*, wl_keyboard*, int32_t, int32_t) {}

    WaylandPlatformState::Output* WaylandPlatformState::findOutput(wl_output* output)
    {
        const auto it = std::ranges::find(fOutputs, output, &Output::object);
        return it != fOutputs.end() ? &*it : nullptr;
    }

    void WaylandPlatformState::outputGeometry(void* data, wl_output* output, int32_t x, int32_t y, int32_t, int32_t,
                                              int32_t, const char*, const char*, int32_t)
    {
        auto& state = *static_cast<WaylandPlatformState*>(data);
        if (Output* item = state.findOutput(output); item != nullptr)
        {
            const Size size{item->description.monitorRect.GetWidth(), item->description.monitorRect.GetHeight()};
            item->description.monitorRect = {{x, y}, {x + size.x, y + size.y}};
            item->description.workRect = item->description.monitorRect;
        }
    }

    void WaylandPlatformState::outputMode(void* data, wl_output* output, uint32_t flags, int32_t width, int32_t height,
                                          int32_t refresh)
    {
        auto& state = *static_cast<WaylandPlatformState*>(data);
        if ((flags & WL_OUTPUT_MODE_CURRENT) != 0)
        {
            if (Output* item = state.findOutput(output); item != nullptr)
            {
                const Point position = item->description.monitorRect.GetCorner(LLUtils::TopLeft);
                item->description.monitorRect = {position, {position.x + width, position.y + height}};
                item->description.workRect = item->description.monitorRect;
                item->description.displayFrequency = refresh > 0 ? static_cast<uint32_t>(refresh / 1000) : 0;
            }
        }
    }

    void WaylandPlatformState::outputDone(void*, wl_output*) {}

    void WaylandPlatformState::outputScale(void* data, wl_output* output, int32_t factor)
    {
        auto& state = *static_cast<WaylandPlatformState*>(data);
        if (Output* item = state.findOutput(output); item != nullptr)
        {
            item->scale = std::max(factor, 1);
            item->description.dpiX = 96U * static_cast<uint32_t>(item->scale);
            item->description.dpiY = item->description.dpiX;
        }
    }

    void WaylandPlatformState::outputName(void* data, wl_output* output, const char* name)
    {
        auto& state = *static_cast<WaylandPlatformState*>(data);
        if (Output* item = state.findOutput(output); item != nullptr && name != nullptr)
        {
            item->description.deviceName = name;
        }
    }

    void WaylandPlatformState::outputDescription(void*, wl_output*, const char*) {}

    void WaylandPlatformState::dispatchTasks()
    {
        std::vector<std::move_only_function<void()>> tasks;
        {
            const std::scoped_lock lock(fTaskMutex);
            tasks.swap(fTasks);
        }
        for (auto& task : tasks)
        {
            task();
        }
    }

    void WaylandPlatformState::dispatchOnce(int timeoutMilliseconds)
    {
        if (fDisplay == nullptr)
        {
            fQuitRequested = true;
            return;
        }

        int result = wl_display_dispatch_pending(fDisplay);
        while (result >= 0 && wl_display_prepare_read(fDisplay) != 0)
        {
            result = wl_display_dispatch_pending(fDisplay);
        }
        if (result < 0)
        {
            fQuitRequested = true;
            return;
        }

        std::ignore = wl_display_flush(fDisplay);
        pollfd descriptors[]{
            {.fd = wl_display_get_fd(fDisplay), .events = POLLIN, .revents = 0},
            {.fd = fWakeDescriptor, .events = POLLIN, .revents = 0},
        };
        const int pollResult = poll(descriptors, std::size(descriptors), timeoutMilliseconds);
        if (pollResult > 0 && (descriptors[0].revents & POLLIN) != 0)
        {
            if (wl_display_read_events(fDisplay) < 0)
            {
                fQuitRequested = true;
            }
        }
        else
        {
            wl_display_cancel_read(fDisplay);
        }

        if (pollResult > 0 && (descriptors[1].revents & POLLIN) != 0)
        {
            uint64_t value = 0;
            while (read(fWakeDescriptor, &value, sizeof(value)) > 0)
            {
            }
            dispatchTasks();
        }
        else if (pollResult < 0 && errno != EINTR)
        {
            fQuitRequested = true;
        }

        if (!fQuitRequested && wl_display_dispatch_pending(fDisplay) < 0)
        {
            fQuitRequested = true;
        }
    }

    void WaylandPlatformState::releaseObjects()
    {
        fWindows.clear();
        fPressedKeys.clear();
        fPointerWindow = nullptr;
        fKeyboardWindow = nullptr;
        if (fKeyboard != nullptr)
            wl_keyboard_release(fKeyboard);
        if (fPointer != nullptr)
            wl_pointer_release(fPointer);
        if (fSeat != nullptr)
            wl_seat_release(fSeat);
        for (Output& output : fOutputs)
            wl_output_destroy(output.object);
        fOutputs.clear();
        if (fShell != nullptr)
            xdg_wm_base_destroy(fShell);
        if (fDecorationManager != nullptr)
            zxdg_decoration_manager_v1_destroy(fDecorationManager);
        if (fSharedMemory != nullptr)
            wl_shm_destroy(fSharedMemory);
        if (fCompositor != nullptr)
            wl_compositor_destroy(fCompositor);
        if (fRegistry != nullptr)
            wl_registry_destroy(fRegistry);
        if (fDisplay != nullptr)
            wl_display_disconnect(fDisplay);
        if (fWakeDescriptor >= 0)
            close(fWakeDescriptor);
        fKeyboard = nullptr;
        fPointer = nullptr;
        fSeat = nullptr;
        fShell = nullptr;
        fDecorationManager = nullptr;
        fSharedMemory = nullptr;
        fCompositor = nullptr;
        fRegistry = nullptr;
        fDisplay = nullptr;
        fWakeDescriptor = -1;
        {
            const std::scoped_lock lock(fTaskMutex);
            fTasks.clear();
        }
        fQuitRequested = false;
    }
}  // namespace LWS::internal

#endif

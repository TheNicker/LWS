#include <catch2/catch_test_macros.hpp>

#ifdef LWS_PLATFORM_WAYLAND

    #include <LWS/Platform.hpp>
    #include <LWS/Timer.hpp>
    #include <LWS/Wayland/PlatformWayland.hpp>
    #include <LWS/Wayland/WindowBackendWayland.hpp>
    #include <LWS/Window.hpp>
    #include <LWS/source/Wayland/internal/KeyCodeLinux.hpp>

    #include <thread>

namespace
{
    class PlatformSession
    {
      public:

        PlatformSession() : result(LWS::Platform::init()) {}

        ~PlatformSession()
        {
            if (result == LWS::Result::Success)
            {
                LWS::Platform::shutdown();
            }
        }

        LWS::Result result;
    };
}  // namespace

TEST_CASE("Wayland translates non-contiguous Linux key codes", "[wayland][keyboard]")
{
    REQUIRE(LWS::internal::keyCodeFromLinux(KEY_A) == LWS::KeyCode::A);
    REQUIRE(LWS::internal::keyCodeFromLinux(KEY_O) == LWS::KeyCode::O);
    REQUIRE(LWS::internal::keyCodeFromLinux(KEY_Z) == LWS::KeyCode::Z);
    REQUIRE(LWS::internal::keyCodeFromLinux(KEY_F10) == LWS::KeyCode::F10);
    REQUIRE(LWS::internal::keyCodeFromLinux(KEY_F11) == LWS::KeyCode::F11);
    REQUIRE(LWS::internal::keyCodeFromLinux(KEY_F12) == LWS::KeyCode::F12);
    REQUIRE(LWS::internal::keyCodeFromLinux(KEY_KP0) == LWS::KeyCode::Numpad0);
    REQUIRE(LWS::internal::keyCodeFromLinux(KEY_KP5) == LWS::KeyCode::Numpad5);
    REQUIRE(LWS::internal::keyCodeFromLinux(KEY_KP9) == LWS::KeyCode::Numpad9);
}

TEST_CASE("Wayland marks repeated key-down events", "[wayland][keyboard]")
{
    LWS::WindowBackendWayland backend;
    bool repeated = false;
    std::ignore = backend.addListener(
        [&](const LWS::AnyEvent& event)
        {
            if (const auto* key = std::get_if<LWS::EventKeyDown>(&event))
                repeated = key->key == LWS::KeyCode::Right && key->repeat;
            return true;
        });

    backend.handleKey(LWS::KeyCode::Right, true, true);
    REQUIRE(repeated);
}

TEST_CASE("Wayland creates an xdg-shell window", "[wayland][window]")
{
    PlatformSession platform;
    if (platform.result != LWS::Result::Success)
    {
        SKIP("No Wayland compositor is available");
    }

    LWS::Window window;
    LWS::WindowConfig config;
    config.visible = false;
    config.styles = LWS::WindowStyle::Caption | LWS::WindowStyle::CloseButton;
    REQUIRE(window.Create(config) == LWS::Result::Success);
    REQUIRE(LWS::Wayland::GetDisplay() != nullptr);
    REQUIRE(window.GetBackendId() == LWS::BackendId::Wayland);
    REQUIRE(window.GetHandle() != 0);

    window.SetMinMaxSize({320, 200}, {1920, 1080});
    REQUIRE((window.GetMinSize() == LWS::Size{320, 200}));
    REQUIRE((window.GetMaxSize() == LWS::Size{1920, 1080}));
}

TEST_CASE("Wayland requests the first paint without background erasure", "[wayland][window]")
{
    PlatformSession platform;
    if (platform.result != LWS::Result::Success)
    {
        SKIP("No Wayland compositor is available");
    }

    bool painted = false;
    LWS::Window window;
    std::ignore = window.AddEventListener(
        [&](const LWS::AnyEvent& event)
        {
            if (std::holds_alternative<LWS::EventPaint>(event))
            {
                painted = true;
                LWS::Platform::requestQuit();
            }
            return true;
        });

    LWS::WindowConfig config;
    config.visible = true;
    config.eraseBackground = false;
    REQUIRE(window.Create(config) == LWS::Result::Success);

    LWS::HighPrecisionTimer timeout([] { LWS::Platform::requestQuit(); });
    timeout.SetDueTime(1000);
    timeout.SetRepeatInterval(0);
    timeout.Enable(true);
    LWS::Platform::runMessageLoop();
    timeout.Enable(false);

    REQUIRE(painted);
}

TEST_CASE("Wayland timers wake the UI loop", "[wayland][timer]")
{
    PlatformSession platform;
    if (platform.result != LWS::Result::Success)
    {
        SKIP("No Wayland compositor is available");
    }

    const std::thread::id uiThread = std::this_thread::get_id();
    bool firedOnUiThread = false;
    LWS::HighPrecisionTimer timer(
        [&]
        {
            firedOnUiThread = std::this_thread::get_id() == uiThread;
            LWS::Platform::requestQuit();
        });
    timer.SetDueTime(10);
    timer.SetRepeatInterval(0);
    timer.Enable(true);
    LWS::Platform::runMessageLoop();
    timer.Enable(false);

    REQUIRE(firedOnUiThread);
}

TEST_CASE("Wayland tasks wake and execute on the UI thread", "[wayland][task]")
{
    PlatformSession platform;
    if (platform.result != LWS::Result::Success)
    {
        SKIP("No Wayland compositor is available");
    }

    const std::thread::id uiThread = std::this_thread::get_id();
    bool firedOnUiThread = false;
    std::thread worker(
        [&]
        {
            LWS::Wayland::PostTask(
                [&]
                {
                    firedOnUiThread = std::this_thread::get_id() == uiThread;
                    LWS::Platform::requestQuit();
                });
        });
    worker.join();
    LWS::Platform::runMessageLoop();

    REQUIRE(firedOnUiThread);
}

#endif

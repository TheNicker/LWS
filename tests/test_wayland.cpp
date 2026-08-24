#include <catch2/catch_test_macros.hpp>

#ifdef LWS_PLATFORM_WAYLAND

    #include <LWS/Platform.hpp>
    #include <LWS/Timer.hpp>
    #include <LWS/Window.hpp>

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
    REQUIRE(window.GetBackendId() == LWS::BackendId::Wayland);
    REQUIRE(window.GetHandle() != 0);

    window.SetMinMaxSize({320, 200}, {1920, 1080});
    REQUIRE((window.GetMinSize() == LWS::Size{320, 200}));
    REQUIRE((window.GetMaxSize() == LWS::Size{1920, 1080}));
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

#endif

# LWS

LWS is a compact C++26 windowing layer. It provides portable window lifecycle, input, event, cursor, timer,
clipboard, drag-and-drop, and bitmap-presentation APIs while keeping native backend behavior behind explicit
platform boundaries.

## Supported platforms

| Platform | Backend | Status |
| --- | --- | --- |
| Windows | Win32 | Supported |
| Linux | Wayland | Supported when the Wayland development packages and protocols are available |
| Linux | X11 | Scaffold only; no window backend is currently provided |

Applications can query optional behavior with `LWS::Platform::supports` rather than assuming that every backend
implements every feature.

## Build

LWS requires CMake 3.20 or newer and a compiler with the C++26 mode used by the project.

```sh
cmake -S . -B build -DLWS_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Embed LWS with `add_subdirectory` and link the `LWSLib` target:

```cmake
add_subdirectory(path/to/LWS)
target_link_libraries(your_application PRIVATE LWSLib)
```

## Quick start

```cpp
#include <LWS/Platform.hpp>
#include <LWS/Window.hpp>

int main()
{
    const LWS::Platform::Session platform;
    if (!platform)
        return 1;

    LWS::Window window;
    const LWS::WindowConfig config{
        .size = {800, 600},
        .styles = LWS::WindowStyle::Caption | LWS::WindowStyle::CloseButton,
        .visible = true,
    };
    if (window.Create(config) != LWS::Result::Success)
        return 1;

    LWS::Platform::runMessageLoop();
    return 0;
}
```

## Repository layout

| Path | Purpose |
| --- | --- |
| `src/LWS/include/LWS` | Public headers and platform extension headers |
| `src/LWS/source` | Portable implementation and native backends |
| `tests` | Unit and platform integration tests |

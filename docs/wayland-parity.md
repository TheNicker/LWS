# LWS Wayland Backend — Feature Parity Reference

This document maps every `IWindowBackend` method and `LWS::Platform` function to its
Wayland protocol equivalent, notes which features are not available, and lists the
Wayland extension protocols required for each optional feature.

---

## Core Wayland Protocols Required

| Protocol | Use |
|---|---|
| `wayland.xml` (core) | `wl_compositor`, `wl_surface`, `wl_shm`, `wl_output`, `wl_seat`, `wl_pointer`, `wl_keyboard` |
| `xdg-shell` (`xdg-wm-base`) | Toplevel window creation, title, min/max size, fullscreen, maximize |
| `xdg-decoration-unstable-v1` | Server-side vs client-side window decorations |
| `zwlr-layer-shell-v1` | Always-on-top (layer surface) — compositor extension, not universally available |
| `zwp-pointer-constraints-unstable-v1` | Mouse capture / lock (`LockMouseToWindowMode`) |
| `wp-fractional-scale-v1` + `wp-viewporter` | Fractional DPI scaling |
| `wl-data-device-manager` | Clipboard and drag-and-drop |
| `xdg-activation-v1` | Window focus/raise requests |

---

## IWindowBackend Method Mapping

| LWS Method | Wayland Protocol Call | Notes |
|---|---|---|
| `create()` | `wl_compositor_create_surface` → `xdg_wm_base_get_xdg_surface` → `xdg_surface_get_toplevel` | Requires `wl_display_roundtrip` to receive initial configure event |
| `destroy()` | `xdg_toplevel_destroy` + `xdg_surface_destroy` + `wl_surface_destroy` | Must be called on the main Wayland thread |
| `show()` | Unset minimized state; `wl_surface_commit()` | Wayland has no show/hide toggle; only minimized state via `xdg_toplevel_set_minimized` |
| `hide()` | `xdg_toplevel_set_minimized(toplevel)` | No direct show after minimize without compositor cooperation |
| `getVisible()` | Track via `xdg_toplevel.configure` state flags | `XDG_TOPLEVEL_STATE_ACTIVATED` indicates visible and focused |
| `setDisplayState(Maximized)` | `xdg_toplevel_set_maximized(toplevel)` | |
| `setDisplayState(Minimized)` | `xdg_toplevel_set_minimized(toplevel)` | |
| `setDisplayState(Restored)` | `xdg_toplevel_unset_maximized(toplevel)` | |
| `getDisplayState()` | Read `XDG_TOPLEVEL_STATE_MAXIMIZED` / `XDG_TOPLEVEL_STATE_FULLSCREEN` from last configure | |
| `setTitle()` | `xdg_toplevel_set_title(toplevel, utf8_title)` | Title must be UTF-8; on Win32 it is UTF-16 (`wstring`) — convert with `wstring_convert` or `iconv` |
| `getTitle()` | Stored locally — no query protocol | |
| `setWindowIcon()` | Not available via Wayland protocol | Icon set via `.desktop` file `Icon=` key |
| `setPosition()` | **Not supported** — compositor controls placement | Return `{0,0}` from `getPosition()` |
| `getPosition()` | **Not supported** | Return `{0,0}` |
| `setSize()` | Requested via `xdg_toplevel` configure response | App requests size; compositor may grant a different size via configure event |
| `getClientSize()` | From last `xdg_surface.configure` width/height | |
| `getClientRect()` | `{0,0}` to `getClientSize()` | |
| `getWindowSize()` | Same as `getClientSize()` on Wayland (no window chrome from app side) | Client-side decorations add borders if CSD used |
| `setPlacement()` | `setSize()` + `setDisplayState()` | Position component ignored |
| `getPlacement()` | `{0,0}` + `getClientSize()` + `getDisplayState()` | |
| `setMinMaxSize()` | `xdg_toplevel_set_min_size(w, h)` + `xdg_toplevel_set_max_size(w, h)` | `0,0` means no constraint |
| `setWindowStyles()` | `zxdg_toplevel_decoration_v1.set_mode(SERVER_SIDE)` for decoration toggle | Limited style support; compositor controls appearance |
| `setForeground()` | `xdg_activation_v1_activate` (token-based) | Requires `xdg-activation-v1` protocol; no direct activation API |
| `setFocus()` | Same as `setForeground()` | |
| `isInFocus()` | Track via `wl_keyboard.enter` / `wl_keyboard.leave` | |
| `setAlwaysOnTop()` | `zwlr_layer_shell_v1` layer surface | Requires `zwlr-layer-shell-v1`; not universally available; use `OVERLAY` layer |
| `getAlwaysOnTop()` | Stored locally | |
| `setTransparent()` | Alpha channel in `wl_shm` buffer — always available | Requires compositor compositing support (most Wayland compositors support ARGB) |
| `setBackgroundColor()` | Fill shared memory buffer with color | |
| `setEraseBackground()` | Fill buffer before drawing if true | |
| `setFullScreenState()` | `xdg_toplevel_set_fullscreen(toplevel, wl_output*)` / `unset_fullscreen()` | Pass `NULL` output for "current output" |
| `getFullScreenState()` | Track via `XDG_TOPLEVEL_STATE_FULLSCREEN` flag in configure | |
| `toggleFullScreen()` | Toggle `xdg_toplevel_set_fullscreen` / `unset_fullscreen` | |
| `isMouseInClientRect()` | Track via `wl_pointer.enter` / `wl_pointer.leave` events | |
| `getMousePosition()` | Last `wl_pointer.motion` event position (surface-local coordinates) | |
| `setLockMouseToWindowMode()` | `zwp_locked_pointer_v1` via `zwp_pointer_constraints_v1.lock_pointer` | Requires `zwp-pointer-constraints-unstable-v1` |
| `setDoubleClickMode()` | Non-client area double-click not applicable — compositor-controlled | Client area double-click: synthesize from `wl_pointer.button` with 400ms interval |
| `setCursor()` | `wl_pointer.set_cursor(serial, wl_surface, hotspot_x, hotspot_y)` | Use `wl_cursor_theme_load` + `wl_cursor_image` for named shapes |
| `enableDragAndDrop()` | `wl_data_device_manager.get_data_device(seat)` + `wl_data_offer` handling | File drop: `wl_data_offer.receive(mime_type, fd)` |
| `addListener()` | Event callbacks; wired to `wl_event_queue` + `epoll` on `wl_display_get_fd()` | |
| `getHandle()` | `reinterpret_cast<Handle>(fWlSurface)` | Returns `wl_surface*` as opaque handle |

---

## Platform Namespace Mapping

| `LWS::Platform` Function | Wayland Equivalent |
|---|---|
| `init()` | `wl_display_connect(nullptr)` + registry listener + `wl_display_roundtrip` |
| `shutdown()` | `wl_display_disconnect(display)` |
| `runMessageLoop()` | `while (wl_display_dispatch(display) != -1) {}` |
| `processMessages()` | `wl_display_dispatch_pending(display)` + `wl_display_flush(display)` |
| `isKeyPressed()` | Track via `wl_keyboard.key` events (key repeat and state from `xkbcommon`) |
| `isKeyToggled()` | `xkb_state_mod_name_is_active(XKB_STATE_MODS_LOCKED)` for CapsLock etc. |
| `getMousePosition()` | Last `wl_pointer.motion` event in screen coordinates (from `wl_output` geometry) |
| `moveMouse()` | **Not supported** — pointer position is compositor-controlled on Wayland |
| `browseToFile()` | `xdg-desktop-portal` `org.freedesktop.portal.OpenURI.OpenFile` via D-Bus |

---

## Features with No Wayland Equivalent

| Feature | Status | Recommended Alternative |
|---|---|---|
| `Window::SetPosition()` | Not supported | Document; always return `{0,0}` from `getPosition()` |
| `Window::SetForeground()` (direct) | Needs `xdg-activation-v1` token from compositor | Return `Result::NotSupported` if token unavailable |
| Always-on-top | Needs `zwlr-layer-shell-v1` | Return `Result::NotSupported` if extension unavailable |
| `Platform::moveMouse()` | Not supported | No-op; document limitation |
| `NotificationIcon` (system tray) | No native Wayland API | D-Bus `StatusNotifierItem` (org.kde.StatusNotifierItem) |
| `NotificationIcon::GetIconRect` | Not available on any non-Win32 platform | Return empty Rect; document as Win32-only |
| Window title bar double-click modes | Compositor-controlled decorations | Win32-only; already moved to `WindowWin32` subclass |
| `WM_MENUCHAR` / menu char intercept | Win32-only | Already in `Win32::WindowWin32.SetMenuChar()` |
| `LockMouseToWindowMode::LockResize/LockMove` via `WM_NCHITTEST` | Win32-only hit-test injection | Wayland uses `xdg_toplevel_resize/move` initiated by compositor |
| `FileDialog` (native) | Via xdg-desktop-portal `org.freedesktop.portal.FileChooser` | Requires D-Bus; not pure Wayland |

---

## Cursor Shape Name Mapping

| `LWS::CursorShape` | Wayland cursor theme name | XCursor fallback |
|---|---|---|
| `Arrow` | `left_ptr` | `arrow` |
| `Hand` | `hand2` | `pointer` |
| `IBeam` | `xterm` | `text` |
| `SizeNS` | `sb_v_double_arrow` | `ns-resize` |
| `SizeEW` | `sb_h_double_arrow` | `ew-resize` |
| `SizeNWSE` | `bd_double_arrow` | `nwse-resize` |
| `SizeNESW` | `fd_double_arrow` | `nesw-resize` |
| `SizeAll` | `fleur` | `move` |
| `No` | `circle` | `not-allowed` |
| `Wait` | `watch` | `wait` |
| `AppStarting` | `left_ptr_watch` | `progress` |

---

## Timer Implementation (no Win32 `SetTimer`)

On Wayland, use `timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK)` with the Wayland event loop:

```cpp
// Pseudo-code for LWS::Timer on Wayland
int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
// set interval via timerfd_settime()
// add tfd to epoll alongside wl_display_get_fd()
// when epoll fires on tfd: read(tfd, &expirations, 8); callback()
```

For `HighPrecisionTimer`, use `timer_create(CLOCK_MONOTONIC)` with `SIGEV_THREAD` signal
delivery, then marshal the callback to the main Wayland thread via `eventfd` + the epoll loop.

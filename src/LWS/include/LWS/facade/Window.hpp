#pragma once
#include <LWS/interfaces/backends.hpp>
#include <LWS/facade/Event.hpp>
#include <LWS/facade/DragAndDrop.hpp>
#include <LWS/facade/Result.hpp>

#include <LLUtils/BitFlags.h>
#include <LLUtils/EnumClassBitwise.h>
#include <LLUtils/StringDefs.h>
#include <LLUtils/Templates.h>
#include <LLUtils/Point.h>
#include <LLUtils/Rect.h>
#include <LLUtils/Color.h>
#include <LLUtils/Colors.h>

#include <cstdint>


namespace LWS
{
  class Cursor;
  enum class FullSceenState
  {
    None,
    Windowed,
    SingleScreen,
    MultiScreen

  };

  enum class DoubleClickMode
  {
    NotSet // use the behaviour defined by the window
    ,
    Default // use the bevour defined by a default window.
    ,
    EntireWindow // Double click allowed on the entire window
    ,
    ClientArea // double click allows only on the client area.
    ,
    NonClientArea // e.g. title
  };

  enum class LockMouseToWindowMode
  {
    NoLock,
    LockResize,
    LockMove // e.g. title
  };

  enum class WindowStyle : uint32_t
  {
    NoStyle = 0 << 0 // WS_CAPTION
    ,
    Caption = 1 << 0 // WS_CAPTION
    ,
    CloseButton = 1 << 1 // WS_SYSMENU
    ,
    ResizableBorder = 1 << 2 // WS_SIZEBOX
    ,
    MinimizeButton = 1 << 3 // WS_MINIMIZEBOX
    ,
    MaximizeButton = 1 << 4 // WS_MAXIMIZEBOX;
    ,
    ChildWindow = 1 << 5 // WS_CHILD
    ,
    All = LLUtils::GetMaxBitsMask<uint32_t>()
  };

    using Size = LLUtils::PointI32;
  using Rect = LLUtils::RectI32;
  using Point = LLUtils::PointI32;
  using Handle = uintptr_t;
  struct WindowPlacement
  {
    Point position;
    Size size;
  };

  struct WindowMessage
  {
  };

  LLUTILS_DEFINE_ENUM_CLASS_FLAG_OPERATIONS(WindowStyle)
  using WindowStyleFlags = LLUtils::BitFlags<WindowStyle>;

  class Window;

  using VecChildWindows = std::vector<Window*>;


  
  class Window
  {
    Result create();
    BackendId backendId() const;
    void SetFocused() const;
    Handle GetHandle() const;
    bool IsInFocus() const;
    bool IsMouseCursorInClientRect() const;
    Point GetMousePosition() const;
    bool IsFullScreen() const;
    Size GetClientSize() const;
    Rect GetClientRectangle() const;
    LLUtils::Corner  GetCorner(const Point &point) const;
    Size GetWindowSize() const;
    FullSceenState GetFullScreenState() const { return fFullSceenState; }
    void SetFullScreenState(FullSceenState fullScreenState);
    bool IsUnderMouseCursor() const;
    bool GetEraseBackground() const { return fEraseBackground; }
    DoubleClickMode GetDoubleClickMode() const { return fDoubleClickMode; }
    bool GetEnableMenuChar() const { return fEnableMenuChar; }

    Cursor* GetMouseCursor() const { return fMouseCursor; }
    LockMouseToWindowMode GetLockMouseToWindowMode() const { return fLockMouseToWindowMode; }
    bool GetVisible() const { return fVisible; }
    WindowStyleFlags GetWindowStyles() const { return fWindowStyles; }
    bool GetTransparent() const { return fIsTransparent; }
    Window *GetParent() const { return fParent; }
    bool GetAlwaysOnTop() const;
    Point GetPosition() const;

    virtual ~Window() = default;
    void Create();
    void Destroy();

    Result SendMessage(const WindowMessage& message);
    void AddEventListener(EventCallback callback);
    void RemoveEventListener(EventCallback callback);
    void SetMenuChar(bool enabled);
    void SetPosition(const Point& position);
    void SetSize(const Size size);
    void SetPlacement(const WindowPlacement& placement );
    void ToggleFullScreen(bool multiMonitor = false);
    void Move(const Point& delta);
    void SetMouseCursor(Cursor* cursor);
    void SetEraseBackground(bool eraseBackground) { fEraseBackground = eraseBackground; }
    void SetDestoryOnClose(bool destoryOnClose) { fDestroyOnClose = destoryOnClose; }
    void SetBackgroundColor(const LLUtils::Color color);
    void SetDoubleClickMode(DoubleClickMode doubleClickMode) { fDoubleClickMode = doubleClickMode; }
    void EnableDragAndDrop(bool enable);
    void SetLockMouseToWindowMode(LockMouseToWindowMode mode);
    void SetVisible(bool visible);
    void SetForground();
    void SetWindowStyles(WindowStyle styles, bool enable);
    void SetParent(Window *parent);
    void SetTransparent(bool transparent) { fIsTransparent = transparent; }
    void SetAlwaysOnTop(bool alwaysOnTop);
    void SetTitle(const LLUtils::native_string_type& title);
    void SetWindowIcon(const LLUtils::native_string_type& title);

  protected:
    bool RaiseEvent(const Event &evnt);
    LONG ComposeWindowStyles() const;

  private: // const methods:
  private: // methods:
    void UpdateWindowStyles();
    void SavePlacement();
    void RestorePlacement();
    void SetWindowed();
    void SetFullScreen(bool multiMonitor);
    void NotifyRemovedForRelatedWindows();
    void AddChild(Window *child);
    void RemoveChild(Window *child);
    void DestroyResources();

  private:
    Window *fParent = nullptr;
    VecChildWindows fChildren;
    Cursor* fMouseCursor = nullptr;
    HWND fHandleWindow = nullptr;
    EventCallbackCollection fListeners;
    bool fEnableMenuChar = true;
    FullSceenState fFullSceenState = FullSceenState::Windowed;
    WINDOWPLACEMENT fLastWindowPlacement{};
    DragAndDrop* fDragAndDrop;
    bool fEraseBackground = true;
    bool fDestroyOnClose = true;
    DoubleClickMode fDoubleClickMode = DoubleClickMode::NotSet;
    friend DragAndDrop;
    LockMouseToWindowMode fLockMouseToWindowMode = LockMouseToWindowMode::NoLock;
    bool fVisible = false;
    bool fIsMaximized = false;
    WindowStyleFlags fWindowStyles = WindowStyle::NoStyle;
    bool fIsTransparent = false;
    bool fAlwaysOnTop = false;
    LLUtils::Color fBackgroundColor = LLUtils::Colors::White;

  protected:
    explicit Window(std::shared_ptr<IWindowBackend> impl);
    std::shared_ptr<IWindowBackend> impl_;
  };
}



// static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
// LRESULT WindowProc(const WinMessage &message);

// class Win32Window
// {
// HBRUSH fBackgroundCachedBrush = CreateSolidBrush(RGB(fBackgroundColor.R(), fBackgroundColor.G(),
// fBackgroundColor.B()));
// public:
//         static constexpr LLUtils::native_char_type WindowClassName[] = LLUTILS_TEXT("OIV_WINDOW_CLASS");
//     private:
//         static constexpr LLUtils::native_char_type WindowAddressPropertyName[] = LLUTILS_TEXT("windowClass");
//     public:
//     // const methods

// };
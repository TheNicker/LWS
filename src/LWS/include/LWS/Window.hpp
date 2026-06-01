#pragma once
#include <LWS/Event.hpp>
#include <LWS/Export.hpp>
#include <LWS/Result.hpp>
#include <LWS/interfaces/backends.hpp>

#include <LLUtils/BitFlags.h>
#include <LLUtils/Color.h>
#include <LLUtils/EnumClassBitwise.h>

#include <filesystem>
#include <memory>
#include <vector>

namespace LWS
{
    LLUTILS_DEFINE_ENUM_CLASS_FLAG_OPERATIONS(WindowStyle)
    using WindowStyleFlags = LLUtils::BitFlags<WindowStyle>;

    class Cursor;
    class Window;
    using VecChildWindows = std::vector<Window*>;

    class LWS_API Window
    {
    public:
        Window();
        explicit Window(std::unique_ptr<IWindowBackend> impl);
        virtual ~Window();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        Window(Window&&) noexcept = default;
        Window& operator=(Window&&) noexcept = default;

        Result Create(const WindowConfig& config = {});
        void Destroy();

        void SetTitle(const LWS::string_type& title);
        LWS::string_type GetTitle() const;

        void SetWindowIcon(const std::filesystem::path& iconPath);

        void SetVisible(bool visible);
        bool GetVisible() const;
        void SetDisplayState(WindowDisplayState state);
        WindowDisplayState GetDisplayState() const;

        void SetPosition(const Point& position);
        Point GetPosition() const;
        void SetSize(const Size& sz);
        Size GetClientSize() const;
        Rect GetClientRect() const;
        Size GetWindowSize() const;
        void SetPlacement(const WindowPlacement& placement);
        WindowPlacement GetPlacement() const;
        void SetMinMaxSize(Size minSize, Size maxSize);
        Size GetMinSize() const;
        Size GetMaxSize() const;
        void Move(const Point& delta);

        void CenterOnScreen();
        void CenterOnParent();

        void SetWindowStyles(WindowStyle styles, bool enable);
        WindowStyleFlags GetWindowStyles() const;

        void SetForeground();
        void SetFocused();
        bool IsInFocus() const;

        void SetAlwaysOnTop(bool onTop);
        bool GetAlwaysOnTop() const;
        void SetTransparent(bool transparent);
        bool GetTransparent() const;

        void SetBackgroundColor(LLUtils::Color color);
        void SetEraseBackground(bool erase);
        bool GetEraseBackground() const;

        void ToggleFullScreen(bool multiMonitor = false);
        void SetFullScreenState(FullScreenState state);
        FullScreenState GetFullScreenState() const;
        bool IsFullScreen() const;

        bool IsMouseInClientRect() const;
        bool IsUnderMouseCursor() const;
        Point GetMousePosition() const;
        void SetDoubleClickMode(DoubleClickMode mode);
        DoubleClickMode GetDoubleClickMode() const;
        void SetLockMouseToWindowMode(LockMouseToWindowMode mode);
        LockMouseToWindowMode GetLockMouseToWindowMode() const;

        void SetMouseCursor(Cursor* cursor);
        Cursor* GetMouseCursor() const;

        void SetParent(Window* parent);
        Window* GetParent() const;

        void EnableDragAndDrop(bool enable);

        void SetDestroyOnClose(bool destroyOnClose);

        EventListenerToken AddEventListener(EventCallback callback);
        void RemoveEventListener(EventListenerToken token);
        EventListenerGuard MakeListenerGuard(EventListenerToken token);

        void InjectRawEvent(void* platformEvent);

        BackendId GetBackendId() const;
        Handle GetHandle() const;

    protected:
        template<typename T>
        T* getBackendAs() { return dynamic_cast<T*>(impl_.get()); }

        template<typename T>
        const T* getBackendAs() const { return dynamic_cast<const T*>(impl_.get()); }

        std::unique_ptr<IWindowBackend> impl_;

    private:
        Window* fParent = nullptr;
        VecChildWindows fChildren;
        Cursor* fMouseCursor = nullptr;
        bool fDestroyOnClose = true;

        void AddChild(Window* child);
        void RemoveChild(Window* child);
        void NotifyRemovedFromRelatedWindows();
    };
}

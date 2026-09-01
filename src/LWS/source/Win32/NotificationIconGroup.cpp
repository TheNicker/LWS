#ifdef LWS_PLATFORM_WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>
    #include <Shellapi.h>
    #include <WindowsX.h>

    #include <LWS/NotificationIconGroup.hpp>
    #include <LWS/Win32/EventWin32.hpp>
    #include <LWS/Window.hpp>
    #include <LLUtils/Exception.h>
    #include <LLUtils/StringUtility.h>
    #include <LLUtils/Templates.h>
    #include <LLUtils/UniqueIDProvider.h>

    #include <map>
    #include <set>

namespace LWS
{
    class NotificationIconGroup::Impl
    {
      public:

        Impl() = default;

        ~Impl()
        {
            NOTIFYICONDATA nid{};
            nid.cbSize = sizeof(nid);
            nid.hWnd = reinterpret_cast<HWND>(fWindow.GetHandle());
            nid.uVersion = NOTIFYICON_VERSION_4;
            nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;

            for (const auto& [id, _] : fMapIconData)
            {
                nid.uID = id;
                Shell_NotifyIcon(NIM_DELETE, &nid);
            }
        }

        IconID AddIconResource(uint16_t iconResourceId, const string_type& tooltip,
                               NotificationIconEvent& notificationEvent)
        {
            if (fWindow.GetHandle() == 0)
            {
                std::ignore = fWindow.Create({.visible = false});
                fWindow.SetVisible(false);
                if (Win32::SetPlatformCallback(
                        fWindow,
                        [this, &notificationEvent](const Win32::PlatformEvent& event)
                        {
                            if (const auto* notification = std::get_if<Win32::NotificationIconEvent>(&event))
                            {
                                HandleMessage(*notification, notificationEvent);
                                return std::optional<LRESULT>{0};
                            }
                            return std::optional<LRESULT>{};
                        }) != Result::Success)
                {
                    LL_EXCEPTION(LLUtils::Exception::ErrorCode::InvalidState,
                                 "Unable to register the notification-icon platform callback");
                }
            }

            NOTIFYICONDATA nid{};
            nid.cbSize = sizeof(nid);
            nid.hWnd = reinterpret_cast<HWND>(fWindow.GetHandle());
            nid.uVersion = NOTIFYICON_VERSION_4;
            nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP;
            IconID iconId = fIconIdProvider.Acquire();
            nid.uID = static_cast<UINT>(iconId);
            nid.uCallbackMessage = Win32::NotificationIconEvent::MessageId;

            LLUtils::StringUtility::StrCpy(nid.szTip, tooltip.c_str(), LLUtils::array_length(nid.szTip));
            nid.hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(iconResourceId));

            if (Shell_NotifyIcon(NIM_ADD, &nid) == TRUE && Shell_NotifyIcon(NIM_SETVERSION, &nid) == TRUE)
            {
                fMapIconData.emplace(iconId, NotificationIconData{iconId});
            }
            else
            {
                fIconIdProvider.Release(iconId);
                LL_EXCEPTION_SYSTEM_ERROR("Cannot add notification icon");
            }

            return iconId;
        }

        Rect GetIconRect(IconID iconid) const
        {
            auto iconData = fMapIconData.find(iconid);
            if (iconData != fMapIconData.end())
            {
                NOTIFYICONIDENTIFIER iconIdentifer{static_cast<DWORD>(sizeof(NOTIFYICONIDENTIFIER)),
                                                   reinterpret_cast<HWND>(fWindow.GetHandle()),
                                                   static_cast<UINT>(iconid), GUID{}};
                RECT rect{};

                if (Shell_NotifyIconGetRect(&iconIdentifer, &rect) == S_OK)
                {
                    return {{rect.left, rect.top}, {rect.right, rect.bottom}};
                }

                LL_EXCEPTION(LLUtils::Exception::ErrorCode::NotFound, "Icon id not found");
            }

            return {};
        }

      private:

        void HandleMessage(const Win32::NotificationIconEvent& message, NotificationIconEvent& notificationEvent)
        {
            switch (message.notification)
            {
                case NIN_SELECT:
                    notificationEvent.Raise(
                        NotificationIconEventArgs{NotificationIconAction::Select, message.x, message.y});
                    break;
                case WM_CONTEXTMENU:
                    notificationEvent.Raise(
                        NotificationIconEventArgs{NotificationIconAction::ContextMenu, message.x, message.y});
                    break;
                default:
                    break;
            }
        }

        struct NotificationIconData
        {
            IconID id;
        };

        std::map<IconID, NotificationIconData> fMapIconData;
        Window fWindow;
        LLUtils::UniqueIdProvider<IconID, std::set<IconID>> fIconIdProvider{1};
    };

    NotificationIconGroup::NotificationIconGroup() : impl_(std::make_unique<Impl>()) {}
    NotificationIconGroup::~NotificationIconGroup() = default;

    NotificationIconGroup::IconID NotificationIconGroup::AddIconResource(uint16_t iconResourceId,
                                                                         const string_type& tooltip)
    {
        return impl_->AddIconResource(iconResourceId, tooltip, OnNotificationIconEvent);
    }

    Rect NotificationIconGroup::GetIconRect(IconID iconid) const
    {
        return impl_->GetIconRect(iconid);
    }
}  // namespace LWS
#endif

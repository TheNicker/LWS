#pragma once
#include <functional>
#include <vector>
#include <LLUtils/StringDefs.h>

namespace LWS
{
    struct WinMessage
    {
        uintptr_t  hWnd;
        uint64_t message;
        uintptr_t  wParam;
        uintptr_t  lParam;
    };

        class Window;
        class Event
        {
        public:
            Window* window = nullptr;
            virtual ~Event() {}
        };

        class EventWinMessage : public Event
        {
        public:
            WinMessage message = {};
        };

    

        typedef std::function< bool(const Event*) > EventCallback;
        typedef std::vector <EventCallback> EventCallbackCollection;
        

        class EventDragDrop : public Event
        {
            
        };


        class EventDdragDropFile : public EventDragDrop
        {
        public:
            LLUtils::native_string_type fileName;
        };
    }
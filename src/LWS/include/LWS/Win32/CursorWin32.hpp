#pragma once

#include <LWS/facade/Cursor.hpp>
#include <LWS/Win32/CursorBackendWin32.hpp>

namespace LWS
{
    class CursorWin32 : public Cursor
    {
    public:
    CursorWin32() : Cursor(std::make_shared<CursorBackendWin32>()) 
    {
    }
    };
}

#include <LWS/Win32/CursorBackendWin32.hpp>
namespace LWS
{
void CursorBackendWin32::setVisible(bool visible) 
{
  std::cout << "Win32: Cursor visibility = " << visible << "\n";
}

BackendId CursorBackendWin32::backend() const 
{ 
    return BackendId::Win32; 
}
}
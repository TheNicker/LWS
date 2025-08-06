#include <LWS/facade/Window.h>
namespace LWS
{
class Win32Window : public Window
{
public:
  Win32Window();
  void win32SpecialCall();
};
}
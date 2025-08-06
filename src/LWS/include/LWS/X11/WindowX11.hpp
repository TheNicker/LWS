#include <LWS/facade/Window.h>
namespace LWS
{
class X11Window : public Window
{
public:
  X11Window();
  void x11SpecialCall();
};
}
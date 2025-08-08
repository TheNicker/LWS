#pragma once
#include <LWS/facade/Window.hpp>
namespace LWS
{
class WindowX11 : public Window
{
public:
  WindowX11();
  void x11SpecialCall();
};
}
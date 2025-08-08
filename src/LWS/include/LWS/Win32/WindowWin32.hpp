#pragma once
#include <LWS/facade/Window.hpp>
namespace LWS
{
class WindowWin32 : public Window
{
public:
  WindowWin32();
  void win32SpecialCall();
};
}
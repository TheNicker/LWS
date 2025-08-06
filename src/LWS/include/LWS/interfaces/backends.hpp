#pragma once
#include <memory>
#include <string>
#include <cassert>
#include <stdexcept>
#include <iostream>

namespace LWS
{
  enum class BackendId {Undefined, Win32, Wayland, X11 };
class Cursor;

class ICursorBackend 
{
public:
  virtual ~ICursorBackend() = default;
  virtual void setVisible(bool) = 0;
  virtual BackendId backend() const = 0;
};

class IWindowBackend 
{
public:
  virtual ~IWindowBackend() = default;
  virtual void show() = 0;
  virtual void setTitle(std::string_view) = 0;
  virtual void setCursor(Cursor) = 0;
  virtual BackendId backend() const = 0;
};
}
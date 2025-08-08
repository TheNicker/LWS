#pragma once
#include <LWS/interfaces/backends.hpp>
namespace LWS
{
class CursorBackendWin32 : public ICursorBackend 
{
public:
  void setVisible(bool visible) override;
  BackendId backend() const override;
};
}
#pragma once
#include <LWS/interfaces/backends.hpp>
namespace LWS
{
class WindowBackendX11 : public IWindowBackend {
public:
  void show() override;
  void setTitle(std::string_view title) override;
  void setCursor(std::shared_ptr<ICursorBackend> cursor) override;
  BackendId backend() const override;
  void getServerAddress();
};
}
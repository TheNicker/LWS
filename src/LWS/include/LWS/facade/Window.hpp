#pragma once
#include <LWS/interfaces/backends.hpp>
namespace LWS
{

class Window 
{
public:
  static Window create(BackendId id);

  void show();
  void setTitle(std::string_view title);
  void setCursor(Cursor* cursor);
  BackendId backendId() const;

protected:
  explicit Window(std::shared_ptr<IWindowBackend> impl);
  std::shared_ptr<IWindowBackend> impl_;
};
}
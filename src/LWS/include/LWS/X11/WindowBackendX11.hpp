#include <LWS/interfaces/backends.hpp>
namespace LWS
{
class X11WindowBackend : public IWindowBackend {
public:
  void show() override;
  void setTitle(std::string_view title) override;
  void setCursor(Cursor* cursor) override;
  BackendId backend() const override;
  void getServerAddress();
};
}
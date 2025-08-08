#include <LWS/facade/Window.hpp>
#include <LWS/facade/Cursor.hpp>
#include <cassert>
namespace LWS 
{
    Window::Window(std::shared_ptr<IWindowBackend> impl) : impl_(std::move(impl)) {}
    
void Window::show() { impl_->show(); }

void Window::setTitle(std::string_view title) { impl_->setTitle(title); }

void Window::setCursor(Cursor* cursor) 
{
  assert(cursor->backendId() == backendId());
  impl_->setCursor(cursor->getBackend());
}

BackendId Window::backendId() const { return impl_->backend(); }
}
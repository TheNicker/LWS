#include <LWS/facade/Cursor.hpp>
namespace LWS
{
    Cursor::Cursor(std::shared_ptr<ICursorBackend> impl) : impl_(std::move(impl)) {}

    void Cursor::setVisible(bool visible) { impl_->setVisible(visible); }

    BackendId Cursor::backendId() const { return impl_->backend(); }

    std::shared_ptr<ICursorBackend> Cursor::getBackend() const { return impl_; }
}
#pragma once



Cursor::Cursor(std::shared_ptr<ICursorBackend> impl) : impl_(std::move(impl)) {}


void Cursor::setVisible(bool visible) { impl_->setVisible(visible); }

BackendId Cursor::backendId() const { return impl_->backend(); }

std::shared_ptr<ICursorBackend> Cursor::getBackend() const { return impl_; }



Window::Window(std::shared_ptr<IWindowBackend> impl) : impl_(std::move(impl)) {}


void Window::show() { impl_->show(); }

void Window::setTitle(std::string_view title) { impl_->setTitle(title); }

void Window::setCursor(const Cursor &cursor) {
  assert(cursor.backendId() == backendId());
  impl_->setCursor(cursor.getBackend());
}

BackendId Window::backendId() const { return impl_->backend(); }


class Win32CursorBackend : public ICursorBackend 
{
public:
  void setVisible(bool visible) override;
  BackendId backend() const override;
};

class Win32Cursor : public Cursor
{
public:
Win32Cursor() : Cursor(std::make_shared<Win32CursorBackend>()) 
{
}
};

void Win32WindowBackend::show() 
{
   std::cout << "Win32: Show window\n"; 
}

void Win32WindowBackend::setTitle(std::string_view title) 
{
  std::cout << "Win32: Set title to " << title << "\n";
}

void Win32WindowBackend::setCursor(std::shared_ptr<ICursorBackend>) 
{
  std::cout << "Win32: Set cursor\n";
}

BackendId Win32WindowBackend::backend() const { return BackendId::Win32; }

void Win32WindowBackend::specialFeature() 
{
  std::cout << "Win32: Special backend-specific feature\n";
}


void Win32CursorBackend::setVisible(bool visible) 
{
  std::cout << "Win32: Cursor visibility = " << visible << "\n";
}

BackendId Win32CursorBackend::backend() const 
{ 
    return BackendId::Win32; 
}



Win32Window::Win32Window() :Window::Window(std::make_shared<Win32WindowBackend>())
{
  
}

void Win32Window::win32SpecialCall() 
{
  auto *win32 = static_cast<Win32WindowBackend *>(impl_.get());
  win32->specialFeature();
}


X11Window::X11Window() :Window::Window(std::make_shared<X11WindowBackend>())
{
  
}

void X11Window::x11SpecialCall()
{
  auto *x11Windowbackend = static_cast<X11WindowBackend*>(impl_.get());
  x11Windowbackend->getServerAddress();
}


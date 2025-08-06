
namespace LWS
{
class Cursor 
{
public:
  void setVisible(bool visible);
  BackendId backendId() const;
  std::shared_ptr<ICursorBackend> getBackend() const;

  protected:
    explicit Cursor(std::shared_ptr<ICursorBackend> impl);
private:

  std::shared_ptr<ICursorBackend> impl_;

};
}
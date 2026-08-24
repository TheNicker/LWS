#include <LWS/Timer.hpp>

namespace LWS
{
    Timer::Timer() : impl_(internal::createTimerBackend()) {}

    Timer::~Timer() = default;
    void Timer::SetTargetWindow(Handle windowHandle)
    {
        impl_->setTargetWindow(windowHandle);
    }
    uint32_t Timer::GetInterval() const
    {
        return impl_->getInterval();
    }
    void Timer::SetInterval(uint32_t interval)
    {
        impl_->setInterval(interval);
    }
    void Timer::SetCallback(Callback callback)
    {
        impl_->setCallback(std::move(callback));
    }

    HighPrecisionTimer::HighPrecisionTimer(Callback callback)
        : impl_(internal::createHighPrecisionTimerBackend(std::move(callback)))
    {
    }

    HighPrecisionTimer::~HighPrecisionTimer() = default;
    void HighPrecisionTimer::SetRepeatInterval(uint32_t interval)
    {
        impl_->setRepeatInterval(interval);
    }
    void HighPrecisionTimer::SetDueTime(uint32_t dueTime)
    {
        impl_->setDueTime(dueTime);
    }
    bool HighPrecisionTimer::GetEnabled() const
    {
        return impl_->getEnabled();
    }
    void HighPrecisionTimer::Enable(bool enabled)
    {
        impl_->enable(enabled);
    }
}  // namespace LWS

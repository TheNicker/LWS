#ifdef LWS_PLATFORM_WAYLAND

    #include <LWS/Timer.hpp>

    #include "internal/PlatformState.hpp"

    #include <atomic>
    #include <chrono>
    #include <condition_variable>
    #include <limits>
    #include <mutex>
    #include <thread>

namespace LWS
{
    namespace
    {
        struct CallbackState
        {
            internal::WaylandPlatformState* platform = nullptr;
            std::mutex mutex;
            Timer::Callback callback;
            std::atomic_bool enabled = false;
        };

        void postCallback(const std::shared_ptr<CallbackState>& state)
        {
            if (state->platform == nullptr || !state->platform->isInitialized())
            {
                return;
            }
            state->platform->postTask(
                [weakState = std::weak_ptr(state)]
                {
                    if (const auto locked = weakState.lock(); locked != nullptr && locked->enabled)
                    {
                        Timer::Callback callback;
                        {
                            const std::scoped_lock lock(locked->mutex);
                            callback = locked->callback;
                        }
                        if (callback)
                            callback();
                    }
                });
        }

        std::jthread startTimerThread(const std::shared_ptr<CallbackState>& state, uint32_t dueTime,
                                      uint32_t repeatInterval)
        {
            return std::jthread(
                [state, dueTime, repeatInterval](std::stop_token stopToken)
                {
                    std::condition_variable_any wake;
                    std::mutex waitMutex;
                    auto wait = [&](uint32_t milliseconds)
                    {
                        std::unique_lock lock(waitMutex);
                        return !wake.wait_for(lock, stopToken, std::chrono::milliseconds(milliseconds),
                                              [] { return false; });
                    };

                    bool fire = dueTime != std::numeric_limits<uint32_t>::max() && wait(dueTime);
                    while (fire && state->enabled && !stopToken.stop_requested())
                    {
                        postCallback(state);
                        fire = repeatInterval != 0 && repeatInterval != std::numeric_limits<uint32_t>::max() &&
                               wait(repeatInterval);
                    }
                });
        }
    }  // namespace

    class TimerBackendWayland final : public ITimerBackend
    {
      public:

        TimerBackendWayland() : fState(std::make_shared<CallbackState>())
        {
            fState->platform = &internal::WaylandPlatformState::current();
        }

        ~TimerBackendWayland() override { stop(); }

        void setTargetWindow(Handle) override {}
        uint32_t getInterval() const override { return fInterval; }
        void setCallback(Callback callback) override
        {
            const std::scoped_lock lock(fState->mutex);
            fState->callback = std::move(callback);
        }

        void setInterval(uint32_t interval) override
        {
            if (fInterval != interval)
            {
                stop();
                fInterval = interval;
                if (interval != 0)
                {
                    fState->enabled = true;
                    fThread = startTimerThread(fState, interval, interval);
                }
            }
        }

        void stop()
        {
            fState->enabled = false;
            if (fThread.joinable())
            {
                fThread.request_stop();
                fThread.join();
            }
        }

        std::shared_ptr<CallbackState> fState;
        std::jthread fThread;
        uint32_t fInterval = 0;
    };

    class HighPrecisionTimerBackendWayland final : public IHighPrecisionTimerBackend
    {
      public:

        explicit HighPrecisionTimerBackendWayland(ITimerBackend::Callback callback)
            : fState(std::make_shared<CallbackState>())
        {
            fState->platform = &internal::WaylandPlatformState::current();
            fState->callback = std::move(callback);
        }

        ~HighPrecisionTimerBackendWayland() override { enable(false); }

        void enable(bool enabled) override
        {
            if (enabled == fState->enabled)
            {
                return;
            }
            fState->enabled = false;
            if (fThread.joinable())
            {
                fThread.request_stop();
                fThread.join();
            }
            if (enabled)
            {
                fState->enabled = true;
                fThread = startTimerThread(fState, fDueTime, fRepeatInterval);
            }
        }

        void setRepeatInterval(uint32_t repeatInterval) override
        {
            if (fRepeatInterval != repeatInterval)
            {
                fRepeatInterval = repeatInterval;
                restartIfEnabled();
            }
        }

        void setDueTime(uint32_t dueTime) override
        {
            if (fDueTime != dueTime)
            {
                fDueTime = dueTime;
                restartIfEnabled();
            }
        }

        bool getEnabled() const override { return fState->enabled; }

        void restartIfEnabled()
        {
            if (fState->enabled)
            {
                enable(false);
                enable(true);
            }
        }

        std::shared_ptr<CallbackState> fState;
        std::jthread fThread;
        uint32_t fDueTime = std::numeric_limits<uint32_t>::max();
        uint32_t fRepeatInterval = std::numeric_limits<uint32_t>::max();
    };

    namespace internal
    {
        std::unique_ptr<ITimerBackend> createTimerBackend()
        {
            return std::make_unique<TimerBackendWayland>();
        }

        std::unique_ptr<IHighPrecisionTimerBackend> createHighPrecisionTimerBackend(ITimerBackend::Callback callback)
        {
            return std::make_unique<HighPrecisionTimerBackendWayland>(std::move(callback));
        }
    }  // namespace internal
}  // namespace LWS

#endif

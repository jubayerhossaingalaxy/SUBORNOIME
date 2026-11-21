#pragma once

#include <Windows.h>
#include <atomic>

namespace suborno::platform::windows::com {

class ModuleLifetime final {
  public:
    void object_created() noexcept {
        objects_.fetch_add(1, std::memory_order_relaxed);
    }

    void object_destroyed() noexcept {
        objects_.fetch_sub(1, std::memory_order_release);
    }

    HRESULT lock_server(bool lock) noexcept {
        if (lock) {
            locks_.fetch_add(1, std::memory_order_relaxed);
            return S_OK;
        }

        unsigned long current = locks_.load(std::memory_order_acquire);
        while (current != 0) {
            if (locks_.compare_exchange_weak(
                    current, current - 1, std::memory_order_release, std::memory_order_acquire)) {
                return S_OK;
            }
        }
        return E_UNEXPECTED;
    }

    HRESULT can_unload() const noexcept {
        return objects_.load(std::memory_order_acquire) == 0
                       && locks_.load(std::memory_order_acquire) == 0
                   ? S_OK
                   : S_FALSE;
    }

  private:
    std::atomic<unsigned long> objects_{0};
    std::atomic<unsigned long> locks_{0};
};

inline ModuleLifetime& module_lifetime() noexcept {
    static ModuleLifetime lifetime;
    return lifetime;
}

}  // namespace suborno::platform::windows::com

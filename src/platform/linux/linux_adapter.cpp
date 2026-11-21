#include "suborno/platform/linux/linux_adapter.hpp"

namespace suborno::platform::linux {

LinuxAdapterStatus IBusAdapter::status() const noexcept {
    return LinuxAdapterStatus::unavailable;
}

LinuxAdapterStatus Fcitx5Adapter::status() const noexcept {
    return LinuxAdapterStatus::unavailable;
}

}  // namespace suborno::platform::linux

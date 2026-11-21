#include "suborno/platform/macos/macos_adapter.hpp"

namespace suborno::platform::macos {

MacOSAdapterStatus MacOSAdapter::status() const noexcept {
    return MacOSAdapterStatus::unavailable;
}

}  // namespace suborno::platform::macos

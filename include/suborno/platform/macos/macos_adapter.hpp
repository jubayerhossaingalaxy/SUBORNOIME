#pragma once

namespace suborno::platform::macos {

enum class MacOSAdapterStatus {
    unavailable,
    available,
};

class InputMethodKitAdapter {
  public:
    virtual ~InputMethodKitAdapter() = default;
    virtual MacOSAdapterStatus status() const noexcept = 0;
};

class MacOSAdapter final : public InputMethodKitAdapter {
  public:
    MacOSAdapterStatus status() const noexcept override;
};

}  // namespace suborno::platform::macos

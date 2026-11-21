#pragma once

#include <string>

namespace suborno::platform::linux {

enum class LinuxAdapterStatus {
    unavailable,
    available,
};

class LinuxAdapter {
  public:
    virtual ~LinuxAdapter() = default;
    virtual LinuxAdapterStatus status() const noexcept = 0;
};

class IBusAdapter final : public LinuxAdapter {
  public:
    LinuxAdapterStatus status() const noexcept override;
};

class Fcitx5Adapter final : public LinuxAdapter {
  public:
    LinuxAdapterStatus status() const noexcept override;
};

}  // namespace suborno::platform::linux

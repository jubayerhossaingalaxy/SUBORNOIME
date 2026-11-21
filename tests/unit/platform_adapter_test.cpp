#include <gtest/gtest.h>

#include "suborno/platform/linux/linux_adapter.hpp"

TEST(PlatformAdapterTest, IBusAndFcitxReportUnavailableWithoutHostLibraries) {
    suborno::platform::linux::IBusAdapter ibus;
    suborno::platform::linux::Fcitx5Adapter fcitx;
    EXPECT_EQ(ibus.status(), suborno::platform::linux::LinuxAdapterStatus::unavailable);
    EXPECT_EQ(fcitx.status(), suborno::platform::linux::LinuxAdapterStatus::unavailable);
}

#pragma once

#include <Windows.h>

namespace suborno::platform::windows::app {

int run(HINSTANCE instance, PWSTR command_line, int show_command);

}  // namespace suborno::platform::windows::app

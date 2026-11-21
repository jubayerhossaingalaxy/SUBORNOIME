#include "suborno/platform/windows/app/application.hpp"

int WINAPI wWinMain(_In_ HINSTANCE instance,
                    _In_opt_ HINSTANCE,
                    _In_ PWSTR command_line,
                    _In_ int show_command) {
    return suborno::platform::windows::app::run(instance, command_line, show_command);
}

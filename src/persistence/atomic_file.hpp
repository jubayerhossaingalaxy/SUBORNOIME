#pragma once

#include <filesystem>
#include <string_view>
#include <system_error>

namespace suborno::persistence {

std::error_code write_file_atomically(const std::filesystem::path& destination,
                                      std::string_view contents);

}  // namespace suborno::persistence

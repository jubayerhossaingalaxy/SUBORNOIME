#pragma once

#include <Windows.h>
#include <filesystem>

namespace suborno::platform::windows {

HRESULT get_user_data_directory(std::filesystem::path& directory) noexcept;
HRESULT get_legacy_user_data_directory(std::filesystem::path& directory) noexcept;
HRESULT get_shared_data_directory(std::filesystem::path& directory) noexcept;
HRESULT get_user_settings_path(std::filesystem::path& path) noexcept;
HRESULT get_user_dictionary_path(std::filesystem::path& path) noexcept;
HRESULT get_user_layouts_directory(std::filesystem::path& directory) noexcept;
HRESULT get_shared_assets_directory(std::filesystem::path& directory) noexcept;
HRESULT get_shared_fonts_directory(std::filesystem::path& directory) noexcept;
HRESULT get_shared_themes_directory(std::filesystem::path& directory) noexcept;
HRESULT get_shared_dictionaries_directory(std::filesystem::path& directory) noexcept;
HRESULT get_shared_layouts_directory(std::filesystem::path& directory) noexcept;

HRESULT prepare_user_data_directory(const std::filesystem::path& directory,
                                    const std::filesystem::path& legacy_directory);

}  // namespace suborno::platform::windows

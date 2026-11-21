#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "suborno/layout-api/input_layout.hpp"

namespace suborno::layout {

struct LayoutMetadata {
    std::string id;
    std::string display_name;
    std::string version = "1";
};

class LayoutRegistry {
  public:
    LayoutRegistry() = default;
    LayoutRegistry(const LayoutRegistry&) = delete;
    LayoutRegistry& operator=(const LayoutRegistry&) = delete;

    bool register_layout(std::string_view id, std::unique_ptr<InputLayout> layout);
    bool register_layout(std::string_view id,
                         std::unique_ptr<InputLayout> layout,
                         LayoutMetadata metadata);
    const LayoutMetadata* find_metadata(std::string_view id) const;
    bool set_active(std::string_view id);
    const InputLayout* find(std::string_view id) const;
    const InputLayout* active() const;
    std::string_view active_id() const noexcept;

  private:
    std::unordered_map<std::string, std::unique_ptr<InputLayout>> layouts_;
    std::unordered_map<std::string, LayoutMetadata> metadata_;
    std::string active_id_;
};

}  // namespace suborno::layout

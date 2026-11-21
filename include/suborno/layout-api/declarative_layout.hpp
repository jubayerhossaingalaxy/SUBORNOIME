#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include "suborno/layout-api/phonetic_layout.hpp"

namespace suborno::layout {

class DeclarativeLayout final : public PhoneticLayout {
  public:
    DeclarativeLayout(std::string id, std::filesystem::path mapping_file);

    bool load();
    const std::string& id() const noexcept;
    std::string transliterate(const std::string& input) const override;
    std::vector<Candidate> generate_candidates(const std::string& input) const override;

  private:
    std::string id_;
    std::filesystem::path mapping_file_;
    std::unordered_map<std::string, std::string> mapping_;
};

}  // namespace suborno::layout

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "suborno/layout-api/candidate.hpp"
#include "suborno/layout-api/input_session.hpp"

namespace suborno::dictionary {
class Dictionary;
}

namespace suborno::layout {

enum class LayoutKind {
    phonetic,
    fixed,
    compositional,
};

class InputLayout {
  public:
    virtual ~InputLayout() = default;

    virtual LayoutKind kind() const noexcept = 0;
    virtual std::string transliterate(const std::string& input) const = 0;
    virtual std::vector<Candidate> generate_candidates(const std::string& input) const = 0;
    virtual std::unique_ptr<InputSession> create_session(
        dictionary::Dictionary* dictionary,
        InputSessionOptions options) const = 0;
};

// Stateless layouts rebuild their output from the complete key sequence.
// Stateful compositional layouts may implement their own InputSession.
class StatelessInputLayout : public InputLayout {
  public:
    std::unique_ptr<InputSession> create_session(
        dictionary::Dictionary* dictionary,
        InputSessionOptions options) const override;
};

}  // namespace suborno::layout

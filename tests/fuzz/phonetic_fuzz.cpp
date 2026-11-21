#include <cstddef>
#include <cstdint>
#include <string>

#include "suborno/layouts/phonetic/phonetic_input_session.hpp"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, std::size_t size) {
    if (data == nullptr) {
        return 0;
    }

    const std::string input(reinterpret_cast<const char*>(data), size);
    suborno::layouts::PhoneticInputSession session;
    session.append_utf8(input);
    session.snapshot();
    session.backspace();
    const auto committed = session.commit();
    (void)committed;
    session.reset();
    return 0;
}

#include <cstdlib>
#include <cstring>
#include <limits>
#include <memory>
#include <utility>

#include "suborno/core/engine.hpp"
#include "suborno/suborno.h"

struct SUBORNO_Engine {
    std::shared_ptr<suborno::core::Engine> engine = std::make_shared<suborno::core::Engine>();
};

struct SUBORNO_Session {
    std::shared_ptr<suborno::core::Engine> engine_owner;
    std::unique_ptr<suborno::layout::InputSession> session;
};

namespace {

int to_c_status(suborno::layout::InputStatus status) noexcept {
    using suborno::layout::InputStatus;
    switch (status) {
    case InputStatus::updated:
        return SUBORNO_STATUS_UPDATED;
    case InputStatus::pending:
        return SUBORNO_STATUS_PENDING;
    case InputStatus::literal_passthrough:
        return SUBORNO_STATUS_LITERAL_PASSTHROUGH;
    case InputStatus::invalid_utf8:
        return SUBORNO_STATUS_INVALID_UTF8;
    case InputStatus::input_limit:
        return SUBORNO_STATUS_INPUT_LIMIT;
    case InputStatus::candidate_out_of_range:
        return SUBORNO_STATUS_CANDIDATE_OUT_OF_RANGE;
    case InputStatus::empty:
        return SUBORNO_STATUS_EMPTY;
    }
    return SUBORNO_ERROR_INTERNAL;
}

template <typename Function>
int protect_c_boundary(Function&& function) noexcept {
    try {
        return function();
    } catch (...) {
        return SUBORNO_ERROR_INTERNAL;
    }
}

int copy_commit_result(suborno::layout::CommitResult result,
                       char** committed,
                       std::size_t* committed_size) {
    if (result.text_utf8.size() == std::numeric_limits<std::size_t>::max()) {
        return SUBORNO_ERROR_INTERNAL;
    }
    char* output = static_cast<char*>(std::malloc(result.text_utf8.size() + 1));
    if (output == nullptr) {
        return SUBORNO_ERROR_INTERNAL;
    }
    std::memcpy(output, result.text_utf8.data(), result.text_utf8.size());
    output[result.text_utf8.size()] = '\0';
    *committed = output;
    *committed_size = result.text_utf8.size();
    return to_c_status(result.status);
}

int copy_text_to_buffer(const std::string& text, char* buffer, std::size_t buffer_size) noexcept {
    if (text.size() >= buffer_size) {
        return SUBORNO_ERROR_BUFFER_TOO_SMALL;
    }
    std::memcpy(buffer, text.data(), text.size());
    buffer[text.size()] = '\0';
    return SUBORNO_STATUS_UPDATED;
}

}  // namespace

extern "C" {

SUBORNO_Engine* suborno_engine_create(void) {
    try {
        return new SUBORNO_Engine{};
    } catch (...) {
        return nullptr;
    }
}

void suborno_engine_destroy(SUBORNO_Engine* engine) {
    delete engine;
}

SUBORNO_Session* suborno_session_create(SUBORNO_Engine* engine) {
    if (engine == nullptr) {
        return nullptr;
    }
    try {
        const std::shared_ptr<suborno::core::Engine> engine_owner = engine->engine;
        auto session = engine_owner->create_session();
        if (session == nullptr) {
            return nullptr;
        }
        return new SUBORNO_Session{engine_owner, std::move(session)};
    } catch (...) {
        return nullptr;
    }
}

void suborno_session_destroy(SUBORNO_Session* session) {
    delete session;
}

int suborno_session_append_utf8(SUBORNO_Session* session, const char* input) {
    if (session == nullptr || input == nullptr) {
        return SUBORNO_ERROR_INVALID_ARGUMENT;
    }
    return protect_c_boundary([&] {
        return to_c_status(session->session->append_utf8(input));
    });
}

int suborno_session_backspace(SUBORNO_Session* session) {
    if (session == nullptr) {
        return SUBORNO_ERROR_INVALID_ARGUMENT;
    }
    return protect_c_boundary([&] {
        return to_c_status(session->session->backspace());
    });
}

int suborno_session_snapshot(SUBORNO_Session* session, char* buffer, size_t buffer_size) {
    if (session == nullptr || buffer == nullptr || buffer_size == 0) {
        return SUBORNO_ERROR_INVALID_ARGUMENT;
    }
    buffer[0] = '\0';
    return protect_c_boundary([&]() -> int {
        const auto snapshot = session->session->snapshot();
        if (snapshot.preedit_utf8.size() >= buffer_size) {
            return SUBORNO_ERROR_BUFFER_TOO_SMALL;
        }
        std::memcpy(buffer, snapshot.preedit_utf8.data(), snapshot.preedit_utf8.size());
        buffer[snapshot.preedit_utf8.size()] = '\0';
        return to_c_status(snapshot.status);
    });
}

int suborno_session_candidate_count(SUBORNO_Session* session, size_t* count) {
    if (count != nullptr) {
        *count = 0;
    }
    if (session == nullptr || count == nullptr) {
        return SUBORNO_ERROR_INVALID_ARGUMENT;
    }
    return protect_c_boundary([&]() -> int {
        const auto snapshot = session->session->snapshot();
        *count = snapshot.candidates.size();
        return to_c_status(snapshot.status);
    });
}

int suborno_session_candidate_snapshot(SUBORNO_Session* session,
                                       size_t index,
                                       char* buffer,
                                       size_t buffer_size) {
    if (session == nullptr || buffer == nullptr || buffer_size == 0) {
        return SUBORNO_ERROR_INVALID_ARGUMENT;
    }
    buffer[0] = '\0';
    return protect_c_boundary([&]() -> int {
        const auto snapshot = session->session->snapshot();
        if (index >= snapshot.candidates.size()) {
            return SUBORNO_STATUS_CANDIDATE_OUT_OF_RANGE;
        }
        return copy_text_to_buffer(snapshot.candidates[index].text, buffer, buffer_size);
    });
}

int suborno_session_select_candidate(SUBORNO_Session* session, size_t index) {
    if (session == nullptr) {
        return SUBORNO_ERROR_INVALID_ARGUMENT;
    }
    return protect_c_boundary([&] {
        return session->session->select_candidate(index) ? SUBORNO_STATUS_UPDATED
                                                         : SUBORNO_STATUS_CANDIDATE_OUT_OF_RANGE;
    });
}

int suborno_session_commit(SUBORNO_Session* session, char** committed, size_t* committed_size) {
    if (committed != nullptr) {
        *committed = nullptr;
    }
    if (committed_size != nullptr) {
        *committed_size = 0;
    }
    if (session == nullptr || committed == nullptr || committed_size == nullptr) {
        return SUBORNO_ERROR_INVALID_ARGUMENT;
    }
    return protect_c_boundary([&] {
        return copy_commit_result(session->session->commit(), committed, committed_size);
    });
}

int suborno_session_commit_candidate(SUBORNO_Session* session,
                                     size_t index,
                                     char** committed,
                                     size_t* committed_size) {
    if (committed != nullptr) {
        *committed = nullptr;
    }
    if (committed_size != nullptr) {
        *committed_size = 0;
    }
    if (session == nullptr || committed == nullptr || committed_size == nullptr) {
        return SUBORNO_ERROR_INVALID_ARGUMENT;
    }
    return protect_c_boundary([&] {
        return copy_commit_result(
            session->session->commit_candidate(index), committed, committed_size);
    });
}

void suborno_string_free(char* text) {
    std::free(text);
}

}  // extern "C"

#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace suborno::dictionary {

enum class DictionaryErrorCategory {
    open,
    schema,
    prepare,
    bind,
    step,
    integrity,
};

class DictionaryError final : public std::runtime_error {
  public:
    DictionaryError(DictionaryErrorCategory category, int sqlite_code, const std::string& message);

    DictionaryErrorCategory category() const noexcept;
    int sqlite_code() const noexcept;

  private:
    DictionaryErrorCategory category_;
    int sqlite_code_;
};

class Dictionary {
  public:
    // database_path is UTF-8 encoded on every platform.
    explicit Dictionary(const std::string& database_path);
    ~Dictionary();
    Dictionary(const Dictionary&) = delete;
    Dictionary& operator=(const Dictionary&) = delete;

    void add_entry(const std::string& word, const std::string& normalized, int frequency = 1);
    std::size_t import_word_lists_if_needed(
        const std::vector<std::filesystem::path>& word_list_paths,
        std::string_view corpus_version);
    void record_commit(const std::string& word, const std::string& normalized);
    void record_commit(const std::string& word,
                       const std::string& canonical_normalized,
                       const std::string& user_alias);
    void clear_user_entries();
    int schema_version() const;
    std::vector<std::string> find_matches(const std::string& normalized,
                                          std::size_t limit = 16) const;
    std::vector<std::string> find_prefix_matches(const std::string& normalized_prefix,
                                                 std::size_t limit = 10) const;
    std::vector<std::string> words_for_prefix(const std::string& prefix) const;
    void initialize_schema();

    // Personal dictionary management
    void add_user_word(const std::string& word, const std::string& normalized);
    void remove_user_word(const std::string& word);
    std::vector<std::string> list_user_words() const;
    int get_user_frequency(const std::string& word) const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace suborno::dictionary

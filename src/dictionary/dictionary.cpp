#include "suborno/dictionary/dictionary.hpp"

#include <algorithm>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sqlite3.h>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "suborno/unicode/bengali.hpp"

namespace suborno::dictionary {
namespace {

std::string read_dictionary_metadata(sqlite3* database, const char* key) {
    sqlite3_stmt* statement = nullptr;
    const int prepare_result = sqlite3_prepare_v2(
        database, "SELECT value FROM dictionary_metadata WHERE key = ?", -1, &statement, nullptr);
    if (prepare_result != SQLITE_OK) {
        throw DictionaryError(DictionaryErrorCategory::prepare,
                              prepare_result,
                              sqlite3_errmsg(database));
    }

    if (sqlite3_bind_text(statement, 1, key, -1, SQLITE_STATIC) != SQLITE_OK) {
        sqlite3_finalize(statement);
        throw DictionaryError(DictionaryErrorCategory::bind,
                              sqlite3_extended_errcode(database),
                              "Unable to bind dictionary metadata key");
    }

    const int step_result = sqlite3_step(statement);
    std::string value;
    if (step_result == SQLITE_ROW) {
        const auto* text = sqlite3_column_text(statement, 0);
        if (text != nullptr) {
            value = reinterpret_cast<const char*>(text);
        }
    } else if (step_result != SQLITE_DONE) {
        sqlite3_finalize(statement);
        throw DictionaryError(DictionaryErrorCategory::step,
                              step_result,
                              sqlite3_errmsg(database));
    }

    sqlite3_finalize(statement);
    return value;
}

void rollback_dictionary_transaction(sqlite3* database) noexcept {
    static_cast<void>(sqlite3_exec(database, "ROLLBACK;", nullptr, nullptr, nullptr));
}

std::string escape_like_prefix(const std::string& prefix) {
    std::string escaped;
    escaped.reserve(prefix.size() + 1);
    for (const char character : prefix) {
        if (character == '\\' || character == '%' || character == '_') {
            escaped.push_back('\\');
        }
        escaped.push_back(character);
    }
    escaped.push_back('%');
    return escaped;
}

void execute_two_text_statement(sqlite3* database,
                                const char* sql,
                                const std::string& first,
                                const std::string& second,
                                const char* operation) {
    sqlite3_stmt* statement = nullptr;
    const int prepare_rc = sqlite3_prepare_v2(database, sql, -1, &statement, nullptr);
    if (prepare_rc != SQLITE_OK) {
        throw DictionaryError(
            DictionaryErrorCategory::prepare, prepare_rc, sqlite3_errmsg(database));
    }

    if (sqlite3_bind_text(statement, 1, first.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK
        || sqlite3_bind_text(statement, 2, second.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
        sqlite3_finalize(statement);
        throw DictionaryError(DictionaryErrorCategory::bind,
                              sqlite3_extended_errcode(database),
                              std::string("Unable to bind ") + operation);
    }

    const int step_rc = sqlite3_step(statement);
    sqlite3_finalize(statement);
    if (step_rc != SQLITE_DONE) {
        throw DictionaryError(DictionaryErrorCategory::step, step_rc, sqlite3_errmsg(database));
    }
}

void begin_dictionary_transaction(sqlite3* database) {
    char* error = nullptr;
    const int rc = sqlite3_exec(database, "BEGIN IMMEDIATE;", nullptr, nullptr, &error);
    if (rc != SQLITE_OK) {
        const std::string message = error == nullptr ? "unknown SQLite error" : error;
        sqlite3_free(error);
        throw DictionaryError(DictionaryErrorCategory::step, rc, message);
    }
}

void commit_dictionary_transaction(sqlite3* database) {
    char* error = nullptr;
    const int rc = sqlite3_exec(database, "COMMIT;", nullptr, nullptr, &error);
    if (rc != SQLITE_OK) {
        const std::string message = error == nullptr ? "unknown SQLite error" : error;
        sqlite3_free(error);
        throw DictionaryError(DictionaryErrorCategory::step, rc, message);
    }
}

}  // namespace

DictionaryError::DictionaryError(DictionaryErrorCategory category,
                                 int sqlite_code,
                                 const std::string& message)
    : std::runtime_error(message),
      category_(category),
      sqlite_code_(sqlite_code) {}

DictionaryErrorCategory DictionaryError::category() const noexcept {
    return category_;
}

int DictionaryError::sqlite_code() const noexcept {
    return sqlite_code_;
}

struct Dictionary::Impl {
    sqlite3* db = nullptr;

    explicit Impl(const std::string& path) {
        const std::filesystem::path database_path = std::filesystem::u8path(path);
        std::error_code directory_error;
        if (!database_path.parent_path().empty()) {
            std::filesystem::create_directories(database_path.parent_path(), directory_error);
            if (directory_error) {
                throw DictionaryError(DictionaryErrorCategory::open,
                                      SQLITE_CANTOPEN,
                                      "Unable to create dictionary directory");
            }
        }

        if (sqlite3_open(path.c_str(), &db) != SQLITE_OK) {
            const int code = db == nullptr ? SQLITE_CANTOPEN : sqlite3_extended_errcode(db);
            const char* text = db == nullptr ? nullptr : sqlite3_errmsg(db);
            if (db != nullptr) {
                sqlite3_close(db);
                db = nullptr;
            }
            throw DictionaryError(
                DictionaryErrorCategory::open,
                code,
                text == nullptr ? "Unable to open dictionary database" : std::string(text));
        }

        sqlite3_busy_timeout(db, 30000);
    }

    ~Impl() {
        if (db != nullptr) {
            sqlite3_close(db);
            db = nullptr;
        }
    }
};

Dictionary::Dictionary(const std::string& database_path)
    : impl_(std::make_unique<Impl>(database_path)) {
    initialize_schema();
}

Dictionary::~Dictionary() = default;

void Dictionary::initialize_schema() {
    sqlite3_stmt* metadata_stmt = nullptr;
    const int metadata_prepare_rc = sqlite3_prepare_v2(
        impl_->db,
        "SELECT 1 FROM sqlite_master WHERE type='table' AND name='dictionary_metadata' LIMIT 1",
        -1,
        &metadata_stmt,
        nullptr);
    if (metadata_prepare_rc != SQLITE_OK) {
        throw DictionaryError(DictionaryErrorCategory::schema,
                              metadata_prepare_rc,
                              "Unable to inspect dictionary metadata table");
    }
    const int metadata_step_rc = sqlite3_step(metadata_stmt);
    sqlite3_finalize(metadata_stmt);
    if (metadata_step_rc == SQLITE_ROW && schema_version() > 3) {
        throw DictionaryError(DictionaryErrorCategory::schema,
                              SQLITE_SCHEMA,
                              "Dictionary database schema is newer than this engine");
    }
    if (metadata_step_rc != SQLITE_ROW && metadata_step_rc != SQLITE_DONE) {
        throw DictionaryError(DictionaryErrorCategory::step,
                              metadata_step_rc,
                              "Unable to inspect dictionary metadata table");
    }

    const char* sql =
        "CREATE TABLE IF NOT EXISTS dictionary_metadata ("
        "  key TEXT PRIMARY KEY,"
        "  value TEXT NOT NULL);"
        "INSERT OR IGNORE INTO dictionary_metadata(key, value) VALUES('schema_version', '3');"
        "CREATE TABLE IF NOT EXISTS dictionary_entries ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  word TEXT NOT NULL UNIQUE,"
        "  normalized TEXT NOT NULL,"
        "  frequency INTEGER NOT NULL DEFAULT 1,"
        "  source TEXT NOT NULL DEFAULT 'system',"
        "  user_frequency INTEGER NOT NULL DEFAULT 0,"
        "  user_pinned INTEGER NOT NULL DEFAULT 0,"
        "  created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP);"
        "CREATE INDEX IF NOT EXISTS idx_dictionary_normalized ON dictionary_entries(normalized);"
        "CREATE INDEX IF NOT EXISTS idx_dictionary_normalized_prefix "
        "ON dictionary_entries(normalized COLLATE NOCASE);"
        "CREATE TABLE IF NOT EXISTS dictionary_user_aliases ("
        "  word TEXT NOT NULL,"
        "  normalized TEXT NOT NULL,"
        "  frequency INTEGER NOT NULL DEFAULT 1,"
        "  PRIMARY KEY(word, normalized));"
        "CREATE INDEX IF NOT EXISTS idx_dictionary_user_alias_normalized "
        "ON dictionary_user_aliases(normalized COLLATE NOCASE);";

    char* error = nullptr;
    const int rc = sqlite3_exec(impl_->db, sql, nullptr, nullptr, &error);
    if (rc != SQLITE_OK) {
        std::string message = error == nullptr ? "unknown SQLite error" : error;
        sqlite3_free(error);
        throw DictionaryError(DictionaryErrorCategory::schema,
                              rc,
                              "Unable to initialize dictionary schema: " + message);
    }

    sqlite3_stmt* column_stmt = nullptr;
    const int prepare_rc = sqlite3_prepare_v2(
        impl_->db, "PRAGMA table_info(dictionary_entries)", -1, &column_stmt, nullptr);
    if (prepare_rc != SQLITE_OK) {
        throw DictionaryError(
            DictionaryErrorCategory::schema, prepare_rc, "Unable to inspect dictionary schema");
    }

    bool has_source = false;
    bool has_user_frequency = false;
    bool has_user_pinned = false;
    int step_rc = SQLITE_OK;
    while ((step_rc = sqlite3_step(column_stmt)) == SQLITE_ROW) {
        const unsigned char* name = sqlite3_column_text(column_stmt, 1);
        if (name != nullptr && std::string(reinterpret_cast<const char*>(name)) == "source") {
            has_source = true;
        }
        if (name != nullptr
            && std::string(reinterpret_cast<const char*>(name)) == "user_frequency") {
            has_user_frequency = true;
        }
        if (name != nullptr && std::string(reinterpret_cast<const char*>(name)) == "user_pinned") {
            has_user_pinned = true;
        }
    }
    sqlite3_finalize(column_stmt);
    if (step_rc != SQLITE_DONE) {
        throw DictionaryError(
            DictionaryErrorCategory::step, step_rc, "Unable to inspect dictionary columns");
    }

    if (!has_source) {
        char* alter_error = nullptr;
        const int alter_rc = sqlite3_exec(impl_->db,
                                          "ALTER TABLE dictionary_entries "
                                          "ADD COLUMN source TEXT NOT NULL DEFAULT 'system';",
                                          nullptr,
                                          nullptr,
                                          &alter_error);
        if (alter_rc != SQLITE_OK) {
            std::string message = alter_error == nullptr ? "unknown SQLite error" : alter_error;
            sqlite3_free(alter_error);
            throw DictionaryError(DictionaryErrorCategory::schema,
                                  alter_rc,
                                  "Unable to migrate dictionary schema: " + message);
        }
    }

    if (!has_user_frequency) {
        char* alter_error = nullptr;
        const int alter_rc = sqlite3_exec(impl_->db,
                                          "ALTER TABLE dictionary_entries "
                                          "ADD COLUMN user_frequency INTEGER NOT NULL DEFAULT 0;",
                                          nullptr,
                                          nullptr,
                                          &alter_error);
        if (alter_rc != SQLITE_OK) {
            std::string message = alter_error == nullptr ? "unknown SQLite error" : alter_error;
            sqlite3_free(alter_error);
            throw DictionaryError(DictionaryErrorCategory::schema,
                                  alter_rc,
                                  "Unable to migrate dictionary schema: " + message);
        }
    }

    if (!has_user_pinned) {
        char* alter_error = nullptr;
        const int alter_rc = sqlite3_exec(impl_->db,
                                          "ALTER TABLE dictionary_entries "
                                          "ADD COLUMN user_pinned INTEGER NOT NULL DEFAULT 0;",
                                          nullptr,
                                          nullptr,
                                          &alter_error);
        if (alter_rc != SQLITE_OK) {
            std::string message = alter_error == nullptr ? "unknown SQLite error" : alter_error;
            sqlite3_free(alter_error);
            throw DictionaryError(DictionaryErrorCategory::schema,
                                  alter_rc,
                                  "Unable to migrate dictionary schema: " + message);
        }
    }

    char* version_error = nullptr;
    const int version_update_rc = sqlite3_exec(impl_->db,
                                               "UPDATE dictionary_metadata SET value='2' "
                                               "WHERE key='schema_version' AND value='1';",
                                               nullptr,
                                               nullptr,
                                               &version_error);
    if (version_update_rc != SQLITE_OK) {
        std::string message = version_error == nullptr ? "unknown SQLite error" : version_error;
        sqlite3_free(version_error);
        throw DictionaryError(DictionaryErrorCategory::schema,
                              version_update_rc,
                              "Unable to update dictionary schema version: " + message);
    }
    char* alias_migration_error = nullptr;
    const int alias_migration_rc = sqlite3_exec(
        impl_->db,
        "INSERT OR IGNORE INTO dictionary_user_aliases(word, normalized, frequency) "
        "SELECT word, normalized, CASE WHEN user_frequency > 0 THEN user_frequency ELSE 1 END "
        "FROM dictionary_entries "
        "WHERE user_frequency > 0 OR source='user' OR user_pinned=1;",
        nullptr,
        nullptr,
        &alias_migration_error);
    if (alias_migration_rc != SQLITE_OK) {
        const std::string message =
            alias_migration_error == nullptr ? "unknown SQLite error" : alias_migration_error;
        sqlite3_free(alias_migration_error);
        throw DictionaryError(DictionaryErrorCategory::schema,
                              alias_migration_rc,
                              "Unable to migrate learned dictionary aliases: " + message);
    }

    char* version_three_error = nullptr;
    const int version_three_rc = sqlite3_exec(impl_->db,
                                              "UPDATE dictionary_metadata SET value='3' "
                                              "WHERE key='schema_version' AND value='2';",
                                              nullptr,
                                              nullptr,
                                              &version_three_error);
    if (version_three_rc != SQLITE_OK) {
        const std::string message =
            version_three_error == nullptr ? "unknown SQLite error" : version_three_error;
        sqlite3_free(version_three_error);
        throw DictionaryError(DictionaryErrorCategory::schema,
                              version_three_rc,
                              "Unable to update dictionary schema version: " + message);
    }

    if (schema_version() > 3) {
        throw DictionaryError(DictionaryErrorCategory::schema,
                              SQLITE_SCHEMA,
                              "Dictionary database schema is newer than this engine");
    }
}

std::size_t Dictionary::import_word_lists_if_needed(const WordListPaths& word_list_paths,
                                                    std::string_view corpus_version) {
    if (word_list_paths.empty() || corpus_version.empty() || corpus_version.size() > 128
        || corpus_version.find_first_of("\r\n") != std::string_view::npos) {
        throw DictionaryError(
            DictionaryErrorCategory::integrity, SQLITE_MISUSE, "Invalid system wordlist metadata");
    }

    constexpr char kImportMetadataKey[] = "system_wordlist_version";
    const std::string requested_version(corpus_version);
    if (read_dictionary_metadata(impl_->db, kImportMetadataKey) == requested_version) {
        return 0;
    }

    begin_dictionary_transaction(impl_->db);
    sqlite3_stmt* insert_statement = nullptr;
    try {
        // Another Suborno process may have completed the same import while this one waited
        // for SQLite's write lock.
        if (read_dictionary_metadata(impl_->db, kImportMetadataKey) == requested_version) {
            commit_dictionary_transaction(impl_->db);
            return 0;
        }

        constexpr char kInsertSql[] =
            "INSERT OR IGNORE INTO dictionary_entries(word, normalized, frequency, source) "
            "VALUES(?, ?, 1, 'system')";
        const int prepare_result =
            sqlite3_prepare_v2(impl_->db, kInsertSql, -1, &insert_statement, nullptr);
        if (prepare_result != SQLITE_OK) {
            throw DictionaryError(
                DictionaryErrorCategory::prepare, prepare_result, sqlite3_errmsg(impl_->db));
        }

        std::size_t imported_count = 0;
        for (const std::filesystem::path& word_list_path : word_list_paths) {
            std::ifstream input(word_list_path, std::ios::binary);
            if (!input.is_open()) {
                throw DictionaryError(DictionaryErrorCategory::open,
                                      SQLITE_CANTOPEN,
                                      "Unable to open the bundled Bengali wordlist");
            }

            std::string word;
            while (std::getline(input, word)) {
                if (!word.empty() && word.back() == '\r') {
                    word.pop_back();
                }
                if (word.empty()) {
                    continue;
                }
                if (word.size() > 512 || !suborno::unicode::is_valid_utf8(word)) {
                    throw DictionaryError(DictionaryErrorCategory::integrity,
                                          SQLITE_MISMATCH,
                                          "The bundled Bengali wordlist contains invalid UTF-8");
                }

                const std::string normalized = suborno::unicode::normalize_nfc(word);
                if (normalized.empty()) {
                    continue;
                }

                if (sqlite3_bind_text(insert_statement,
                                      1,
                                      word.data(),
                                      static_cast<int>(word.size()),
                                      SQLITE_TRANSIENT)
                        != SQLITE_OK
                    || sqlite3_bind_text(insert_statement,
                                         2,
                                         normalized.data(),
                                         static_cast<int>(normalized.size()),
                                         SQLITE_TRANSIENT)
                           != SQLITE_OK) {
                    throw DictionaryError(DictionaryErrorCategory::bind,
                                          sqlite3_extended_errcode(impl_->db),
                                          "Unable to bind bundled Bengali word");
                }

                const int step_result = sqlite3_step(insert_statement);
                if (step_result != SQLITE_DONE) {
                    throw DictionaryError(
                        DictionaryErrorCategory::step, step_result, sqlite3_errmsg(impl_->db));
                }
                if (sqlite3_changes(impl_->db) > 0) {
                    ++imported_count;
                }

                const int reset_result = sqlite3_reset(insert_statement);
                if (reset_result != SQLITE_OK
                    || sqlite3_clear_bindings(insert_statement) != SQLITE_OK) {
                    throw DictionaryError(DictionaryErrorCategory::step,
                                          sqlite3_extended_errcode(impl_->db),
                                          "Unable to reset bundled Bengali word insert");
                }
            }

            if (input.bad()) {
                throw DictionaryError(DictionaryErrorCategory::open,
                                      SQLITE_IOERR_READ,
                                      "Unable to read the bundled Bengali wordlist");
            }
        }

        sqlite3_finalize(insert_statement);
        insert_statement = nullptr;
        execute_two_text_statement(impl_->db,
                                   "INSERT INTO dictionary_metadata(key, value) VALUES(?, ?) "
                                   "ON CONFLICT(key) DO UPDATE SET value=excluded.value",
                                   kImportMetadataKey,
                                   requested_version,
                                   "system wordlist version");
        commit_dictionary_transaction(impl_->db);
        return imported_count;
    } catch (...) {
        if (insert_statement != nullptr) {
            sqlite3_finalize(insert_statement);
        }
        rollback_dictionary_transaction(impl_->db);
        throw;
    }
}

void Dictionary::add_entry(const std::string& word, const std::string& normalized, int frequency) {
    const std::string statement =
        "INSERT INTO dictionary_entries(word, normalized, frequency, source) "
        "VALUES(?, ?, ?, 'system') "
        "ON CONFLICT(word) DO UPDATE SET normalized=excluded.normalized,"
        " frequency=excluded.frequency, source='system'";
    sqlite3_stmt* stmt = nullptr;
    const int prepare_rc = sqlite3_prepare_v2(impl_->db, statement.c_str(), -1, &stmt, nullptr);
    if (prepare_rc != SQLITE_OK) {
        throw DictionaryError(
            DictionaryErrorCategory::prepare, prepare_rc, sqlite3_errmsg(impl_->db));
    }

    if (sqlite3_bind_text(stmt, 1, word.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK
        || sqlite3_bind_text(stmt, 2, normalized.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK
        || sqlite3_bind_int(stmt, 3, frequency) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        throw DictionaryError(DictionaryErrorCategory::bind,
                              sqlite3_extended_errcode(impl_->db),
                              "Unable to bind dictionary insert");
    }

    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        throw DictionaryError(DictionaryErrorCategory::step, rc, sqlite3_errmsg(impl_->db));
    }
}

void Dictionary::record_commit(const std::string& word, const std::string& normalized) {
    record_commit(word, normalized, normalized);
}

void Dictionary::record_commit(const std::string& word,
                               const std::string& canonical_normalized,
                               const std::string& user_alias) {
    begin_dictionary_transaction(impl_->db);
    try {
        execute_two_text_statement(
            impl_->db,
            "INSERT INTO dictionary_entries("
            "word, normalized, frequency, source, user_frequency) "
            "VALUES(?, ?, 1, 'user', 1) "
            "ON CONFLICT(word) DO UPDATE SET "
            "user_frequency=MIN(dictionary_entries.user_frequency+1, 2147483647);",
            word,
            canonical_normalized,
            "committed dictionary entry");
        execute_two_text_statement(
            impl_->db,
            "INSERT INTO dictionary_user_aliases(word, normalized, frequency) "
            "VALUES(?, ?, 1) "
            "ON CONFLICT(word, normalized) DO UPDATE SET "
            "frequency=MIN(dictionary_user_aliases.frequency+1, 2147483647);",
            word,
            user_alias,
            "committed dictionary alias");
        commit_dictionary_transaction(impl_->db);
    } catch (...) {
        sqlite3_exec(impl_->db, "ROLLBACK;", nullptr, nullptr, nullptr);
        throw;
    }
}

void Dictionary::clear_user_entries() {
    begin_dictionary_transaction(impl_->db);
    char* error = nullptr;
    const int rc = sqlite3_exec(impl_->db,
                                "DELETE FROM dictionary_user_aliases;"
                                "DELETE FROM dictionary_entries WHERE source='user';"
                                "UPDATE dictionary_entries "
                                "SET user_pinned=0, user_frequency=0;",
                                nullptr,
                                nullptr,
                                &error);
    if (rc != SQLITE_OK) {
        const std::string message = error == nullptr ? "unknown SQLite error" : error;
        sqlite3_free(error);
        sqlite3_exec(impl_->db, "ROLLBACK;", nullptr, nullptr, nullptr);
        throw DictionaryError(DictionaryErrorCategory::step, rc, message);
    }
    try {
        commit_dictionary_transaction(impl_->db);
    } catch (...) {
        sqlite3_exec(impl_->db, "ROLLBACK;", nullptr, nullptr, nullptr);
        throw;
    }
}

int Dictionary::schema_version() const {
    sqlite3_stmt* stmt = nullptr;
    const int prepare_rc =
        sqlite3_prepare_v2(impl_->db,
                           "SELECT value FROM dictionary_metadata WHERE key='schema_version'",
                           -1,
                           &stmt,
                           nullptr);
    if (prepare_rc != SQLITE_OK) {
        throw DictionaryError(
            DictionaryErrorCategory::prepare, prepare_rc, sqlite3_errmsg(impl_->db));
    }
    int version = 0;
    const int step_rc = sqlite3_step(stmt);
    if (step_rc == SQLITE_ROW) {
        const unsigned char* value = sqlite3_column_text(stmt, 0);
        if (value != nullptr) {
            const char* text = reinterpret_cast<const char*>(value);
            const char* text_end = text + sqlite3_column_bytes(stmt, 0);
            const auto [parsed_end, parse_error] = std::from_chars(text, text_end, version);
            if (parse_error != std::errc{} || parsed_end != text_end || version < 0) {
                sqlite3_finalize(stmt);
                throw DictionaryError(DictionaryErrorCategory::schema,
                                      SQLITE_MISMATCH,
                                      "Invalid dictionary schema version");
            }
        }
    } else if (step_rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw DictionaryError(DictionaryErrorCategory::step, step_rc, sqlite3_errmsg(impl_->db));
    }
    sqlite3_finalize(stmt);
    return version;
}

std::vector<std::string> Dictionary::find_matches(const std::string& normalized,
                                                  std::size_t limit) const {
    if (limit == 0) {
        return {};
    }

    const std::string query =
        "WITH matches AS ("
        "SELECT word, frequency + user_frequency AS score "
        "FROM dictionary_entries WHERE normalized = ? "
        "UNION ALL "
        "SELECT entries.word, entries.frequency + entries.user_frequency "
        "+ aliases.frequency AS score "
        "FROM dictionary_user_aliases AS aliases "
        "JOIN dictionary_entries AS entries ON entries.word = aliases.word "
        "WHERE aliases.normalized = ?) "
        "SELECT word FROM matches GROUP BY word "
        "ORDER BY MAX(score) DESC, word ASC LIMIT ?";
    sqlite3_stmt* stmt = nullptr;
    const int prepare_rc = sqlite3_prepare_v2(impl_->db, query.c_str(), -1, &stmt, nullptr);
    if (prepare_rc != SQLITE_OK) {
        throw DictionaryError(
            DictionaryErrorCategory::prepare, prepare_rc, sqlite3_errmsg(impl_->db));
    }

    const std::size_t bounded_limit =
        std::min(limit, static_cast<std::size_t>(std::numeric_limits<sqlite3_int64>::max()));
    if (sqlite3_bind_text(stmt, 1, normalized.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK
        || sqlite3_bind_text(stmt, 2, normalized.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK
        || sqlite3_bind_int64(stmt, 3, static_cast<sqlite3_int64>(bounded_limit)) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        throw DictionaryError(DictionaryErrorCategory::bind,
                              sqlite3_extended_errcode(impl_->db),
                              "Unable to bind dictionary lookup");
    }

    std::vector<std::string> results;
    int step_rc = SQLITE_OK;
    while ((step_rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const unsigned char* value = sqlite3_column_text(stmt, 0);
        if (value != nullptr) {
            results.emplace_back(reinterpret_cast<const char*>(value));
        }
    }
    if (step_rc != SQLITE_DONE) {
        const std::string message = sqlite3_errmsg(impl_->db);
        sqlite3_finalize(stmt);
        throw DictionaryError(DictionaryErrorCategory::step, step_rc, message);
    }
    sqlite3_finalize(stmt);
    return results;
}

std::vector<std::string> Dictionary::find_prefix_matches(const std::string& normalized_prefix,
                                                         std::size_t limit) const {
    if (normalized_prefix.empty() || limit == 0) {
        return {};
    }

    const std::string query =
        "WITH matches AS ("
        "SELECT word, frequency + user_frequency AS score "
        "FROM dictionary_entries WHERE normalized LIKE ? ESCAPE '\\' "
        "UNION ALL "
        "SELECT entries.word, entries.frequency + entries.user_frequency "
        "+ aliases.frequency AS score "
        "FROM dictionary_user_aliases AS aliases "
        "JOIN dictionary_entries AS entries ON entries.word = aliases.word "
        "WHERE aliases.normalized LIKE ? ESCAPE '\\') "
        "SELECT word FROM matches GROUP BY word "
        "ORDER BY MAX(score) DESC, word ASC LIMIT ?";
    sqlite3_stmt* stmt = nullptr;
    const int prepare_rc = sqlite3_prepare_v2(impl_->db, query.c_str(), -1, &stmt, nullptr);
    if (prepare_rc != SQLITE_OK) {
        throw DictionaryError(
            DictionaryErrorCategory::prepare, prepare_rc, sqlite3_errmsg(impl_->db));
    }

    const std::string pattern = escape_like_prefix(normalized_prefix);
    const std::size_t bounded_limit =
        std::min(limit, static_cast<std::size_t>(std::numeric_limits<sqlite3_int64>::max()));
    if (sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK
        || sqlite3_bind_text(stmt, 2, pattern.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK
        || sqlite3_bind_int64(stmt, 3, static_cast<sqlite3_int64>(bounded_limit)) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        throw DictionaryError(DictionaryErrorCategory::bind,
                              sqlite3_extended_errcode(impl_->db),
                              "Unable to bind dictionary normalized prefix lookup");
    }

    std::vector<std::string> results;
    int step_rc = SQLITE_OK;
    while ((step_rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const unsigned char* value = sqlite3_column_text(stmt, 0);
        if (value != nullptr) {
            results.emplace_back(reinterpret_cast<const char*>(value));
        }
    }
    if (step_rc != SQLITE_DONE) {
        const std::string message = sqlite3_errmsg(impl_->db);
        sqlite3_finalize(stmt);
        throw DictionaryError(DictionaryErrorCategory::step, step_rc, message);
    }
    sqlite3_finalize(stmt);
    return results;
}

std::vector<std::string> Dictionary::words_for_prefix(const std::string& prefix) const {
    const std::string query =
        "SELECT word FROM dictionary_entries WHERE word LIKE ? ESCAPE '\\' "
        "ORDER BY (frequency + COALESCE(user_frequency, 0)) DESC, word ASC LIMIT 10";
    sqlite3_stmt* stmt = nullptr;
    const int prepare_rc = sqlite3_prepare_v2(impl_->db, query.c_str(), -1, &stmt, nullptr);
    if (prepare_rc != SQLITE_OK) {
        throw DictionaryError(
            DictionaryErrorCategory::prepare, prepare_rc, sqlite3_errmsg(impl_->db));
    }

    const std::string pattern = escape_like_prefix(prefix);
    if (sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        throw DictionaryError(DictionaryErrorCategory::bind,
                              sqlite3_extended_errcode(impl_->db),
                              "Unable to bind dictionary prefix lookup");
    }

    std::vector<std::string> results;
    int step_rc = SQLITE_OK;
    while ((step_rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const unsigned char* value = sqlite3_column_text(stmt, 0);
        if (value != nullptr) {
            results.emplace_back(reinterpret_cast<const char*>(value));
        }
    }
    if (step_rc != SQLITE_DONE) {
        const std::string message = sqlite3_errmsg(impl_->db);
        sqlite3_finalize(stmt);
        throw DictionaryError(DictionaryErrorCategory::step, step_rc, message);
    }
    sqlite3_finalize(stmt);
    return results;
}

void Dictionary::add_user_word(const std::string& word, const std::string& normalized) {
    begin_dictionary_transaction(impl_->db);
    try {
        execute_two_text_statement(
            impl_->db,
            "INSERT INTO dictionary_entries("
            "word, normalized, frequency, source, user_frequency, user_pinned) "
            "VALUES(?, ?, 1, 'user', 1, 1) "
            "ON CONFLICT(word) DO UPDATE SET "
            "user_frequency=MIN(dictionary_entries.user_frequency+1, 2147483647),"
            "user_pinned=1;",
            word,
            normalized,
            "user dictionary entry");
        execute_two_text_statement(
            impl_->db,
            "INSERT INTO dictionary_user_aliases(word, normalized, frequency) "
            "VALUES(?, ?, 1) "
            "ON CONFLICT(word, normalized) DO UPDATE SET "
            "frequency=MIN(dictionary_user_aliases.frequency+1, 2147483647);",
            word,
            normalized,
            "user dictionary alias");
        commit_dictionary_transaction(impl_->db);
    } catch (...) {
        sqlite3_exec(impl_->db, "ROLLBACK;", nullptr, nullptr, nullptr);
        throw;
    }
}

void Dictionary::remove_user_word(const std::string& word) {
    char* error = nullptr;
    const int begin_rc = sqlite3_exec(impl_->db, "BEGIN IMMEDIATE;", nullptr, nullptr, &error);
    if (begin_rc != SQLITE_OK) {
        std::string message = error == nullptr ? "unknown SQLite error" : error;
        sqlite3_free(error);
        throw DictionaryError(DictionaryErrorCategory::step, begin_rc, message);
    }

    const auto execute_word_statement = [this, &word](const char* sql, const char* operation) {
        sqlite3_stmt* stmt = nullptr;
        const int prepare_rc = sqlite3_prepare_v2(impl_->db, sql, -1, &stmt, nullptr);
        if (prepare_rc != SQLITE_OK) {
            throw DictionaryError(
                DictionaryErrorCategory::prepare, prepare_rc, sqlite3_errmsg(impl_->db));
        }
        if (sqlite3_bind_text(stmt, 1, word.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
            sqlite3_finalize(stmt);
            throw DictionaryError(DictionaryErrorCategory::bind,
                                  sqlite3_extended_errcode(impl_->db),
                                  std::string("Unable to bind user word ") + operation);
        }
        const int step_rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        if (step_rc != SQLITE_DONE) {
            throw DictionaryError(
                DictionaryErrorCategory::step, step_rc, sqlite3_errmsg(impl_->db));
        }
    };

    try {
        execute_word_statement("DELETE FROM dictionary_user_aliases WHERE word=?;",
                               "remove aliases");
        execute_word_statement(
            "UPDATE dictionary_entries SET user_pinned=0, user_frequency=0 "
            "WHERE word=? AND source='system';",
            "unpin");
        execute_word_statement("DELETE FROM dictionary_entries WHERE word=? AND source='user';",
                               "delete");
        char* commit_error = nullptr;
        const int commit_rc = sqlite3_exec(impl_->db, "COMMIT;", nullptr, nullptr, &commit_error);
        if (commit_rc != SQLITE_OK) {
            std::string message = commit_error == nullptr ? "unknown SQLite error" : commit_error;
            sqlite3_free(commit_error);
            throw DictionaryError(DictionaryErrorCategory::step, commit_rc, message);
        }
    } catch (...) {
        sqlite3_exec(impl_->db, "ROLLBACK;", nullptr, nullptr, nullptr);
        throw;
    }
}

std::vector<std::string> Dictionary::list_user_words() const {
    const std::string query =
        "SELECT word FROM dictionary_entries "
        "WHERE source = 'user' OR user_pinned = 1 OR user_frequency > 0 "
        "ORDER BY user_frequency DESC, word ASC";
    sqlite3_stmt* stmt = nullptr;
    const int prepare_rc = sqlite3_prepare_v2(impl_->db, query.c_str(), -1, &stmt, nullptr);
    if (prepare_rc != SQLITE_OK) {
        throw DictionaryError(
            DictionaryErrorCategory::prepare, prepare_rc, sqlite3_errmsg(impl_->db));
    }

    std::vector<std::string> results;
    int step_rc = SQLITE_OK;
    while ((step_rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const unsigned char* value = sqlite3_column_text(stmt, 0);
        if (value != nullptr) {
            results.emplace_back(reinterpret_cast<const char*>(value));
        }
    }
    if (step_rc != SQLITE_DONE) {
        const std::string message = sqlite3_errmsg(impl_->db);
        sqlite3_finalize(stmt);
        throw DictionaryError(DictionaryErrorCategory::step, step_rc, message);
    }
    sqlite3_finalize(stmt);
    return results;
}

int Dictionary::get_user_frequency(const std::string& word) const {
    const std::string query = "SELECT user_frequency FROM dictionary_entries WHERE word = ?";
    sqlite3_stmt* stmt = nullptr;
    const int prepare_rc = sqlite3_prepare_v2(impl_->db, query.c_str(), -1, &stmt, nullptr);
    if (prepare_rc != SQLITE_OK) {
        throw DictionaryError(
            DictionaryErrorCategory::prepare, prepare_rc, sqlite3_errmsg(impl_->db));
    }

    if (sqlite3_bind_text(stmt, 1, word.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
        sqlite3_finalize(stmt);
        throw DictionaryError(DictionaryErrorCategory::bind,
                              sqlite3_extended_errcode(impl_->db),
                              "Unable to bind user frequency query");
    }

    int frequency = 0;
    const int step_rc = sqlite3_step(stmt);
    if (step_rc == SQLITE_ROW) {
        frequency = sqlite3_column_int(stmt, 0);
    } else if (step_rc != SQLITE_DONE) {
        const std::string message = sqlite3_errmsg(impl_->db);
        sqlite3_finalize(stmt);
        throw DictionaryError(DictionaryErrorCategory::step, step_rc, message);
    }
    sqlite3_finalize(stmt);
    return frequency;
}

}  // namespace suborno::dictionary

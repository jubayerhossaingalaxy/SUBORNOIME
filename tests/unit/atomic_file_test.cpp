#include "persistence/atomic_file.hpp"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <iterator>
#include <string>

TEST(AtomicFileTest, ReplacesExistingFileWithCompleteContents) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "suborno_atomic_file_test.txt";
    std::error_code ignored;
    std::filesystem::remove(path, ignored);

    ASSERT_FALSE(suborno::persistence::write_file_atomically(path, "old"));
    ASSERT_FALSE(suborno::persistence::write_file_atomically(path, "new contents"));

    std::ifstream input(path, std::ios::binary);
    ASSERT_TRUE(input.is_open());
    const std::string contents{std::istreambuf_iterator<char>(input), {}};
    EXPECT_EQ(contents, "new contents");

    input.close();
    std::filesystem::remove(path, ignored);
}

TEST(AtomicFileTest, RejectsAnEmptyDestination) {
    const std::error_code error =
        suborno::persistence::write_file_atomically(std::filesystem::path{}, "data");
    EXPECT_EQ(error, std::make_error_code(std::errc::invalid_argument));
}

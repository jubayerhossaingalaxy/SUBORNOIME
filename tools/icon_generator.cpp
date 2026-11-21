#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

namespace {

struct Point {
    double x;
    double y;
};

struct Pixel {
    std::uint8_t red;
    std::uint8_t green;
    std::uint8_t blue;
    std::uint8_t alpha;
};

constexpr std::array<Point, 12> kMarkPath{{
    {0.66, 0.29},
    {0.57, 0.23},
    {0.43, 0.23},
    {0.33, 0.30},
    {0.32, 0.39},
    {0.40, 0.46},
    {0.60, 0.54},
    {0.68, 0.61},
    {0.67, 0.70},
    {0.57, 0.77},
    {0.42, 0.77},
    {0.32, 0.70},
}};

constexpr std::array<std::uint32_t, 7> kIconSizes{
    16U, 24U, 32U, 48U, 64U, 128U, 256U,
};

constexpr std::uint32_t kSupersamplingScale = 4U;

double distance_to_segment(Point point, Point start, Point end) noexcept {
    const double dx = end.x - start.x;
    const double dy = end.y - start.y;
    const double length_squared = dx * dx + dy * dy;
    const double projection = std::clamp(
        ((point.x - start.x) * dx + (point.y - start.y) * dy) / length_squared,
        0.0,
        1.0);
    const double nearest_x = start.x + projection * dx;
    const double nearest_y = start.y + projection * dy;
    const double delta_x = point.x - nearest_x;
    const double delta_y = point.y - nearest_y;
    return std::sqrt(delta_x * delta_x + delta_y * delta_y);
}

bool is_inside_rounded_square(Point point) noexcept {
    constexpr double radius = 0.22;
    const double edge = std::min(
        std::min(point.x, point.y),
        std::min(1.0 - point.x, 1.0 - point.y));

    if (edge >= radius) {
        return true;
    }

    const double center_x = point.x < radius ? radius : 1.0 - radius;
    const double center_y = point.y < radius ? radius : 1.0 - radius;
    const double delta_x = point.x - center_x;
    const double delta_y = point.y - center_y;
    return std::sqrt(delta_x * delta_x + delta_y * delta_y) <= radius;
}

Pixel render_sample(Point point, std::uint32_t size) noexcept {
    if (!is_inside_rounded_square(point)) {
        return {0U, 0U, 0U, 0U};
    }

    constexpr Pixel background{22U, 54U, 82U, 255U};
    constexpr Pixel mark{246U, 248U, 250U, 255U};
    constexpr Pixel accent{255U, 195U, 72U, 255U};
    const double thickness = std::max(0.035, 2.2 / static_cast<double>(size));

    for (std::size_t index = 0; index + 1U < kMarkPath.size(); ++index) {
        if (distance_to_segment(point, kMarkPath[index], kMarkPath[index + 1U]) < thickness) {
            return mark;
        }
    }

    constexpr Point accent_center{0.72, 0.27};
    constexpr double accent_radius = 0.045;
    const double accent_delta_x = point.x - accent_center.x;
    const double accent_delta_y = point.y - accent_center.y;
    if (std::sqrt(accent_delta_x * accent_delta_x + accent_delta_y * accent_delta_y)
        < accent_radius) {
        return accent;
    }

    return background;
}

std::vector<Pixel> render_icon(std::uint32_t size) {
    std::vector<Pixel> pixels(static_cast<std::size_t>(size) * size);
    const std::uint32_t sample_count = kSupersamplingScale * kSupersamplingScale;

    for (std::uint32_t y = 0; y < size; ++y) {
        for (std::uint32_t x = 0; x < size; ++x) {
            std::array<std::uint32_t, 4> totals{};
            for (std::uint32_t sample_y = 0; sample_y < kSupersamplingScale; ++sample_y) {
                for (std::uint32_t sample_x = 0; sample_x < kSupersamplingScale; ++sample_x) {
                    const double px = (
                        static_cast<double>(x)
                        + (static_cast<double>(sample_x) + 0.5) / kSupersamplingScale)
                        / static_cast<double>(size);
                    const double py = (
                        static_cast<double>(y)
                        + (static_cast<double>(sample_y) + 0.5) / kSupersamplingScale)
                        / static_cast<double>(size);
                    const Pixel sample = render_sample({px, py}, size);
                    totals[0] += sample.red;
                    totals[1] += sample.green;
                    totals[2] += sample.blue;
                    totals[3] += sample.alpha;
                }
            }

            pixels[static_cast<std::size_t>(y) * size + x] = {
                static_cast<std::uint8_t>(totals[0] / sample_count),
                static_cast<std::uint8_t>(totals[1] / sample_count),
                static_cast<std::uint8_t>(totals[2] / sample_count),
                static_cast<std::uint8_t>(totals[3] / sample_count),
            };
        }
    }

    return pixels;
}

void write_u8(std::ofstream& output, std::uint8_t value) {
    output.put(static_cast<char>(value));
}

void write_u16(std::ofstream& output, std::uint16_t value) {
    write_u8(output, static_cast<std::uint8_t>(value & 0xFFU));
    write_u8(output, static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
}

void write_u32(std::ofstream& output, std::uint32_t value) {
    write_u16(output, static_cast<std::uint16_t>(value & 0xFFFFU));
    write_u16(output, static_cast<std::uint16_t>((value >> 16U) & 0xFFFFU));
}

void write_bitmap(std::ofstream& output, std::uint32_t size, const std::vector<Pixel>& pixels) {
    const std::uint32_t xor_stride = size * 4U;
    const std::uint32_t mask_stride = ((size + 31U) / 32U) * 4U;
    const std::uint32_t xor_size = xor_stride * size;
    const std::uint32_t mask_size = mask_stride * size;

    write_u32(output, 40U);
    write_u32(output, size);
    write_u32(output, size * 2U);
    write_u16(output, 1U);
    write_u16(output, 32U);
    write_u32(output, 0U);
    write_u32(output, xor_size + mask_size);
    write_u32(output, 0U);
    write_u32(output, 0U);
    write_u32(output, 0U);
    write_u32(output, 0U);

    for (std::uint32_t y = size; y > 0U; --y) {
        for (std::uint32_t x = 0; x < size; ++x) {
            const Pixel pixel = pixels[static_cast<std::size_t>(y - 1U) * size + x];
            write_u8(output, pixel.blue);
            write_u8(output, pixel.green);
            write_u8(output, pixel.red);
            write_u8(output, pixel.alpha);
        }
    }

    std::vector<std::uint8_t> mask(mask_stride, 0U);
    for (std::uint32_t y = size; y > 0U; --y) {
        std::fill(mask.begin(), mask.end(), 0U);
        for (std::uint32_t x = 0; x < size; ++x) {
            const Pixel& pixel = pixels[static_cast<std::size_t>(y - 1U) * size + x];
            if (pixel.alpha < 128U) {
                mask[x / 8U] |= static_cast<std::uint8_t>(0x80U >> (x % 8U));
            }
        }
        output.write(reinterpret_cast<const char*>(mask.data()),
                     static_cast<std::streamsize>(mask.size()));
    }
}

bool generate_icon(const std::filesystem::path& destination) {
    std::error_code filesystem_error;
    const std::filesystem::path parent = destination.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, filesystem_error);
        if (filesystem_error) {
            std::cerr << "Unable to create icon output directory: "
                      << filesystem_error.message() << '\n';
            return false;
        }
    }

    std::vector<std::vector<Pixel>> images;
    images.reserve(kIconSizes.size());
    for (const std::uint32_t size : kIconSizes) {
        images.push_back(render_icon(size));
    }

    std::ofstream output(destination, std::ios::binary | std::ios::trunc);
    if (!output) {
        std::cerr << "Unable to open icon output file: " << destination.string() << '\n';
        return false;
    }

    write_u16(output, 0U);
    write_u16(output, 1U);
    write_u16(output, static_cast<std::uint16_t>(kIconSizes.size()));

    const std::uint32_t directory_size =
        6U + 16U * static_cast<std::uint32_t>(kIconSizes.size());
    std::uint32_t image_offset = directory_size;

    for (std::size_t index = 0; index < kIconSizes.size(); ++index) {
        const std::uint32_t size = kIconSizes[index];
        const std::uint32_t xor_size = size * size * 4U;
        const std::uint32_t mask_stride = ((size + 31U) / 32U) * 4U;
        const std::uint32_t image_size = 40U + xor_size + mask_stride * size;

        write_u8(output, size == 256U ? 0U : static_cast<std::uint8_t>(size));
        write_u8(output, size == 256U ? 0U : static_cast<std::uint8_t>(size));
        write_u8(output, 0U);
        write_u8(output, 0U);
        write_u16(output, 1U);
        write_u16(output, 32U);
        write_u32(output, image_size);
        write_u32(output, image_offset);
        image_offset += image_size;
    }

    for (std::size_t index = 0; index < kIconSizes.size(); ++index) {
        write_bitmap(output, kIconSizes[index], images[index]);
    }

    output.flush();
    if (!output) {
        std::cerr << "Failed while writing icon output: " << destination.string() << '\n';
        return false;
    }

    return true;
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: suborno_icon_generator OUTPUT.ico\n";
        return 2;
    }

    return generate_icon(std::filesystem::path(argv[1])) ? 0 : 1;
}

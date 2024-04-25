#pragma once

#include <emmintrin.h>
#include <immintrin.h>

#include <array>
#include <print>
#include <ranges>

template <bool binary, bool ascii, bool hex, bool decimal>
inline void DisplayM128(const std::string_view title, const __m128i& v)
{
    std::array<uint8_t, 16> data{};
    const uint8_t* p = reinterpret_cast<const uint8_t*>(&v);
    std::copy(p, p + 16, data.data());
    std::println("{:-^155}", title);

    // header
    std::print("{:>10}: ", "index");
    for (auto i : std::ranges::reverse_view(std::views::iota(0, 16)))
    {
        std::print("{:>8} ", i);
    }
    std::println("");

    if constexpr (binary)
    {
        std::print("{:>10}: ", "binary");
        for (auto b : std::ranges::reverse_view(data))
        {
            std::print("{:08b} ", b);
        }
        std::println("");
    }
    if constexpr (ascii)
    {
        std::print("{:>10}: ", "ascii");
        for (auto b : std::ranges::reverse_view(data))
        {
            if (const char c = std::bit_cast<char>(b); isprint(c))
            {
                std::print("{:>8} ", c);
            }
            else
            {
                std::print("{:>8} ", "bad");
            }
        }
        std::println("");
    }
    if constexpr (decimal)
    {
        std::print("{:>10}: ", "decimal");
        for (auto b : std::ranges::reverse_view(data))
        {
            std::print("{:8} ", b);
        }
        std::println("");
    }
    if constexpr (hex)
    {
        std::print("{:>10}: ", "hex");
        for (auto b : std::ranges::reverse_view(data))
        {
            std::print("{:8X} ", b);
        }
        std::println("");
    }
    std::println("");
}

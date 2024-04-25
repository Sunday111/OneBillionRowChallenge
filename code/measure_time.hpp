#pragma once

#include <chrono>
#include <concepts>

template <std::invocable<> T>
std::chrono::milliseconds MeasureDuration(T&& fn)
{
    const auto start = std::chrono::high_resolution_clock::now();
    fn();
    const auto finish = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(finish - start);
}

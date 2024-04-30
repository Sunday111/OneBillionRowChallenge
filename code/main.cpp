#include <algorithm>
#include <cassert>
#include <ranges>
#include <thread>
#include <vector>

#include "ankerl/unordered_dense.h"
#include "bit_scan.h"
#include "file_utils.hpp"
#include "measure_time.hpp"

constexpr bool kWithDiagnosticInfo = false;
constexpr std::optional<size_t> kOverrideThreadsCount = std::nullopt;
// constexpr std::optional<size_t> kOverrideThreadsCount = 1;

class StationStats
{
public:
    double Min() const
    {
        return static_cast<double>(min) / 10;
    }

    double Max() const
    {
        return static_cast<double>(max) / 10;
    }

    double Average() const
    {
        return static_cast<double>(sum) / (static_cast<double>(count) * 10);
    }

    void MergeFrom(const StationStats& other)
    {
        min = std::min(min, other.min);
        max = std::max(max, other.max);
        sum += other.sum;
        count += other.count;
    }

    int64_t sum = 0;
    uint32_t count = 0;
    int16_t min = std::numeric_limits<int16_t>::max();
    int16_t max = std::numeric_limits<int16_t>::min();
};

void DeclareAffinity(size_t thread_index)
{
    // Set thread affinity to bind to a specific core
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(thread_index, &cpuset);
    [[maybe_unused]] auto r = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    assert(r == 0);
}

int main([[maybe_unused]] const int argc, char** argv)
{
    if (argc < 2)
    {
        std::println("File path expected as program argument");
        return 1;
    }

    // Open file and map it's content to the memory
    const std::string_view file_path = argv[1];
    const auto read_file_result = MappedFile::Open(file_path);
    if (!read_file_result)
    {
        switch (read_file_result.error())
        {
        case MappedFileError::CouldNotOpenFile:
            std::println("Failed to open {} file.", file_path);
            break;
        case MappedFileError::FailedToGetFileSize:
            std::println("Failed to get file stas for file {}", file_path);
            break;
        case MappedFileError::FailedToMmap:
            std::println("Failed to mmap the whole file.");
            break;
        }
        return 2 + static_cast<int>(read_file_result.error());
    }

    const std::string_view file_data = read_file_result.value().GetData();
    assert((reinterpret_cast<size_t>(file_data.data())) % 64 == 0);

    using StationsMap = ankerl::unordered_dense::map<std::string_view, StationStats>;
    using StationsIterator = StationsMap::iterator;

    StationsMap name_to_stats{};
    std::vector<StationsMap> threads_stats;
    const auto file_read_time = MeasureDuration(
        [&]
        {
            const size_t threads_count = kOverrideThreadsCount.value_or(std::thread::hardware_concurrency());

            threads_stats.resize(threads_count);
            std::vector<std::jthread> threads;
            threads.reserve(threads_count);

            size_t previous_end_pos = 0;
            for (size_t thread_index : std::views::iota(0UZ, threads_count))
            {
                const bool is_last_chunk = thread_index == threads_count - 1;
                size_t begin = previous_end_pos;
                size_t end = 0;

                // The last thread takes remaining data
                if (is_last_chunk)
                {
                    end = file_data.size();
                }
                else
                {
                    const size_t threads_remaining = threads_count - thread_index;
                    const size_t bytes_remaining = file_data.size() - previous_end_pos;
                    const auto bytes_per_thread =
                        static_cast<size_t>(static_cast<double>(bytes_remaining / threads_remaining) * 1.001);
                    end = begin + bytes_per_thread;
                    auto it = std::find(&file_data[end], &file_data.back(), '\n');
                    assert(it != file_data.end());
                    end = static_cast<size_t>(it - &file_data.front()) + 1;
                }

                previous_end_pos = end;

                const auto thread_fn = [&threads_stats,
                                        thread_index,
                                        data_begin = file_data.data() + begin,
                                        data_end = file_data.begin() + end]()
                {
                    DeclareAffinity(thread_index);
                    auto pos = data_begin;

                    const auto align_offset = std::bit_cast<size_t>(data_begin) % SymbolScanner::kAlignment;
                    const auto align_pos = pos - align_offset;
                    SymbolScanner semicolon_scanner(align_pos, align_offset, ';');
                    SymbolScanner line_br_scanner(align_pos, align_offset, '\n');

                    auto read_name = [&]() -> std::string_view
                    {
                        auto start = pos;
                        auto it = semicolon_scanner.GetNext();
                        assert(it > start);
                        assert(it != data_end);
                        pos = it + 1;
                        return std::string_view(start, it);
                    };

                    auto read_value = [&]() -> int16_t
                    {
                        const auto lb = line_br_scanner.GetNext();
                        assert(*lb == '\n');

                        const bool negative = *pos == '-';
                        pos = negative ? pos + 1 : pos;

                        const int value =
                            ((lb[-1] - '0') + (lb[-3] - '0') * 10 + (lb[-4] - '0') * 100 * (lb - pos == 4)) *
                            (negative ? -1 : 1);

                        pos = lb;
                        return static_cast<int16_t>(value);  // NOLINT
                    };

                    StationsMap name_to_stats(1024);
                    while (pos != data_end)
                    {
                        auto name = read_name();
                        StationStats& stats = name_to_stats[name];

                        const auto value = read_value();
                        assert(*pos == '\n');
                        ++pos;

                        stats.min = std::min(stats.min, value);
                        stats.max = std::max(stats.max, value);
                        stats.count++;
                        stats.sum += value;
                    }

                    threads_stats[thread_index] = std::move(name_to_stats);
                };

                // No need to spawn a new thread for the last chunk - going to wait for all of them anyway
                // so this thread can be reused
                if (is_last_chunk)
                {
                    thread_fn();
                }
                else
                {
                    threads.emplace_back(std::move(thread_fn));
                }
            }
        });

    const auto merge_duration = MeasureDuration(
        [&]
        {
            // Merge data
            name_to_stats = std::move(threads_stats.front());

            for (auto& thread_stats : threads_stats | std::views::drop(1))
            {
                for (const auto& [name, stats] : thread_stats)
                {
                    name_to_stats[name].MergeFrom(stats);
                }
            }
        });

    // Print merged data
    std::vector<StationsIterator> sorted_stats;

    const auto sorting_duration = MeasureDuration(
        [&]()
        {
            sorted_stats.reserve(name_to_stats.size());
            for (auto it = name_to_stats.begin(); it != name_to_stats.end(); ++it)
            {
                sorted_stats.emplace_back(it);
            }

            std::ranges::sort(
                sorted_stats,
                [](const StationsIterator& a, const StationsIterator& b)
                {
                    return a->first < b->first;
                });
        });

    const auto printing_duration = MeasureDuration(
        [&]
        {
            std::print("{{");
            for (size_t i = 0; i != sorted_stats.size(); ++i)
            {
                if (i != 0) std::print(", ");
                const auto& [name, stats] = *sorted_stats[i];
                std::print(
                    "{}={:.1f}/{:.1f}/{:.1f}",
                    name,
                    stats.Min(),
                    std::round(stats.Average() * 10.0) / 10.0,
                    stats.Max());
            }
            std::println("}}");
        });

    if constexpr (kWithDiagnosticInfo)
    {
        const auto max_string =
            std::ranges::max_element(std::views::keys(name_to_stats), std::less<>{}, &std::string_view::size);

        std::println("File read time: {}", file_read_time);
        std::println("Merge time: {}", merge_duration);
        std::println("Sorting time: {}", sorting_duration);
        std::println("Printing duration: {}", printing_duration);
        std::println("Max string: {}", *max_string);
        std::println("Max string length: {}", (*max_string).size());
    }

    return 0;
}

#pragma once

#include <immintrin.h>

#include <bit>
#include <cassert>
#include <cstdint>
#include <print>

static_assert(alignof(__m128i) == 16);
static_assert(alignof(__m256i) == 32);
static_assert(alignof(__m512i) == 64);

class SymbolScanner128
{
public:
    using SimdType = __m128i;
    using MaskType = uint32_t;
    static constexpr size_t kAlignment = alignof(SimdType);

    explicit SymbolScanner128(const char* pos, size_t offset, const char symbol)
        : pattern_(_mm_set1_epi8(symbol)),
          pos_(pos)
    {
        assert(std::bit_cast<size_t>(pos) % kAlignment == 0 && offset < kAlignment);
        RefreshMask();
        if (offset != 0)
        {
            SkipBytes(offset);
        }
    }

    const char* GetNext()
    {
        while (!mask_)
        {
            pos_ += sizeof(SimdType);
            RefreshMask();
        }

        const size_t idx = static_cast<size_t>(std::countr_zero(mask_));
        SkipBytes(idx + 1);
        return pos_ + idx;
    }

private:
    void RefreshMask()
    {
        SimdType cmp = _mm_cmpeq_epi8(*reinterpret_cast<const SimdType*>(pos_), pattern_);
        mask_ = std::bit_cast<uint32_t>(_mm_movemask_epi8(cmp));
    }

    void SkipBytes(size_t count)
    {
        uint32_t k = 0xFFFF;
        k <<= count;
        mask_ &= k;
    }

private:
    SimdType pattern_;
    const char* pos_ = nullptr;
    MaskType mask_ = 0;
};

class SymbolScanner256
{
public:
    using SimdType = __m256i;
    using MaskType = uint32_t;
    static constexpr size_t kAlignment = alignof(SimdType);

    explicit SymbolScanner256(const char* pos, size_t offset, const char symbol)
        : pattern_(_mm256_set1_epi8(symbol)),
          pos_(pos)
    {
        assert(std::bit_cast<size_t>(pos) % kAlignment == 0 && offset < kAlignment);
        RefreshMask();
        if (offset != 0)
        {
            SkipBytes(offset);
        }
    }

    const char* GetNext()
    {
        while (!mask_)
        {
            pos_ += sizeof(SimdType);
            RefreshMask();
        }

        const size_t idx = static_cast<size_t>(std::countr_zero(mask_));
        SkipBytes(idx + 1);
        return pos_ + idx;
    }

private:
    void RefreshMask()
    {
        SimdType cmp = _mm256_cmpeq_epi8(*reinterpret_cast<const SimdType*>(pos_), pattern_);
        mask_ = std::bit_cast<uint32_t>(_mm256_movemask_epi8(cmp));
    }

    void SkipBytes(size_t count)
    {
        uint64_t k = 0xFFFFFFFF;
        k <<= count;
        mask_ &= k;
    }

private:
    SimdType pattern_;
    const char* pos_ = nullptr;
    MaskType mask_ = 0;
};

using SymbolScanner = SymbolScanner256;

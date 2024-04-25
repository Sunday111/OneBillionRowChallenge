#pragma once

#include <emmintrin.h>
#include <immintrin.h>

#include <bit>
#include <cstdint>

uint32_t ParseUint(const char* ptr, size_t len)
{
    const __m128i mul1 = _mm_set_epi32(0, 0, 1 << 24 | 10 << 16 | 100 << 8, 0);
    const __m128i shift_amount = _mm_cvtsi32_si128(static_cast<int32_t>((8UZ - len) * 8UZ));

    __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr));  // unsafe chunking
    v = _mm_subs_epu8(v, _mm_set1_epi8('0'));
    v = _mm_sll_epi64(v, shift_amount);  // shift off non-digit trash

    // convert
    v = _mm_maddubs_epi16(mul1, v);
    v = _mm_hadd_epi16(v, v);

    return std::bit_cast<uint32_t>(_mm_extract_epi16(v, 1));
}

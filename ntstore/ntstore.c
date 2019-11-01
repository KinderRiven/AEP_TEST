#include <emmintrin.h>
#include <stddef.h>
#include <stdint.h>

#define FLUSH_OPT

#define FLUSH_ALIGN ((uintptr_t)64)
#define _mm_clflushopt(addr)              \
    asm volatile(".byte 0x66; clflush %0" \
                 : "+m"(*(volatile char*)(addr)));

static inline int
util_is_pow2(uint64_t v)
{
    return v && !(v & (v - 1));
}

static inline void
flush(const void* addr, size_t len)
{
    uintptr_t uptr;
    /*
	 * Loop through cache-line-size (typically 64B) aligned chunks
	 * covering the given range.
	 */
    for (uptr = (uintptr_t)addr & ~(FLUSH_ALIGN - 1);
         uptr < (uintptr_t)addr + len; uptr += FLUSH_ALIGN) {
        _mm_clflush((char*)uptr);
    }
}

static inline void
flushopt(const void* addr, size_t len)
{
    uintptr_t uptr;
    /*
	 * Loop through cache-line-size (typically 64B) aligned chunks
	 * covering the given range.
	 */
    for (uptr = (uintptr_t)addr & ~(FLUSH_ALIGN - 1);
         uptr < (uintptr_t)addr + len; uptr += FLUSH_ALIGN) {
        _mm_clflushopt((char*)uptr);
    }
}

static inline void
memmove_small_sse2_noflush(char* dest, const char* src, size_t len)
{
    __m128i xmm0;
    __m128i xmm1;
    __m128i xmm2;
    __m128i xmm3;

    /* 9..16 */
    uint64_t d80;
    uint64_t d81;
    uint16_t d20;
    uint16_t d21;

    if (len <= 8) {
        goto le8;
    }
    if (len <= 32) {
        goto le32;
    }

    if (len > 48) {
        /* 49..64 */
        xmm0 = _mm_loadu_si128((__m128i*)src);
        xmm1 = _mm_loadu_si128((__m128i*)(src + 16));
        xmm2 = _mm_loadu_si128((__m128i*)(src + 32));
        xmm3 = _mm_loadu_si128((__m128i*)(src + len - 16));

        _mm_storeu_si128((__m128i*)dest, xmm0);
        _mm_storeu_si128((__m128i*)(dest + 16), xmm1);
        _mm_storeu_si128((__m128i*)(dest + 32), xmm2);
        _mm_storeu_si128((__m128i*)(dest + len - 16), xmm3);
        return;
    }

    /* 33..48 */
    xmm0 = _mm_loadu_si128((__m128i*)src);
    xmm1 = _mm_loadu_si128((__m128i*)(src + 16));
    xmm2 = _mm_loadu_si128((__m128i*)(src + len - 16));

    _mm_storeu_si128((__m128i*)dest, xmm0);
    _mm_storeu_si128((__m128i*)(dest + 16), xmm1);
    _mm_storeu_si128((__m128i*)(dest + len - 16), xmm2);
    return;

le32:
    if (len > 16) {
        /* 17..32 */
        xmm0 = _mm_loadu_si128((__m128i*)src);
        xmm1 = _mm_loadu_si128((__m128i*)(src + len - 16));

        _mm_storeu_si128((__m128i*)dest, xmm0);
        _mm_storeu_si128((__m128i*)(dest + len - 16), xmm1);
        return;
    }

    /* 9..16 */
    d80 = *(uint64_t*)src;
    d81 = *(uint64_t*)(src + len - 8);

    *(uint64_t*)dest = d80;
    *(uint64_t*)(dest + len - 8) = d81;
    return;

le8:
    if (len <= 2) {
        goto le2;
    }

    if (len > 4) {
        /* 5..8 */
        uint32_t d40 = *(uint32_t*)src;
        uint32_t d41 = *(uint32_t*)(src + len - 4);

        *(uint32_t*)dest = d40;
        *(uint32_t*)(dest + len - 4) = d41;
        return;
    }

    /* 3..4 */
    d20 = *(uint16_t*)src;
    d21 = *(uint16_t*)(src + len - 2);

    *(uint16_t*)dest = d20;
    *(uint16_t*)(dest + len - 2) = d21;
    return;

le2:
    if (len == 2) {
        *(uint16_t*)dest = *(uint16_t*)src;
        return;
    }

    *(uint8_t*)dest = *(uint8_t*)src;
}

static inline void
memmove_small_sse2(char* dest, const char* src, size_t len)
{
    memmove_small_sse2_noflush(dest, src, len);
    flushopt(dest, len);
}

static inline void
memmove_movnt4x64b(char* dest, const char* src) // 64B * 4 = 256B
{
    __m128i xmm0 = _mm_loadu_si128((__m128i*)src + 0); // 16
    __m128i xmm1 = _mm_loadu_si128((__m128i*)src + 1); // 32
    __m128i xmm2 = _mm_loadu_si128((__m128i*)src + 2); // 48
    __m128i xmm3 = _mm_loadu_si128((__m128i*)src + 3); // 64
    __m128i xmm4 = _mm_loadu_si128((__m128i*)src + 4);
    __m128i xmm5 = _mm_loadu_si128((__m128i*)src + 5);
    __m128i xmm6 = _mm_loadu_si128((__m128i*)src + 6);
    __m128i xmm7 = _mm_loadu_si128((__m128i*)src + 7); // 128B
    __m128i xmm8 = _mm_loadu_si128((__m128i*)src + 8);
    __m128i xmm9 = _mm_loadu_si128((__m128i*)src + 9);
    __m128i xmm10 = _mm_loadu_si128((__m128i*)src + 10);
    __m128i xmm11 = _mm_loadu_si128((__m128i*)src + 11);
    __m128i xmm12 = _mm_loadu_si128((__m128i*)src + 12);
    __m128i xmm13 = _mm_loadu_si128((__m128i*)src + 13);
    __m128i xmm14 = _mm_loadu_si128((__m128i*)src + 14);
    __m128i xmm15 = _mm_loadu_si128((__m128i*)src + 15); // 256B

    _mm_stream_si128((__m128i*)dest + 0, xmm0);
    _mm_stream_si128((__m128i*)dest + 1, xmm1);
    _mm_stream_si128((__m128i*)dest + 2, xmm2);
    _mm_stream_si128((__m128i*)dest + 3, xmm3);
    _mm_stream_si128((__m128i*)dest + 4, xmm4);
    _mm_stream_si128((__m128i*)dest + 5, xmm5);
    _mm_stream_si128((__m128i*)dest + 6, xmm6);
    _mm_stream_si128((__m128i*)dest + 7, xmm7);
    _mm_stream_si128((__m128i*)dest + 8, xmm8);
    _mm_stream_si128((__m128i*)dest + 9, xmm9);
    _mm_stream_si128((__m128i*)dest + 10, xmm10);
    _mm_stream_si128((__m128i*)dest + 11, xmm11);
    _mm_stream_si128((__m128i*)dest + 12, xmm12);
    _mm_stream_si128((__m128i*)dest + 13, xmm13);
    _mm_stream_si128((__m128i*)dest + 14, xmm14);
    _mm_stream_si128((__m128i*)dest + 15, xmm15);
}

static inline void
memmove_movnt2x64b(char* dest, const char* src)
{
    __m128i xmm0 = _mm_loadu_si128((__m128i*)src + 0);
    __m128i xmm1 = _mm_loadu_si128((__m128i*)src + 1);
    __m128i xmm2 = _mm_loadu_si128((__m128i*)src + 2);
    __m128i xmm3 = _mm_loadu_si128((__m128i*)src + 3);
    __m128i xmm4 = _mm_loadu_si128((__m128i*)src + 4);
    __m128i xmm5 = _mm_loadu_si128((__m128i*)src + 5);
    __m128i xmm6 = _mm_loadu_si128((__m128i*)src + 6);
    __m128i xmm7 = _mm_loadu_si128((__m128i*)src + 7);

    _mm_stream_si128((__m128i*)dest + 0, xmm0);
    _mm_stream_si128((__m128i*)dest + 1, xmm1);
    _mm_stream_si128((__m128i*)dest + 2, xmm2);
    _mm_stream_si128((__m128i*)dest + 3, xmm3);
    _mm_stream_si128((__m128i*)dest + 4, xmm4);
    _mm_stream_si128((__m128i*)dest + 5, xmm5);
    _mm_stream_si128((__m128i*)dest + 6, xmm6);
    _mm_stream_si128((__m128i*)dest + 7, xmm7);
}

static inline void
memmove_movnt1x64b(char* dest, const char* src)
{
    __m128i xmm0 = _mm_loadu_si128((__m128i*)src + 0);
    __m128i xmm1 = _mm_loadu_si128((__m128i*)src + 1);
    __m128i xmm2 = _mm_loadu_si128((__m128i*)src + 2);
    __m128i xmm3 = _mm_loadu_si128((__m128i*)src + 3);

    _mm_stream_si128((__m128i*)dest + 0, xmm0);
    _mm_stream_si128((__m128i*)dest + 1, xmm1);
    _mm_stream_si128((__m128i*)dest + 2, xmm2);
    _mm_stream_si128((__m128i*)dest + 3, xmm3);
}

static inline void
memmove_movnt1x32b(char* dest, const char* src)
{
    __m128i xmm0 = _mm_loadu_si128((__m128i*)src + 0);
    __m128i xmm1 = _mm_loadu_si128((__m128i*)src + 1);

    _mm_stream_si128((__m128i*)dest + 0, xmm0);
    _mm_stream_si128((__m128i*)dest + 1, xmm1);
}

static inline void
memmove_movnt1x16b(char* dest, const char* src)
{
    __m128i xmm0 = _mm_loadu_si128((__m128i*)src);
    _mm_stream_si128((__m128i*)dest, xmm0);
}

static inline void
memmove_movnt1x8b(char* dest, const char* src)
{
    _mm_stream_si64((long long*)dest, *(long long*)src);
}

static inline void
memmove_movnt1x4b(char* dest, const char* src)
{
    _mm_stream_si32((int*)dest, *(int*)src);
}

static inline void
memmove_movnt_sse_fw(char* dest, const char* src, size_t len)
{
    size_t cnt = (uint64_t)dest & 63;

    /* This might a align 64B ensure for PMDK */
    /*
	if (cnt > 0)
	{
		cnt = 64 - cnt;
		if (cnt > len)
		{
			cnt = len;
		}
		memmove_small_sse2(dest, src, cnt);
		dest += cnt;
		src += cnt;
		len -= cnt;
	}
	*/

    while (len >= 4 * 64) {
        memmove_movnt4x64b(dest, src);
        dest += 4 * 64;
        src += 4 * 64;
        len -= 4 * 64;
    }

    if (len >= 2 * 64) {
        memmove_movnt2x64b(dest, src);
        dest += 2 * 64;
        src += 2 * 64;
        len -= 2 * 64;
    }

    if (len >= 1 * 64) {
        memmove_movnt1x64b(dest, src);
        dest += 1 * 64;
        src += 1 * 64;
        len -= 1 * 64;
    }

    if (len == 0) {
        return;
    }

    /* There's no point in using more than 1 nt store for 1 cache line. */
    if (util_is_pow2(len)) {
        if (len == 32)
            memmove_movnt1x32b(dest, src);
        else if (len == 16)
            memmove_movnt1x16b(dest, src);
        else if (len == 8)
            memmove_movnt1x8b(dest, src);
        else if (len == 4)
            memmove_movnt1x4b(dest, src);
        else
            goto nonnt;
        return;
    }

nonnt:
    memmove_small_sse2(dest, src, len);
}

static inline void
memmove_movnt_sse_bw(char* dest, const char* src, size_t len)
{
    dest += len;
    src += len;

    size_t cnt = (uint64_t)dest & 63;

    /*if (cnt > 0)
	{
		if (cnt > len)
			cnt = len;

		dest -= cnt;
		src -= cnt;
		len -= cnt;

		memmove_small_sse2(dest, src, cnt);
	}*/

    while (len >= 4 * 64) {
        dest -= 4 * 64;
        src -= 4 * 64;
        len -= 4 * 64;
        memmove_movnt4x64b(dest, src);
#ifdef FLUSH_OPT
        _mm_sfence();
#endif
    }

    if (len >= 2 * 64) {
        dest -= 2 * 64;
        src -= 2 * 64;
        len -= 2 * 64;
        memmove_movnt2x64b(dest, src);
#ifdef FLUSH_OPT
        _mm_sfence();
#endif
    }

    if (len >= 1 * 64) {
        dest -= 1 * 64;
        src -= 1 * 64;
        len -= 1 * 64;
        memmove_movnt1x64b(dest, src);
#ifdef FLUSH_OPT
        _mm_sfence();
#endif
    }

    if (len == 0) {
        return;
    }

    /* There's no point in using more than 1 nt store for 1 cache line. */
    if (util_is_pow2(len)) {
        if (len == 32) {
            dest -= 32;
            src -= 32;
            memmove_movnt1x32b(dest, src);
        } else if (len == 16) {
            dest -= 16;
            src -= 16;
            memmove_movnt1x16b(dest, src);
        } else if (len == 8) {
            dest -= 8;
            src -= 8;
            memmove_movnt1x8b(dest, src);
        } else if (len == 4) {
            dest -= 4;
            src -= 4;
            memmove_movnt1x4b(dest, src);
        } else {
            goto nonnt;
        }
        return;
    }

nonnt:
    dest -= len;
    src -= len;
    memmove_small_sse2(dest, src, len);
}

void nvmem_memcpy(char* dest, const char* src, size_t len)
{
    if ((uintptr_t)dest - (uintptr_t)src >= len) {
        memmove_movnt_sse_fw(dest, src, len);
    } else {
        memmove_movnt_sse_bw(dest, src, len);
    }
    // memcpy(dest, src, len);
    // flush(dest, len);
    // flushopt(dest, len);
    _mm_sfence();
}

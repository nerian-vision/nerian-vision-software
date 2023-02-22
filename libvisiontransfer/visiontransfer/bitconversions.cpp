/*******************************************************************************
 * Copyright (c) 2022 Nerian Vision GmbH
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *******************************************************************************/

#include "visiontransfer/bitconversions.h"
#include "visiontransfer/exceptions.h"

// SIMD Headers
#ifdef __AVX2__
#   include <immintrin.h>
#elif __SSE4_1__
#   include <smmintrin.h>
#elif __SSE2__
#   include <emmintrin.h>
#endif

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

using namespace visiontransfer;
using namespace visiontransfer::internal;

namespace visiontransfer {
namespace internal {

void BitConversions::decode12BitPacked(int startRow, int stopRow, const unsigned char* src,
        unsigned char* dst, int srcStride, int dstStride, int rowWidth) {

    const unsigned char* dispStart = src;

#   ifdef __SSE4_1__
    if(rowWidth % 32 == 0) {
        if(srcStride % 16 == 0 && reinterpret_cast<size_t>(src) % 16 == 0) {
            decode12BitPackedSSE4<true>(startRow, stopRow, dispStart,
                rowWidth, reinterpret_cast<unsigned short*>(dst), srcStride, dstStride);
        } else {
            decode12BitPackedSSE4<false>(startRow, stopRow, dispStart,
                rowWidth, reinterpret_cast<unsigned short*>(dst), srcStride, dstStride);
        }

    } else // We use fallback implementation if the image width is not dividable by 32
#   endif
#   if defined(__ARM_NEON) && defined(__ARM_ARCH_ISA_A64)
    if(rowWidth % 32 == 0) {
        if(srcStride % 16 == 0 && reinterpret_cast<size_t>(src) % 16 == 0) {
            decode12BitPackedNEON<true>(startRow, stopRow, dispStart,
                rowWidth, reinterpret_cast<unsigned short*>(dst), srcStride, dstStride);
        } else {
            decode12BitPackedNEON<false>(startRow, stopRow, dispStart,
                rowWidth, reinterpret_cast<unsigned short*>(dst), srcStride, dstStride);
        }

    } else // We use fallback implementation if the image width is not dividable by 32
#   endif
    {
        decode12BitPackedFallback(startRow, stopRow, dispStart, rowWidth,
            reinterpret_cast<unsigned short*>(dst), srcStride, dstStride);
    }
}

#ifdef __SSE4_1__
template <bool alignedLoad>
void BitConversions::decode12BitPackedSSE4(int startRow, int stopRow, const unsigned char* dispStart,
        int width, unsigned short* dst, int srcStride, int dstStride) {
    if(width % 32 != 0) {
        throw ProtocolException("Image width must be a multiple of 32!");
    }

    // SSE optimized code
    unsigned char* outPos = &reinterpret_cast<unsigned char*>(dst)[startRow*dstStride];
    int outRowPadding = dstStride - 2*width;

    constexpr char ff = (char)0xff; // to prevent warnings
    const __m128i shuffleMask1a = _mm_set_epi8(11, 10, 10, 9, 8, 7, 7, 6, 5, 4, 4, 3, 2, 1, 1, 0);
    const __m128i shuffleMask1b = _mm_set_epi8(ff, ff, ff, ff, ff, ff, ff, ff, ff, ff, ff, 15, 14, 13, 13, 12);

    const __m128i shuffleMask2a = _mm_set_epi8(7, 6, 6, 5, 4, 3, 3, 2, 1, 0, 0, ff, ff, ff, ff, ff);
    const __m128i shuffleMask2b = _mm_set_epi8(ff, ff, ff, ff, ff, 15, 15, 14, 13, 12, 12, 11, 10, 9, 9, 8);

    const __m128i shuffleMask3a = _mm_set_epi8(3, 2, 2, 1, 0, ff, ff, ff, ff, ff, ff, ff, ff, ff, ff, ff);
    const __m128i shuffleMask3b = _mm_set_epi8(15, 14, 14, 13, 12, 11, 11, 10, 9, 8, 8, 7, 6, 5, 5, 4);

    const __m128i shiftMultiplyMask = _mm_set_epi16(1, 16, 1, 16, 1, 16, 1, 16);

    const __m128i blendMask1 = _mm_set_epi8(ff, ff, ff, ff, ff, ff, ff, ff, ff, ff, ff, 0, 0, 0, 0, 0);
    const __m128i blendMask2 = _mm_set_epi8(ff, ff, ff, ff, ff, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    int dispRowWidth = width * 3/2;

    for(int y = startRow; y<stopRow; y++) {
        const unsigned char* rowPos = &dispStart[y*srcStride];
        const unsigned char* rowEnd = &dispStart[y*srcStride + dispRowWidth];

        while(rowPos < rowEnd) {
            // Load 16 pixels
            // AA BA BB CC DC DD EE FE FF ...
            __m128i rowPixels1, rowPixels2, rowPixels3;
            if(alignedLoad) {
                rowPixels1 = _mm_load_si128(reinterpret_cast<const __m128i*>(rowPos));
                rowPos += 16;

                rowPixels2 = _mm_load_si128(reinterpret_cast<const __m128i*>(rowPos));
                rowPos += 16;

                rowPixels3 = _mm_load_si128(reinterpret_cast<const __m128i*>(rowPos));
                rowPos += 16;
            } else {
                rowPixels1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(rowPos));
                rowPos += 16;

                rowPixels2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(rowPos));
                rowPos += 16;

                rowPixels3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(rowPos));
                rowPos += 16;
            }

            // Duplicate bytes with shared data
            // BAAA BBBA DCCC DDDC FEEE FFFE (example without endianess swap!)
            __m128i part1 = _mm_shuffle_epi8(rowPixels1, shuffleMask1a);
            __m128i part2a = _mm_shuffle_epi8(rowPixels1, shuffleMask1b);
            __m128i part2b = _mm_shuffle_epi8(rowPixels2, shuffleMask2a);
            __m128i part3a = _mm_shuffle_epi8(rowPixels2, shuffleMask2b);
            __m128i part3b = _mm_shuffle_epi8(rowPixels3, shuffleMask3a);
            __m128i part4 = _mm_shuffle_epi8(rowPixels3, shuffleMask3b);

            __m128i part2 = _mm_blendv_epi8(part2a, part2b, blendMask1);
            __m128i part3 = _mm_blendv_epi8(part3a, part3b, blendMask2);

            // Shift left through multiplication
            // AAA0 BBBA CCC0 DDDC EEE0 FFFE
            __m128i shift1a =  _mm_mullo_epi16(part1, shiftMultiplyMask);
            __m128i shift2a =  _mm_mullo_epi16(part2, shiftMultiplyMask);
            __m128i shift3a =  _mm_mullo_epi16(part3, shiftMultiplyMask);
            __m128i shift4a =  _mm_mullo_epi16(part4, shiftMultiplyMask);

            // Shift right again
            // 0AAA 0BBB 0CCC 0DDD 0EEE 0FFF ...
            __m128i shift1b = _mm_srli_epi16(shift1a, 4);
            __m128i shift2b = _mm_srli_epi16(shift2a, 4);
            __m128i shift3b = _mm_srli_epi16(shift3a, 4);
            __m128i shift4b = _mm_srli_epi16(shift4a, 4);

            _mm_storeu_si128(reinterpret_cast<__m128i*>(outPos), shift1b);
            outPos += 16;
            _mm_storeu_si128(reinterpret_cast<__m128i*>(outPos), shift2b);
            outPos += 16;
            _mm_storeu_si128(reinterpret_cast<__m128i*>(outPos), shift3b);
            outPos += 16;
            _mm_storeu_si128(reinterpret_cast<__m128i*>(outPos), shift4b);
            outPos += 16;
        }

        outPos += outRowPadding;
    }
}
#endif

#if defined(__ARM_NEON) && defined(__ARM_ARCH_ISA_A64)
#define TX(y,x) ((x + y*16)/3 + ((x + y*16)%3)*16)

template <bool alignedLoad>
void BitConversions::decode12BitPackedNEON(int startRow, int stopRow, const unsigned char* dispStart,
        int width, unsigned short* dst, int srcStride, int dstStride) {
    if(width % 32 != 0) {
        throw ProtocolException("Image width must be a multiple of 32!");
    }

    // ARM NEON A64 optimized code
    unsigned char* outPos = &reinterpret_cast<unsigned char*>(dst)[startRow*dstStride];
    int outRowPadding = dstStride - 2*width;

    // Shuffle mask already performs endianess swapping
    const uint8x16_t shuffleMask1 = {TX(0,0), TX(0,1), TX(0,1), TX(0,2), TX(0,3), TX(0,4),
        TX(0,4), TX(0,5), TX(0,6), TX(0,7), TX(0,7), TX(0,8), TX(0,9), TX(0,10), TX(0,10), TX(0,11)};
    const uint8x16_t shuffleMask2 = {TX(0,12), TX(0,13), TX(0,13), TX(0,14), TX(0,15), TX(1,0),
        TX(1,0), TX(1,1), TX(1,2), TX(1,3), TX(1,3), TX(1,4), TX(1,5), TX(1,6), TX(1,6), TX(1,7)};
    const uint8x16_t shuffleMask3 = {TX(1,8), TX(1,9), TX(1,9), TX(1,10), TX(1,11), TX(1,12),
        TX(1,12), TX(1,13), TX(1,14), TX(1,15), TX(1,15), TX(2,0), TX(2,1), TX(2,2), TX(2,2), TX(2,3)};
    const uint8x16_t shuffleMask4 = {TX(2,4), TX(2,5), TX(2,5), TX(2,6), TX(2,7), TX(2,8),
        TX(2,8), TX(2,9), TX(2,10), TX(2,11), TX(2,11), TX(2,12), TX(2,13), TX(2,14), TX(2,14), TX(2,15)};

    const int16x8_t shiftMask = {4, 0, 4, 0, 4, 0, 4, 0};

    int dispRowWidth = width * 3/2;

    for(int y = startRow; y<stopRow; y++) {
        const unsigned char* rowPos = &dispStart[y*srcStride];
        const unsigned char* rowEnd = &dispStart[y*srcStride + dispRowWidth];

        while(rowPos < rowEnd) {
            // Load 16 pixels
            // AA BA BB CC DC DD EE FE FF
            uint8x16x3_t rowPixels;
            if(alignedLoad) {
                rowPixels = vld3q_u8(reinterpret_cast<const uint8_t*>(
                    __builtin_assume_aligned(rowPos, 16)));
            } else {
                rowPixels = vld3q_u8(reinterpret_cast<const uint8_t*>(rowPos));
            }
            rowPos += 48;

            // Duplicate bytes with shared data
            // BAAA BBBA DCCC DDDC FEEE FFFE (example without endianess swap!)
            uint8x16_t part1 = vqtbl3q_u8(rowPixels, shuffleMask1);
            uint8x16_t part2 = vqtbl3q_u8(rowPixels, shuffleMask2);
            uint8x16_t part3 = vqtbl3q_u8(rowPixels, shuffleMask3);
            uint8x16_t part4 = vqtbl3q_u8(rowPixels, shuffleMask4);

            // Shift left
            // AAA0 BBBA CCC0 DDDC EEE0 FFFE
            uint16x8_t shift1a = vshlq_u16(vreinterpretq_u16_u8(part1), shiftMask);
            uint16x8_t shift2a = vshlq_u16(vreinterpretq_u16_u8(part2), shiftMask);
            uint16x8_t shift3a = vshlq_u16(vreinterpretq_u16_u8(part3), shiftMask);
            uint16x8_t shift4a = vshlq_u16(vreinterpretq_u16_u8(part4), shiftMask);

            // Shift right again
            // 0AAA 0BBB 0CCC 0DDD 0EEE 0FFF ...
            uint16x8_t shift1b = vshrq_n_u16(shift1a, 4);
            uint16x8_t shift2b = vshrq_n_u16(shift2a, 4);
            uint16x8_t shift3b = vshrq_n_u16(shift3a, 4);
            uint16x8_t shift4b = vshrq_n_u16(shift4a, 4);

            vst1q_u16(reinterpret_cast<uint16_t*>(outPos), shift1b);
            outPos += 16;
            vst1q_u16(reinterpret_cast<uint16_t*>(outPos), shift2b);
            outPos += 16;
            vst1q_u16(reinterpret_cast<uint16_t*>(outPos), shift3b);
            outPos += 16;
            vst1q_u16(reinterpret_cast<uint16_t*>(outPos), shift4b);
            outPos += 16;
        }

        outPos += outRowPadding;
    }
}
#endif

void BitConversions::decode12BitPackedFallback(int startRow, int stopRow, const unsigned char* dispStart,
        int width, unsigned short* dst, int srcStride, int dstStride) {

    int dstStrideShort =  dstStride/2;

    // Non-SSE version
    for(int y = startRow; y < stopRow; y++) {
        const unsigned char* srcPtr = &dispStart[y*srcStride];
        unsigned short* dstPtr = &dst[y*dstStrideShort];
        unsigned short* dstEndPtr = dstPtr + width;

        while(dstPtr != dstEndPtr) {
            *dstPtr = static_cast<unsigned short>(*srcPtr);
            srcPtr++;
            *dstPtr |= static_cast<unsigned short>(*srcPtr & 0x0f) << 8;
            dstPtr++;

            *dstPtr = static_cast<unsigned short>(*srcPtr) >> 4;
            srcPtr++;
            *dstPtr |= static_cast<unsigned short>(*srcPtr) << 4;
            srcPtr++;
            dstPtr++;
        }
    }
}

void BitConversions::encode12BitPacked(int startRow, int stopRow, const unsigned char* src,
        unsigned char* dst, int srcStride, int dstStride, int rowWidth) {
    const unsigned short* srcShort = reinterpret_cast<const unsigned short*>(src);
    int srcStrideShort =  srcStride/2;

    // SSE/NEON optimization is not yet available
    for(int y = startRow; y < stopRow; y++) {
        const unsigned short* srcPtr = &srcShort[y*srcStrideShort];
        const unsigned short* srcEndPtr = srcPtr + rowWidth;
        unsigned char* dstPtr = &dst[y*dstStride];

        while(srcPtr != srcEndPtr) {
            *dstPtr = static_cast<unsigned char>(*srcPtr);
            dstPtr++;
            *dstPtr = static_cast<unsigned char>(*srcPtr >> 8) & 0x0f;
            srcPtr++;

            *dstPtr |= static_cast<unsigned char>(*srcPtr) << 4;
            dstPtr++;
            *dstPtr = static_cast<unsigned char>(*srcPtr >> 4);
            srcPtr++;
            dstPtr++;
        }
    }
}

}} // namespace


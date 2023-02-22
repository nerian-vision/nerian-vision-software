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

#ifndef VISIONTRANSFER_BITCONVERSIONS_H
#define VISIONTRANSFER_BITCONVERSIONS_H

namespace visiontransfer {
namespace internal {

/**
 * \brief Various implementations for converting from 12-bit to
 * 16-bit per pixels formats.
 */
class BitConversions {
public:
    static void decode12BitPacked(int startRow, int stopRow, const unsigned char* src,
        unsigned char* dst, int srcStride, int dstStride, int rowWidth);

    static void encode12BitPacked(int startRow, int stopRow, const unsigned char* src,
        unsigned char* dst, int srcStride, int dstStride, int rowWidth);

private:
    template <bool alignedLoad>
    static void decode12BitPackedSSE4(int startRow, int stopRow, const unsigned char* dispStart,
        int width, unsigned short* dst, int srcStride, int dstStride);

    template <bool alignedLoad>
    static void decode12BitPackedNEON(int startRow, int stopRow, const unsigned char* dispStart,
        int width, unsigned short* dst, int srcStride, int dstStride);

    static void decode12BitPackedFallback(int startRow, int stopRow, const unsigned char* dispStart,
        int width, unsigned short* dst, int srcStride, int dstStride);
};

}} // namespace

#endif

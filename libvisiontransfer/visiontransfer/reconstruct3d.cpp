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

#include "reconstruct3d.h"
#include "visiontransfer/alignedallocator.h"
#include <vector>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <cmath>

// SIMD Headers
#ifdef __AVX2__
#include <immintrin.h>
#elif __SSE2__
#include <emmintrin.h>
#endif

using namespace std;
using namespace visiontransfer;
using namespace visiontransfer::internal;

namespace visiontransfer {

/*************** Pimpl class containing all private members ***********/

class Reconstruct3D::Pimpl {
public:
    Pimpl();

    float* createPointMap(const unsigned short* dispMap, int width, int height,
        int rowStride, const float* q, unsigned short minDisparity, int subpixelFactor,
        unsigned short maxDisparity);

    float* createPointMap(const ImageSet& imageSet, unsigned short minDisparity, unsigned short maxDisparity = 0xFFF);

    float* createZMap(const ImageSet& imageSet, unsigned short minDisparity, unsigned short maxDisparity);

    void projectSinglePoint(int imageX, int imageY, unsigned short disparity, const float* q,
        float& pointX, float& pointY, float& pointZ, int subpixelFactor);

    void writePlyFile(const char* file, const unsigned short* dispMap,
        const unsigned char* image, int width, int height, ImageSet::ImageFormat format,
        int dispRowStride, int imageRowStride, const float* q,
        double maxZ, bool binary, int subpixelFactor, unsigned short maxDisparity);

   void writePlyFile(const char* file, const ImageSet& imageSet,
        double maxZ, bool binary, ColorSource colSource, unsigned short maxDisparity);

private:
    std::vector<float, AlignedAllocator<float> > pointMap;

    float* createPointMapFallback(const unsigned short* dispMap, int width, int height,
        int rowStride, const float* q, unsigned short minDisparity, int subpixelFactor,
        unsigned short maxDisparity);

    float* createPointMapSSE2(const unsigned short* dispMap, int width, int height,
        int rowStride, const float* q, unsigned short minDisparity, int subpixelFactor,
        unsigned short maxDisparity);

    float* createPointMapAVX2(const unsigned short* dispMap, int width, int height,
        int rowStride, const float* q, unsigned short minDisparity, int subpixelFactor,
        unsigned short maxDisparity);
};

/******************** Stubs for all public members ********************/

Reconstruct3D::Reconstruct3D()
    :pimpl(new Pimpl) {
}

Reconstruct3D::~Reconstruct3D() {
    delete pimpl;
}

float* Reconstruct3D::createPointMap(const unsigned short* dispMap, int width, int height,
        int rowStride, const float* q, unsigned short minDisparity, int subpixelFactor,
        unsigned short maxDisparity) {
    return pimpl->createPointMap(dispMap, width, height, rowStride, q, minDisparity,
        subpixelFactor, maxDisparity);
}

float* Reconstruct3D::createPointMap(const ImageSet& imageSet, unsigned short minDisparity, unsigned short maxDisparity) {
    return pimpl->createPointMap(imageSet, minDisparity, maxDisparity);
}

float* Reconstruct3D::createZMap(const ImageSet& imageSet, unsigned short minDisparity,
    unsigned short maxDisparity) {
    return pimpl->createZMap(imageSet, minDisparity, maxDisparity);
}

void Reconstruct3D::projectSinglePoint(int imageX, int imageY, unsigned short disparity,
        const float* q, float& pointX, float& pointY, float& pointZ, int subpixelFactor) {
    pimpl->projectSinglePoint(imageX, imageY, disparity, q, pointX, pointY, pointZ,
        subpixelFactor);
}

void Reconstruct3D::writePlyFile(const char* file, const unsigned short* dispMap,
        const unsigned char* image,  int width, int height, ImageSet::ImageFormat format, int dispRowStride,
        int imageRowStride, const float* q, double maxZ, bool binary, int subpixelFactor,
        unsigned short maxDisparity) {
    pimpl->writePlyFile(file, dispMap, image, width, height, format, dispRowStride,
        imageRowStride, q, maxZ, binary, subpixelFactor, maxDisparity);
}

void Reconstruct3D::writePlyFile(const char* file, const ImageSet& imageSet,
        double maxZ, bool binary, ColorSource colSource, unsigned short maxDisparity) {
    pimpl->writePlyFile(file, imageSet, maxZ, binary, colSource, maxDisparity);
}

/******************** Implementation in pimpl class *******************/

Reconstruct3D::Pimpl::Pimpl() {
}

float* Reconstruct3D::Pimpl::createPointMap(const unsigned short* dispMap, int width,
        int height, int rowStride, const float* q, unsigned short minDisparity,
        int subpixelFactor, unsigned short maxDisparity) {

    // Allocate the buffer
    if(pointMap.size() < static_cast<unsigned int>(4*width*height)) {
        pointMap.resize(4*width*height);
    }

#   ifdef __AVX2__
        if(maxDisparity <= 0x1000 && width % 16 == 0 && (uintptr_t)dispMap % 32 == 0) {
            return createPointMapAVX2(dispMap, width, height, rowStride, q,
                minDisparity, subpixelFactor, maxDisparity);
        } else
#   endif
#   ifdef __SSE2__
        if(maxDisparity <= 0x1000 && width % 8 == 0 && (uintptr_t)dispMap % 16 == 0) {
            return createPointMapSSE2(dispMap, width, height, rowStride, q,
                minDisparity, subpixelFactor, maxDisparity);
        } else
#   endif
        return createPointMapFallback(dispMap, width, height, rowStride, q,
            minDisparity, subpixelFactor, maxDisparity);
}

float* Reconstruct3D::Pimpl::createPointMap(const ImageSet& imageSet, unsigned short minDisparity, unsigned short maxDisparity) {
    if(!imageSet.hasImageType(ImageSet::IMAGE_DISPARITY)) {
        throw std::runtime_error("ImageSet does not contain a disparity map!");
    }

    if(imageSet.getPixelFormat(ImageSet::IMAGE_DISPARITY) != ImageSet::FORMAT_12_BIT_MONO) {
        throw std::runtime_error("Disparity map must have 12-bit pixel format!");
    }

    return createPointMap(reinterpret_cast<unsigned short*>(imageSet.getPixelData(ImageSet::IMAGE_DISPARITY)), imageSet.getWidth(),
        imageSet.getHeight(), imageSet.getRowStride(ImageSet::IMAGE_DISPARITY), imageSet.getQMatrix(), minDisparity,
        imageSet.getSubpixelFactor(), maxDisparity);
}

float* Reconstruct3D::Pimpl::createPointMapFallback(const unsigned short* dispMap, int width,
        int height, int rowStride, const float* q, unsigned short minDisparity,
        int subpixelFactor, unsigned short maxDisparity) {
    // Code without SSE or AVX optimization
    float* outputPtr = &pointMap[0];
    int stride = rowStride / 2;

    for(int y = 0; y < height; y++) {
        double qx = q[1]*y + q[3];
        double qy = q[5]*y + q[7];
        double qz = q[9]*y + q[11];
        double qw = q[13]*y + q[15];

        const unsigned short* dispRow = &dispMap[y*stride];
        for(int x = 0; x < width; x++) {
            unsigned short intDisp = std::max(minDisparity, dispRow[x]);
            if(intDisp >= maxDisparity) {
                intDisp = minDisparity; // Invalid disparity
            }

            double d = intDisp / double(subpixelFactor);
            double w = qw + q[14]*d;

            *outputPtr = static_cast<float>((qx + q[2]*d)/w); // x
            outputPtr++;

            *outputPtr = static_cast<float>((qy + q[6]*d)/w); // y
            outputPtr++;

            *outputPtr = static_cast<float>((qz + q[10]*d)/w); // z
            outputPtr+=2; // Consider padding

            qx += q[0];
            qy += q[4];
            qz += q[8];
            qw += q[12];
        }
    }
    return &pointMap[0];
}

float* Reconstruct3D::Pimpl::createZMap(const ImageSet& imageSet, unsigned short minDisparity,
        unsigned short maxDisparity) {
    // Allocate the buffer
    if(pointMap.size() < static_cast<unsigned int>(imageSet.getWidth()*imageSet.getHeight())) {
        pointMap.resize(imageSet.getWidth()*imageSet.getHeight());
    }

    float* outputPtr = &pointMap[0];
    int stride = imageSet.getRowStride(ImageSet::IMAGE_DISPARITY) / 2;
    const unsigned short* dispMap = reinterpret_cast<const unsigned short*>(imageSet.getPixelData(ImageSet::IMAGE_DISPARITY));
    int subpixelFactor = imageSet.getSubpixelFactor();
    const float* q = imageSet.getQMatrix();

    for(int y = 0; y < imageSet.getHeight(); y++) {
        double qz = q[9]*y + q[11];
        double qw = q[13]*y + q[15];

        const unsigned short* dispRow = &dispMap[y*stride];
        for(int x = 0; x < imageSet.getWidth(); x++) {
            unsigned short intDisp = std::max(minDisparity, dispRow[x]);
            if(intDisp >= maxDisparity) {
                intDisp = minDisparity; // Invalid disparity
            }

            double d = intDisp / double(subpixelFactor);
            double w = qw + q[14]*d;

            *outputPtr = static_cast<float>((qz + q[10]*d)/w); // z
            outputPtr++;

            qz += q[8];
        }
    }
    return &pointMap[0];
}

void Reconstruct3D::Pimpl::projectSinglePoint(int imageX, int imageY, unsigned short disparity,
        const float* q, float& pointX, float& pointY, float& pointZ, int subpixelFactor) {

    double d = disparity / double(subpixelFactor);
    double w = q[15] + q[14]*d;
    pointX = static_cast<float>((imageX*q[0] + q[3])/w);
    pointY = static_cast<float>((imageY*q[5] + q[7])/w);
    pointZ = static_cast<float>(q[11]/w);
}

# ifdef __AVX2__
float* Reconstruct3D::Pimpl::createPointMapAVX2(const unsigned short* dispMap, int width,
        int height, int rowStride, const float* q, unsigned short minDisparity,
        int subpixelFactor, unsigned short maxDisparity) {

    // Create column vectors of q
    const __m256 qCol0 = _mm256_setr_ps(q[0], q[4], q[8], q[12],   q[0], q[4], q[8], q[12]);
    const __m256 qCol1 = _mm256_setr_ps(q[1], q[5], q[9], q[13],   q[1], q[5], q[9], q[13]);
    const __m256 qCol2 = _mm256_setr_ps(q[2], q[6], q[10], q[14],  q[2], q[6], q[10], q[14]);
    const __m256 qCol3 = _mm256_setr_ps(q[3], q[7], q[11], q[15],  q[3], q[7], q[11], q[15]);

    // More constants that we need
    const __m256i minDispVector = _mm256_set1_epi16(minDisparity);
    const __m256i maxDispVector = _mm256_set1_epi16(maxDisparity);
    const __m256 scaleVector = _mm256_set1_ps(1.0/double(subpixelFactor));
    const __m256i zeroVector = _mm256_set1_epi16(0);

    float* outputPtr = &pointMap[0];

    for(int y = 0; y < height; y++) {
        const unsigned char* rowStart = &reinterpret_cast<const unsigned char*>(dispMap)[y*rowStride];
        const unsigned char* rowEnd = &reinterpret_cast<const unsigned char*>(dispMap)[y*rowStride + 2*width];

        int x = 0;
        for(const unsigned char* ptr = rowStart; ptr != rowEnd; ptr += 32) {
            __m256i disparities = _mm256_load_si256(reinterpret_cast<const __m256i*>(ptr));

            // Find invalid disparities and set them to 0
            __m256i validMask =  _mm256_cmpgt_epi16(maxDispVector, disparities);
            disparities = _mm256_and_si256(validMask, disparities);

            // Clamp to minimum disparity
            disparities = _mm256_max_epi16(disparities, minDispVector);

            // Stupid AVX2 unpack mixes everything up! Lets swap the register beforehand.
            __m256i disparitiesMixup = _mm256_permute4x64_epi64(disparities, 0xd8);

            // Convert to floats and scale with 1/subpixelFactor
            __m256 floatDisp = _mm256_cvtepi32_ps(_mm256_unpacklo_epi16(disparitiesMixup, zeroVector));
            __m256 dispScaled = _mm256_mul_ps(floatDisp, scaleVector);

            // Copy to array
#ifdef _MSC_VER
            __declspec(align(32)) float dispArray[16];
#else
            float dispArray[16]__attribute__((aligned(32)));
#endif
            _mm256_store_ps(&dispArray[0], dispScaled);

            // Same for other half
            floatDisp = _mm256_cvtepi32_ps(_mm256_unpackhi_epi16(disparitiesMixup, zeroVector));
            dispScaled = _mm256_mul_ps(floatDisp, scaleVector);
            _mm256_store_ps(&dispArray[8], dispScaled);

            // Iterate over disparities and perform matrix multiplication for each
            for(int i=0; i<16; i+=2) {
                // Create two vectors
                __m256 vec = _mm256_setr_ps(x, y, dispArray[i], 1.0,
                    x+1, y, dispArray[i+1], 1.0);

                // Multiply with matrix
                __m256 u1 = _mm256_shuffle_ps(vec,vec, _MM_SHUFFLE(0,0,0,0));
                __m256 u2 = _mm256_shuffle_ps(vec,vec, _MM_SHUFFLE(1,1,1,1));
                __m256 u3 = _mm256_shuffle_ps(vec,vec, _MM_SHUFFLE(2,2,2,2));
                __m256 u4 = _mm256_shuffle_ps(vec,vec, _MM_SHUFFLE(3,3,3,3));

                __m256 prod1 = _mm256_mul_ps(u1, qCol0);
                __m256 prod2 = _mm256_mul_ps(u2, qCol1);
                __m256 prod3 = _mm256_mul_ps(u3, qCol2);
                __m256 prod4 = _mm256_mul_ps(u4, qCol3);

                __m256 multResult = _mm256_add_ps(_mm256_add_ps(prod1, prod2), _mm256_add_ps(prod3, prod4));

                // Divide by w to receive point coordinates
                __m256 point = _mm256_div_ps(multResult,
                    _mm256_shuffle_ps(multResult,multResult, _MM_SHUFFLE(3,3,3,3)));

                // Write result to memory
                _mm256_store_ps(outputPtr, point);

                outputPtr += 8;
                x+=2;
            }
        }
    }

    return &pointMap[0];
}
#endif

#ifdef __SSE2__
float* Reconstruct3D::Pimpl::createPointMapSSE2(const unsigned short* dispMap, int width,
        int height, int rowStride, const float* q, unsigned short minDisparity,
        int subpixelFactor, unsigned short maxDisparity) {

    // Create column vectors of q
    const __m128 qCol0 = _mm_setr_ps(q[0], q[4], q[8], q[12]);
    const __m128 qCol1 = _mm_setr_ps(q[1], q[5], q[9], q[13]);
    const __m128 qCol2 = _mm_setr_ps(q[2], q[6], q[10], q[14]);
    const __m128 qCol3 = _mm_setr_ps(q[3], q[7], q[11], q[15]);

    // More constants that we need
    const __m128i minDispVector = _mm_set1_epi16(minDisparity);
    const __m128i maxDispVector = _mm_set1_epi16(maxDisparity);
    const __m128 scaleVector = _mm_set1_ps(1.0/double(subpixelFactor));
    const __m128i zeroVector = _mm_set1_epi16(0);

    float* outputPtr = &pointMap[0];

    for(int y = 0; y < height; y++) {
        const unsigned char* rowStart = &reinterpret_cast<const unsigned char*>(dispMap)[y*rowStride];
        const unsigned char* rowEnd = &reinterpret_cast<const unsigned char*>(dispMap)[y*rowStride + 2*width];

        int x = 0;
        for(const unsigned char* ptr = rowStart; ptr != rowEnd; ptr += 16) {
            __m128i disparities = _mm_load_si128(reinterpret_cast<const __m128i*>(ptr));

            // Find invalid disparities and set them to 0
            __m128i validMask =  _mm_cmplt_epi16(disparities, maxDispVector);
            disparities = _mm_and_si128(validMask, disparities);

            // Clamp to minimum disparity
            disparities = _mm_max_epi16(disparities, minDispVector);

            // Convert to floats and scale with 1/subpixelFactor
            __m128 floatDisp = _mm_cvtepi32_ps(_mm_unpacklo_epi16(disparities, zeroVector));
            __m128 dispScaled = _mm_mul_ps(floatDisp, scaleVector);

            // Copy to array
#ifdef _MSC_VER
            __declspec(align(16)) float dispArray[8];
#else
            float dispArray[8]__attribute__((aligned(16)));
#endif
            _mm_store_ps(&dispArray[0], dispScaled);

            // Same for other half
            floatDisp = _mm_cvtepi32_ps(_mm_unpackhi_epi16(disparities, zeroVector));
            dispScaled = _mm_mul_ps(floatDisp, scaleVector);
            _mm_store_ps(&dispArray[4], dispScaled);

            // Iterate over disparities and perform matrix multiplication for each
            for(int i=0; i<8; i++) {
                // Create vector
                __m128 vec = _mm_setr_ps(static_cast<float>(x), static_cast<float>(y), dispArray[i], 1.0);

                // Multiply with matrix
                __m128 u1 = _mm_shuffle_ps(vec,vec, _MM_SHUFFLE(0,0,0,0));
                __m128 u2 = _mm_shuffle_ps(vec,vec, _MM_SHUFFLE(1,1,1,1));
                __m128 u3 = _mm_shuffle_ps(vec,vec, _MM_SHUFFLE(2,2,2,2));
                __m128 u4 = _mm_shuffle_ps(vec,vec, _MM_SHUFFLE(3,3,3,3));

                __m128 prod1 = _mm_mul_ps(u1, qCol0);
                __m128 prod2 = _mm_mul_ps(u2, qCol1);
                __m128 prod3 = _mm_mul_ps(u3, qCol2);
                __m128 prod4 = _mm_mul_ps(u4, qCol3);

                __m128 multResult = _mm_add_ps(_mm_add_ps(prod1, prod2), _mm_add_ps(prod3, prod4));

                // Divide by w to receive point coordinates
                __m128 point = _mm_div_ps(multResult,
                    _mm_shuffle_ps(multResult,multResult, _MM_SHUFFLE(3,3,3,3)));

                // Write result to memory
                _mm_store_ps(outputPtr, point);

                outputPtr += 4;
                x++;
            }
        }
    }

    return &pointMap[0];
}
#endif

void Reconstruct3D::Pimpl::writePlyFile(const char* file, const unsigned short* dispMap,
        const unsigned char* image, int width, int height, ImageSet::ImageFormat format, int dispRowStride,
        int imageRowStride, const float* q, double maxZ, bool binary, int subpixelFactor,
        unsigned short maxDisparity) {

    float* pointMap = createPointMap(dispMap, width, height, dispRowStride,
        q, 0, subpixelFactor, maxDisparity);

    // Count number of valid points
    int pointsCount = 0;
    if(maxZ >= 0) {
        for(int i=0; i<width*height; i++) {
            if(pointMap[4*i+2] <= maxZ) {
                pointsCount++;
            }
        }
    } else {
        pointsCount = width*height;
    }

    // Write file header
    fstream strm(file,  binary ? (ios::out | ios::binary) : ios::out);
    strm << "ply" << endl;

    if(binary) {
        strm << "format binary_little_endian 1.0" << endl;
    } else {
        strm << "format ascii 1.0" << endl;
    }

    strm << "element vertex " << pointsCount << endl
         << "property float x" << endl
         << "property float y" << endl
         << "property float z" << endl;
    if (image != nullptr) {
        // include RGB information only if a camera image was provided
        strm << "property uchar red" << endl
             << "property uchar green" << endl
             << "property uchar blue" << endl;
    }
    strm << "end_header" << endl;

    // Write points
    for(int i=0; i<width*height; i++) {
        int y = i / width;
        int x = i % width;

        if(maxZ < 0 || pointMap[4*i+2] <= maxZ) {
            if(binary) {
                // Write binary format
                strm.write(reinterpret_cast<char*>(&pointMap[4*i]), sizeof(float)*3);
                if (image == nullptr) {
                    // disparity only, no image data
                } else if(format == ImageSet::FORMAT_8_BIT_RGB) {
                    const unsigned char* col = &image[y*imageRowStride + 3*x];
                    strm.write(reinterpret_cast<const char*>(col), 3*sizeof(*col));
                } else if(format == ImageSet::FORMAT_8_BIT_MONO) {
                    const unsigned char* col = &image[y*imageRowStride + x];
                    unsigned char writeData[3] = {*col, *col, *col};
                    strm.write(reinterpret_cast<const char*>(writeData), sizeof(writeData));
                } else if(format == ImageSet::FORMAT_12_BIT_MONO) {
                    const unsigned short* col = reinterpret_cast<const unsigned short*>(&image[y*imageRowStride + 2*x]);
                    unsigned char writeData[3] = {
                        (unsigned char)(*col >> 4),
                        (unsigned char)(*col >> 4),
                        (unsigned char)(*col >> 4)
                    };
                    strm.write(reinterpret_cast<const char*>(writeData), sizeof(writeData));
                }
            } else {
                // Write ASCII format
                if(std::isfinite(pointMap[4*i + 2])) {
                    strm << pointMap[4*i]
                        << " " << pointMap[4*i + 1]
                        << " " << pointMap[4*i + 2];
                } else {
                    strm << "NaN NaN NaN";
                }

                if (image == nullptr) {
                    // disparity only, no image data
                    strm << endl;
                } else if(format == ImageSet::FORMAT_8_BIT_RGB) {
                    const unsigned char* col = &image[y*imageRowStride + 3*x];
                    strm << " " << static_cast<int>(col[0])
                        << " " << static_cast<int>(col[1])
                        << " " << static_cast<int>(col[2]) << endl;
                } else if(format == ImageSet::FORMAT_8_BIT_MONO) {
                    const unsigned char* col = &image[y*imageRowStride + x];
                    strm << " " << static_cast<int>(*col)
                        << " " << static_cast<int>(*col)
                        << " " << static_cast<int>(*col) << endl;
                } else if(format == ImageSet::FORMAT_12_BIT_MONO) {
                    const unsigned short* col = reinterpret_cast<const unsigned short*>(&image[y*imageRowStride + 2*x]);
                    strm << " " << static_cast<int>(*col >> 4)
                        << " " << static_cast<int>(*col >> 4)
                        << " " << static_cast<int>(*col >> 4) << endl;
                }
            }
        }
    }
}

void Reconstruct3D::Pimpl::writePlyFile(const char* file, const ImageSet& imageSet,
        double maxZ, bool binary, ColorSource colSource, unsigned short maxDisparity) {

    ImageSet::ImageType colImg = getColorImage(imageSet, colSource);

    int indexDisp = imageSet.getIndexOf(ImageSet::IMAGE_DISPARITY);
    int indexImg = imageSet.getIndexOf(colImg);
    if(indexDisp == -1) {
        throw std::runtime_error("No disparity channel present, cannot create point map!");
    }
    if(imageSet.getPixelFormat(ImageSet::IMAGE_DISPARITY) != ImageSet::FORMAT_12_BIT_MONO) {
        throw std::runtime_error("Disparity map must have 12-bit pixel format!");
    }

    // write Ply file, passing image data for point colors, if available
    writePlyFile(file, reinterpret_cast<unsigned short*>(imageSet.getPixelData(indexDisp)),
        (indexImg == -1) ? nullptr : imageSet.getPixelData(indexImg),
        imageSet.getWidth(), imageSet.getHeight(),
        (indexImg == -1) ? ImageSet::FORMAT_8_BIT_MONO : imageSet.getPixelFormat(indexImg),
        imageSet.getRowStride(indexDisp),
        (indexImg == -1) ? 0 : imageSet.getRowStride(indexImg),
        imageSet.getQMatrix(),
        maxZ, binary, imageSet.getSubpixelFactor(), maxDisparity);
}

} // namespace


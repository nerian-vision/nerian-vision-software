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

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cstring>
#include "visiontransfer/imageset.h"

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

using namespace visiontransfer;

namespace visiontransfer {

ImageSet::ImageSet()
    : width(0), height(0), qMatrix(NULL), timeSec(0), timeMicrosec(0),
        seqNum(0), minDisparity(0), maxDisparity(0), subpixelFactor(16),
        referenceCounter(NULL), numberOfImages(2), indexLeftImage(0), indexRightImage(1), indexDisparityImage(-1),
        indexColorImage(-1), exposureTime(0), lastSyncPulseSec(0), lastSyncPulseMicrosec(0) {
    for (int i=0; i<MAX_SUPPORTED_IMAGES; ++i) {
        formats[i] = FORMAT_8_BIT_MONO;
        data[i] = NULL;
        rowStride[i] = 0;
    }
}

ImageSet::ImageSet(const ImageSet& other) {
    copyData(*this, other, true);
}

ImageSet& ImageSet::operator= (ImageSet const& other) {
    if(&other != this) {
        decrementReference();
        copyData(*this, other, true);
    }
    return *this;
}

ImageSet::~ImageSet() {
    decrementReference();
}

void ImageSet::copyData(ImageSet& dest, const ImageSet& src, bool countRef) {
    dest.width = src.width;
    dest.height = src.height;

    dest.numberOfImages = src.numberOfImages;
    for(int i=0; i<MAX_SUPPORTED_IMAGES; i++) {
        dest.rowStride[i] = src.rowStride[i];
        dest.formats[i] = src.formats[i];
        dest.data[i] = src.data[i];
    }

    dest.qMatrix = src.qMatrix;
    dest.timeSec = src.timeSec;
    dest.timeMicrosec = src.timeMicrosec;
    dest.seqNum = src.seqNum;
    dest.minDisparity = src.minDisparity;
    dest.maxDisparity = src.maxDisparity;
    dest.subpixelFactor = src.subpixelFactor;
    dest.referenceCounter = src.referenceCounter;
    dest.numberOfImages = src.numberOfImages;
    dest.indexLeftImage = src.indexLeftImage;
    dest.indexRightImage = src.indexRightImage;
    dest.indexDisparityImage = src.indexDisparityImage;
    dest.indexColorImage = src.indexColorImage;
    dest.exposureTime = src.exposureTime;
    dest.lastSyncPulseSec = src.lastSyncPulseSec;
    dest.lastSyncPulseMicrosec = src.lastSyncPulseMicrosec;

    if(dest.referenceCounter != nullptr && countRef) {
        (*dest.referenceCounter)++;
    }
}

void ImageSet::decrementReference() {
    if(referenceCounter != nullptr && --(*referenceCounter) == 0) {
        for (int i=0; i<getNumberOfImages(); ++i) {
            delete []data[i];
            data[i] = nullptr;
        }
        delete []qMatrix;
        delete referenceCounter;

        qMatrix = nullptr;
        referenceCounter = nullptr;
    }
}

void ImageSet::writePgmFile(int imageNumber, const char* fileName) const {
    if(imageNumber < 0 || imageNumber >= getNumberOfImages()) {
        throw std::runtime_error("Illegal image number!");
    }

    std::fstream strm(fileName, std::ios::out | std::ios::binary);

    // Write PGM / PBM header
    int type, maxVal, bytesPerChannel, channels;
    switch(formats[imageNumber]) {
        case FORMAT_8_BIT_MONO:
            type = 5;
            maxVal = 255;
            bytesPerChannel = 1;
            channels = 1;
            break;
        case FORMAT_12_BIT_MONO:
            type = 5;
            maxVal = 4095;
            bytesPerChannel = 2;
            channels = 1;
            break;
        case FORMAT_8_BIT_RGB:
            type = 6;
            maxVal = 255;
            bytesPerChannel = 1;
            channels = 3;
            break;
        default:
            throw std::runtime_error("Illegal pixel format!");
    }

    strm << "P" << type << " " << width << " " << height << " " << maxVal << std::endl;

    // Write image data
    for(int y = 0; y < height; y++) {
        for(int x = 0; x < width*channels; x++) {
            unsigned char* pixel = &data[imageNumber][y*rowStride[imageNumber] + x*bytesPerChannel];
            if(bytesPerChannel == 2) {
                // Swap endianess
                unsigned short swapped = htons(*reinterpret_cast<unsigned short*>(pixel));
                strm.write(reinterpret_cast<char*>(&swapped), sizeof(swapped));
            } else {
                strm.write(reinterpret_cast<char*>(pixel), 1);
            }
        }
    }
}

int ImageSet::getBitsPerPixel(ImageFormat format) {
    switch(format) {
        case FORMAT_8_BIT_MONO: return 8;
        case FORMAT_8_BIT_RGB: return 24;
        case FORMAT_12_BIT_MONO: return 12;
        default: throw std::runtime_error("Invalid image format!");
    }
}

void ImageSet::copyTo(ImageSet& dest) {
    dest.decrementReference();
    copyData(dest, *this, false);

    dest.qMatrix = new float[16];
    memcpy(const_cast<float*>(dest.qMatrix), qMatrix, sizeof(float)*16);

    for(int i=0; i<getNumberOfImages(); i++) {
        int bytesPixel = getBytesPerPixel(i);

        dest.rowStride[i] = width*bytesPixel;
        dest.data[i] = new unsigned char[height*dest.rowStride[i]];

        // Convert possibly different row strides
        for(int y = 0; y < height; y++) {
            memcpy(&dest.data[i][y*dest.rowStride[i]], &data[i][y*rowStride[i]],
                dest.rowStride[i]);
        }
    }

    dest.referenceCounter = new int;
    (*dest.referenceCounter) = 1;
}

int ImageSet::getBytesPerPixel(ImageFormat format) {
    switch(format) {
        case FORMAT_8_BIT_MONO: return 1;
        case FORMAT_8_BIT_RGB: return 3;
        case FORMAT_12_BIT_MONO: return 2;
        default: throw std::runtime_error("Invalid image format!");
    }
}

ImageSet::ImageType ImageSet::getImageType(int imageNumber) const {
    assert(imageNumber >= 0 && imageNumber < getNumberOfImages());
    if (imageNumber == getIndexOf(ImageSet::ImageType::IMAGE_LEFT)) return ImageSet::ImageType::IMAGE_LEFT;
    if (imageNumber == getIndexOf(ImageSet::ImageType::IMAGE_RIGHT)) return ImageSet::ImageType::IMAGE_RIGHT;
    if (imageNumber == getIndexOf(ImageSet::ImageType::IMAGE_DISPARITY)) return ImageSet::ImageType::IMAGE_DISPARITY;
    if (imageNumber == getIndexOf(ImageSet::ImageType::IMAGE_COLOR)) return ImageSet::ImageType::IMAGE_COLOR;
    throw std::runtime_error("Invalid image number for getImageType!");
}

void ImageSet::setImageDisparityPair(bool dispPair) {
    if (getNumberOfImages() != 2) throw std::runtime_error("setImageDisparityPair is only supported for two-image sets");
    // Let index assignments directly follow the mode
    indexLeftImage = 0;
    indexRightImage = dispPair ? -1 : 1;
    indexDisparityImage = dispPair ? 1 : -1;
}

int ImageSet::getIndexOf(ImageType what, bool throwIfNotFound) const {
    int idx = -1;
    switch(what) {
        case IMAGE_LEFT: {
                idx = indexLeftImage;
                break;
            }
        case IMAGE_RIGHT: {
                idx = indexRightImage;
                break;
            }
        case IMAGE_DISPARITY: {
                idx = indexDisparityImage;
                break;
            }
        case IMAGE_COLOR: {
                idx = indexColorImage;
                break;
            }
        default:
            throw std::runtime_error("Invalid ImageType for query!");
    }
    if (throwIfNotFound && (idx==-1)) throw std::runtime_error("ImageSet does not contain the queried ImageType");
    return idx;
}

void ImageSet::setIndexOf(ImageType what, int idx) {
    switch(what) {
        case IMAGE_LEFT: {
                indexLeftImage = idx;
                break;
            }
        case IMAGE_RIGHT: {
                indexRightImage = idx;
                break;
            }
        case IMAGE_DISPARITY: {
                indexDisparityImage = idx;
                break;
            }
        case IMAGE_COLOR: {
                indexColorImage = idx;
                break;
            }
        default:
            throw std::runtime_error("Invalid ImageType for setIndexOf!");
    }
}
} // namespace


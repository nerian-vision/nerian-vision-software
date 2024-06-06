/*******************************************************************************
 * Copyright (c) 2024 Allied Vision Technologies GmbH
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

// Pimpl (implementation) class

class ImageSet::Pimpl {
public:
    /**
     * \brief Default constructor creating an image set with no pixel data.
     */
    Pimpl();

    Pimpl(const Pimpl& other);

    ~Pimpl();
    Pimpl& operator= (Pimpl const& other);

    void setWidth(int w) {width = w;}

    void setHeight(int h) {height = h;}

    void setRowStride(int imageNumber, int stride) {
        assert(imageNumber >= 0 && imageNumber < getNumberOfImages());
        rowStride[imageNumber] = stride;
    }

    void setPixelFormat(int imageNumber, ImageFormat format) {
        assert(imageNumber >= 0 && imageNumber < getNumberOfImages());
        formats[imageNumber] = format;
    }

    void setPixelData(int imageNumber, unsigned char* pixelData) {
        assert(imageNumber >= 0 && imageNumber < getNumberOfImages());
        data[imageNumber] = pixelData;
    }

    void setQMatrix(const float* q) {
        qMatrix = q;
    }

    void setSequenceNumber(unsigned int num) {
        seqNum = num;
    }

    void setTimestamp(int seconds, int microsec) {
        timeSec = seconds;
        timeMicrosec = microsec;
    }

    void setDisparityRange(int minimum, int maximum) {
        minDisparity = minimum;
        maxDisparity = maximum;
    }

    void setSubpixelFactor(int subpixFact) {
        subpixelFactor = subpixFact;
    }

    void setImageDisparityPair(bool dispPair);

    int getWidth() const {return width;}

    int getHeight() const {return height;}

    int getRowStride(int imageNumber) const {
        assert(imageNumber >= 0 && imageNumber < getNumberOfImages());
        return rowStride[imageNumber];
    }

    int getRowStride(ImageType what) const {
        int idx = getIndexOf(what, true);
        return getRowStride(idx);
    }

    ImageFormat getPixelFormat(int imageNumber) const {
        assert(imageNumber >= 0 && imageNumber < getNumberOfImages());
        return formats[imageNumber];
    }

    ImageFormat getPixelFormat(ImageType what) const {
        int idx = getIndexOf(what, true);
        return getPixelFormat(idx);
    }

    unsigned char* getPixelData(int imageNumber) const {
        assert(imageNumber >= 0 && imageNumber < getNumberOfImages());
        return data[imageNumber];
    }

    unsigned char* getPixelData(ImageType what) const {
        int idx = getIndexOf(what, true);
        return getPixelData(idx);
    }

    const float* getQMatrix() const {
        return qMatrix;
    }

    unsigned int getSequenceNumber() const {return seqNum;}

    void getTimestamp(int& seconds, int& microsec) const {
        seconds = timeSec;
        microsec = timeMicrosec;
    }

    void getDisparityRange(int& minimum, int& maximum) const {
        minimum = minDisparity;
        maximum = maxDisparity;
    }

    int getSubpixelFactor() const {
        return subpixelFactor;
    }

    void writePgmFile(int imageNumber, const char* fileName) const;

    void copyTo(ImageSet::Pimpl& dest);

    int getBytesPerPixel(int imageNumber) const {
        assert(imageNumber >= 0 && imageNumber < getNumberOfImages());
        return ImageSet::getBytesPerPixel(formats[imageNumber]);
    }

    int getBitsPerPixel(int imageNumber) const {
        assert(imageNumber >= 0 && imageNumber < getNumberOfImages());
        return ImageSet::getBitsPerPixel(formats[imageNumber]);
    }

    int getBitsPerPixel(ImageType what) const {
        int idx = getIndexOf(what, true);
        return getBitsPerPixel(idx);
    }

    int getNumberOfImages() const {
        return numberOfImages;
    }

    void setNumberOfImages(int number) {
        assert(number >= 1 && number <= ImageSet::MAX_SUPPORTED_IMAGES);
        numberOfImages = number;
    }

    ImageType getImageType(int imageNumber) const;

    int getIndexOf(ImageType what, bool throwIfNotFound=false) const;

    bool hasImageType(ImageType what) const {
        return getIndexOf(what) >= 0;
    }

    void setIndexOf(ImageType what, int idx);

#ifdef CV_MAJOR_VERSION
    /**
     * \brief Converts one image of the set to an OpenCV image.
     *
     * \param imageNumber The number of the image that shall be converted
     *        (0 ... getNumberOfImages()-1).
     * \param convertRgbToBgr If true, then color images will converted from
     *        RGB to BGR in order to comply to OpenCV's convention.
     *
     * For this method to be available, the OpenCV headers need to be
     * included before including headers for libvisiontransfer.
     *
     * Please note that only a shallow copy is performed. The ImageSet object
     * must be kept alive for as long as the OpenCV image is in use.
     */
    inline void toOpenCVImage(int imageNumber, cv::Mat& dest, bool convertRgbToBgr = true);
#endif

    void setExposureTime(int timeMicrosec) {
        exposureTime = timeMicrosec;
    }

    int getExposureTime() const {
        return exposureTime;
    }

    void setLastSyncPulse(int seconds, int microsec) {
        lastSyncPulseSec = seconds;
        lastSyncPulseMicrosec = microsec;
    }

    void getLastSyncPulse(int& seconds, int& microsec) const {
        seconds = lastSyncPulseSec;
        microsec = lastSyncPulseMicrosec;
    }

private:
    int width;
    int height;
    int rowStride[ImageSet::MAX_SUPPORTED_IMAGES];
    ImageFormat formats[ImageSet::MAX_SUPPORTED_IMAGES];
    unsigned char* data[ImageSet::MAX_SUPPORTED_IMAGES];
    const float* qMatrix;
    int timeSec;
    int timeMicrosec;
    unsigned int seqNum;
    int minDisparity;
    int maxDisparity;
    int subpixelFactor;
    int* referenceCounter;
    int numberOfImages;

    int indexLeftImage;
    int indexRightImage;
    int indexDisparityImage;
    int indexColorImage;

    int exposureTime;
    int lastSyncPulseSec;
    int lastSyncPulseMicrosec;

    void copyData(ImageSet::Pimpl& dest, const ImageSet::Pimpl& src, bool countRef);
    void decrementReference();
};


ImageSet::Pimpl::Pimpl()
    : width(0), height(0), qMatrix(NULL), timeSec(0), timeMicrosec(0),
        seqNum(0), minDisparity(0), maxDisparity(0), subpixelFactor(16),
        referenceCounter(NULL), numberOfImages(2), indexLeftImage(0), indexRightImage(1), indexDisparityImage(-1),
        indexColorImage(-1), exposureTime(0), lastSyncPulseSec(0), lastSyncPulseMicrosec(0) {
    for (int i=0; i<ImageSet::MAX_SUPPORTED_IMAGES; ++i) {
        formats[i] = ImageSet::FORMAT_8_BIT_MONO;
        data[i] = NULL;
        rowStride[i] = 0;
    }
}

ImageSet::Pimpl::Pimpl(const ImageSet::Pimpl& other) {
    copyData(*this, other, true);
}

ImageSet::Pimpl& ImageSet::Pimpl::operator= (ImageSet::Pimpl const& other) {
    if(&other != this) {
        decrementReference();
        copyData(*this, other, true);
    }
    return *this;
}

ImageSet::Pimpl::~Pimpl() {
    decrementReference();
}

void ImageSet::Pimpl::copyData(ImageSet::Pimpl& dest, const ImageSet::Pimpl& src, bool countRef) {
    dest.width = src.width;
    dest.height = src.height;

    dest.numberOfImages = src.numberOfImages;
    for(int i=0; i<ImageSet::MAX_SUPPORTED_IMAGES; i++) {
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

void ImageSet::Pimpl::decrementReference() {
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

void ImageSet::Pimpl::writePgmFile(int imageNumber, const char* fileName) const {
    if(imageNumber < 0 || imageNumber >= getNumberOfImages()) {
        throw std::runtime_error("Illegal image number!");
    }

    std::fstream strm(fileName, std::ios::out | std::ios::binary);

    // Write PGM / PBM header
    int type, maxVal, bytesPerChannel, channels;
    switch(formats[imageNumber]) {
        case ImageSet::FORMAT_8_BIT_MONO:
            type = 5;
            maxVal = 255;
            bytesPerChannel = 1;
            channels = 1;
            break;
        case ImageSet::FORMAT_12_BIT_MONO:
            type = 5;
            maxVal = 4095;
            bytesPerChannel = 2;
            channels = 1;
            break;
        case ImageSet::FORMAT_8_BIT_RGB:
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

void ImageSet::Pimpl::copyTo(ImageSet::Pimpl& dest) {
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

ImageSet::ImageType ImageSet::Pimpl::getImageType(int imageNumber) const {
    assert(imageNumber >= 0 && imageNumber < getNumberOfImages());
    if (imageNumber == getIndexOf(ImageSet::ImageType::IMAGE_LEFT)) return ImageSet::ImageType::IMAGE_LEFT;
    if (imageNumber == getIndexOf(ImageSet::ImageType::IMAGE_RIGHT)) return ImageSet::ImageType::IMAGE_RIGHT;
    if (imageNumber == getIndexOf(ImageSet::ImageType::IMAGE_DISPARITY)) return ImageSet::ImageType::IMAGE_DISPARITY;
    if (imageNumber == getIndexOf(ImageSet::ImageType::IMAGE_COLOR)) return ImageSet::ImageType::IMAGE_COLOR;
    throw std::runtime_error("Invalid image number for getImageType!");
}

void ImageSet::Pimpl::setImageDisparityPair(bool dispPair) {
    if (getNumberOfImages() != 2) throw std::runtime_error("setImageDisparityPair is only supported for two-image sets");
    // Let index assignments directly follow the mode
    indexLeftImage = 0;
    indexRightImage = dispPair ? -1 : 1;
    indexDisparityImage = dispPair ? 1 : -1;
}

int ImageSet::Pimpl::getIndexOf(ImageType what, bool throwIfNotFound) const {
    int idx = -1;
    switch(what) {
        case ImageSet::IMAGE_LEFT: {
                idx = indexLeftImage;
                break;
            }
        case ImageSet::IMAGE_RIGHT: {
                idx = indexRightImage;
                break;
            }
        case ImageSet::IMAGE_DISPARITY: {
                idx = indexDisparityImage;
                break;
            }
        case ImageSet::IMAGE_COLOR: {
                idx = indexColorImage;
                break;
            }
        default:
            throw std::runtime_error("Invalid ImageType for query!");
    }
    if (throwIfNotFound && (idx==-1)) throw std::runtime_error("ImageSet does not contain the queried ImageType");
    return idx;
}

void ImageSet::Pimpl::setIndexOf(ImageType what, int idx) {
    switch(what) {
        case ImageSet::IMAGE_LEFT: {
                indexLeftImage = idx;
                break;
            }
        case ImageSet::IMAGE_RIGHT: {
                indexRightImage = idx;
                break;
            }
        case ImageSet::IMAGE_DISPARITY: {
                indexDisparityImage = idx;
                break;
            }
        case ImageSet::IMAGE_COLOR: {
                indexColorImage = idx;
                break;
            }
        default:
            throw std::runtime_error("Invalid ImageType for setIndexOf!");
    }
}

//
//
// API class (externally exported)
//
//

ImageSet::ImageSet()
: pimpl(new Pimpl())
{
}

ImageSet::ImageSet(const ImageSet& other)
: pimpl(new Pimpl(*(other.pimpl))) {

}

ImageSet::~ImageSet() {
    delete pimpl;
}

ImageSet& ImageSet::operator= (ImageSet const& other) {
    (*pimpl) = *(other.pimpl);
    return *this;
}

void ImageSet::setWidth(int w) {
    pimpl->setWidth(w);
}

void ImageSet::setHeight(int h) {
    pimpl->setHeight(h);
}

void ImageSet::setRowStride(int imageNumber, int stride) {
    pimpl->setRowStride(imageNumber, stride);
}

void ImageSet::setPixelFormat(int imageNumber, ImageSet::ImageFormat format) {
    pimpl->setPixelFormat(imageNumber, format);
}

void ImageSet::setPixelData(int imageNumber, unsigned char* pixelData) {
    pimpl->setPixelData(imageNumber, pixelData);
}

void ImageSet::setQMatrix(const float* q) {
    pimpl->setQMatrix(q);
}

void ImageSet::setSequenceNumber(unsigned int num) {
    pimpl->setSequenceNumber(num);
}

void ImageSet::setTimestamp(int seconds, int microsec) {
    pimpl->setTimestamp(seconds, microsec);
}

void ImageSet::setDisparityRange(int minimum, int maximum) {
    pimpl->setDisparityRange(minimum, maximum);
}

void ImageSet::setSubpixelFactor(int subpixFact) {
    pimpl->setSubpixelFactor(subpixFact);
}

void ImageSet::setImageDisparityPair(bool dispPair) {
    pimpl->setImageDisparityPair(dispPair);
}

int ImageSet::getWidth() const {
    return pimpl->getWidth();
}

int ImageSet::getHeight() const {
    return pimpl->getHeight();
}

int ImageSet::getRowStride(int imageNumber) const {
    return pimpl->getRowStride(imageNumber);
}

int ImageSet::getRowStride(ImageSet::ImageType what) const {
    return pimpl->getRowStride(what);
}

ImageSet::ImageFormat ImageSet::getPixelFormat(int imageNumber) const {
    return pimpl->getPixelFormat(imageNumber);
}

ImageSet::ImageFormat ImageSet::getPixelFormat(ImageSet::ImageType what) const {
    return pimpl->getPixelFormat(what);
}

unsigned char* ImageSet::getPixelData(int imageNumber) const {
    return pimpl->getPixelData(imageNumber);
}

unsigned char* ImageSet::getPixelData(ImageSet::ImageType what) const {
    return pimpl->getPixelData(what);
}

const float* ImageSet::getQMatrix() const {
    return pimpl->getQMatrix();
}

unsigned int ImageSet::getSequenceNumber() const {
    return pimpl->getSequenceNumber();
}

void ImageSet::getTimestamp(int& seconds, int& microsec) const {
    pimpl->getTimestamp(seconds, microsec);
}

void ImageSet::getDisparityRange(int& minimum, int& maximum) const {
    pimpl->getDisparityRange(minimum, maximum);
}

int ImageSet::getSubpixelFactor() const {
    return pimpl->getSubpixelFactor();
}

void ImageSet::writePgmFile(int imageNumber, const char* fileName) const {
    pimpl->writePgmFile(imageNumber, fileName);
}

void ImageSet::copyTo(ImageSet& dest) {
    pimpl->copyTo(*(dest.pimpl));
}

int ImageSet::getBytesPerPixel(int imageNumber) const {
    return pimpl->getBytesPerPixel(imageNumber);
}

int ImageSet::getBitsPerPixel(int imageNumber) const {
    return pimpl->getBitsPerPixel(imageNumber);
}

int ImageSet::getBitsPerPixel(ImageSet::ImageType what) const {
    return pimpl->getBitsPerPixel(what);
}

int ImageSet::getNumberOfImages() const {
    return pimpl->getNumberOfImages();
}

void ImageSet::setNumberOfImages(int number) {
    pimpl->setNumberOfImages(number);
}

ImageSet::ImageType ImageSet::getImageType(int imageNumber) const {
    return pimpl->getImageType(imageNumber);
}

int ImageSet::getIndexOf(ImageSet::ImageType what, bool throwIfNotFound) const {
    return pimpl->getIndexOf(what, throwIfNotFound);
}

bool ImageSet::hasImageType(ImageSet::ImageType what) const {
    return pimpl->hasImageType(what);
}

void ImageSet::setIndexOf(ImageSet::ImageType what, int idx) {
    pimpl->setIndexOf(what, idx);
}

#ifdef CV_MAJOR_VERSION
inline void ImageSet::toOpenCVImage(int imageNumber, cv::Mat& dest, bool convertRgbToBgr) {
    pimpl->toOpenCVImage(imageNumber, dest, convertRgbToBgr);
}
#endif

void ImageSet::setExposureTime(int timeMicrosec) {
    pimpl->setExposureTime(timeMicrosec);
}

int ImageSet::getExposureTime() const {
    return pimpl->getExposureTime();
}

void ImageSet::setLastSyncPulse(int seconds, int microsec) {
    pimpl->setLastSyncPulse(seconds, microsec);
}

void ImageSet::getLastSyncPulse(int& seconds, int& microsec) const {
    pimpl->getLastSyncPulse(seconds, microsec);
}

// static
int ImageSet::getBitsPerPixel(ImageFormat format) {
    switch(format) {
        case ImageSet::FORMAT_8_BIT_MONO: return 8;
        case ImageSet::FORMAT_8_BIT_RGB: return 24;
        case ImageSet::FORMAT_12_BIT_MONO: return 12;
        default: throw std::runtime_error("Invalid image format!");
    }
}

// static
int ImageSet::getBytesPerPixel(ImageFormat format) {
    switch(format) {
        case ImageSet::FORMAT_8_BIT_MONO: return 1;
        case ImageSet::FORMAT_8_BIT_RGB: return 3;
        case ImageSet::FORMAT_12_BIT_MONO: return 2;
        default: throw std::runtime_error("Invalid image format!");
    }
}


} // namespace


/*******************************************************************************
 * Copyright (c) 2023 Nerian Vision GmbH
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

#include "stream/buffermapping.h"
#include "device/physicaldevice.h"

using namespace visiontransfer;

namespace GenTL {

// Default constructor: empty buffer mapping
BufferMapping::BufferMapping()
: numBufferParts(0), totalBufferSize(0)
{
}

BufferMapping::BufferMapping(const ImageSet& metaData, int intensitySource, bool componentEnabledRange)
: metaData(metaData), intensitySource(intensitySource), componentEnabledRange(componentEnabledRange)
{
    // Calculates and caches the effective number of buffer parts, the indices in the ImageSet, buffer part offsets and sizes, and the purpose ID for each part
    numBufferParts = 0;

    // Intensity, selected based on preference and availability in the ImageSet
    activeIntensityChannel = ImageSet::IMAGE_UNDEFINED;
    if (metaData.hasImageType(ImageSet::IMAGE_COLOR) && (intensitySource!=(int) PhysicalDevice::INTENSITY_SOURCE_LEFT)) {
        activeIntensityChannel = ImageSet::IMAGE_COLOR;
    } else if (metaData.hasImageType(ImageSet::IMAGE_LEFT)) {
        activeIntensityChannel = ImageSet::IMAGE_LEFT;
    }
    if (activeIntensityChannel != ImageSet::IMAGE_UNDEFINED) {
        imageSetFunctionMapping[numBufferParts] = activeIntensityChannel;
        purposeIDMapping[numBufferParts] = PURPOSE_INTENSITY;
        numBufferParts++;
    }

    // Disparity
    if (metaData.hasImageType(ImageSet::IMAGE_DISPARITY)) {
        imageSetFunctionMapping[numBufferParts] = ImageSet::IMAGE_DISPARITY;
        purposeIDMapping[numBufferParts] = PURPOSE_DISPARITY;
        numBufferParts++;
    }

    // Right cam, if present
    if (metaData.hasImageType(ImageSet::IMAGE_RIGHT)) {
        imageSetFunctionMapping[numBufferParts] = ImageSet::IMAGE_RIGHT;
        purposeIDMapping[numBufferParts] = PURPOSE_CUSTOM_0;
        numBufferParts++;
    }

    // Range - not possible without disparity - and unless turned off
    if (metaData.hasImageType(ImageSet::IMAGE_DISPARITY) && componentEnabledRange) {
        imageSetFunctionMapping[numBufferParts] = ImageSet::IMAGE_UNDEFINED; // UNDEFINED is the special handling for the point cloud (i.e. not a channel in the ImageSet)
        purposeIDMapping[numBufferParts] = PURPOSE_RANGE;
        numBufferParts++;
    }

    // Third camera (left or color; the one not selected for Intensity)
    // - we append this (and any new channels) after the range data, for backward compatibility
    ImageSet::ImageType extraType = (intensitySource==(int) PhysicalDevice::INTENSITY_SOURCE_LEFT) ? ImageSet::IMAGE_COLOR : ImageSet::IMAGE_LEFT;
    if ((extraType != activeIntensityChannel) && metaData.hasImageType(extraType)) {
        imageSetFunctionMapping[numBufferParts] = extraType;
        purposeIDMapping[numBufferParts] = PURPOSE_CUSTOM_1;
        numBufferParts++;
    }

    // Cache buffer part offsets and sizes
    int offset = 0;
    for (int i=0; i<numBufferParts; ++i) {
        int sz;
        ImageSet::ImageType imageTypeForThisPart = imageSetFunctionMapping[i];
        if (imageTypeForThisPart == ImageSet::IMAGE_UNDEFINED) { // specially marked; point cloud
            // Reconstruct3D point cloud buffer size
            sz = metaData.getWidth() * metaData.getHeight() * 3*sizeof(float);
        } else {
            // Image buffer size
            int bytespp = metaData.getBytesPerPixel(metaData.getPixelFormat(imageTypeForThisPart));
            sz = metaData.getWidth() * metaData.getHeight() * bytespp; // cf. "newStride" in PhysicalDevice::copyImageToBufferMemory
        }
        bufferPartSize[i] = sz;
        bufferPartOffset[i] = offset;
        offset += sz;
    }
    totalBufferSize = offset;
}

int BufferMapping::getNumBufferParts() const {
    return numBufferParts;
}

int BufferMapping::getTotalBufferSize() const {
    return totalBufferSize;
}

int BufferMapping::getBufferPartOffset(int partIdx) const {
    if ((partIdx < 0) || (partIdx >= numBufferParts)) throw std::runtime_error("Invalid buffer part index");
    return bufferPartOffset[partIdx];
}

int BufferMapping::getBufferPartSize(int partIdx) const {
    if ((partIdx < 0) || (partIdx >= numBufferParts)) throw std::runtime_error("Invalid buffer part index");
    return bufferPartSize[partIdx];
}

ImageSet::ImageType BufferMapping::getBufferPartImageSetFunction(int partIdx) const {
    if ((partIdx < 0) || (partIdx >= numBufferParts)) throw std::runtime_error("Invalid buffer part index");
    return imageSetFunctionMapping[partIdx];
}

uint64_t BufferMapping::getBufferPartPixelFormat(int partIdx) const {
    if ((partIdx < 0) || (partIdx >= numBufferParts)) throw std::runtime_error("Invalid buffer part index");
    ImageSet::ImageType imageTypeForThisPart = imageSetFunctionMapping[partIdx];
    if (imageTypeForThisPart == ImageSet::IMAGE_UNDEFINED) { // specially marked; point cloud
        // Point cloud
        return 0x026000C0; // "Coord3D_ABC32f"
    } else {
        // Image
        auto pixFmt = metaData.getPixelFormat(imageTypeForThisPart);
        if (pixFmt == ImageSet::FORMAT_8_BIT_RGB) {
            return 0x02180014; //"RGB8"
        } else if(pixFmt == ImageSet::FORMAT_12_BIT_MONO) { // true for both 12bit-mono images and disparity
            return 0x01100005; //"Mono12"
        } else {
            // Fallback for unhandled pixel format
            return 0x01080001; //"Mono8"
        }
    }
}

int BufferMapping::getBufferPartPurposeID(int partIdx) const {
    if ((partIdx < 0) || (partIdx >= numBufferParts)) throw std::runtime_error("Invalid buffer part index");
    return purposeIDMapping[partIdx];
}

void BufferMapping::dumpToStream(std::ostream& os) const {
    os << "BufferMapping with " << numBufferParts << " parts; ";
    os << "total buffer size: " << totalBufferSize << " (" << (totalBufferSize >> 10) << " KiB):" << std::endl;
    for (int i=0; i<numBufferParts; ++i) {
        os << " Index " << i << " -- Purpose: ";
        switch (purposeIDMapping[i]) {
            case PURPOSE_INTENSITY: {
                    os << "INTENSITY";
                    os << ((activeIntensityChannel==ImageSet::IMAGE_COLOR)?"(color)":"(left)");
                    break;
                }
            case PURPOSE_DISPARITY: {
                    os << "DISPARITY";
                    break;
                }
            case PURPOSE_RANGE: {
                    os << "RANGE";
                    break;
                }
            case PURPOSE_CUSTOM_0: {
                    os << "CUSTOM+0(right)";
                    break;
               }
            case PURPOSE_CUSTOM_1: {
                    os << "CUSTOM+1";
                    os << ((activeIntensityChannel==ImageSet::IMAGE_COLOR)?"(left)":"(color)");
                    break;
               }
            default:
                os << "(INVALID!)";
        }
        os << " -- Buffer offset: " << bufferPartOffset[i] << " -- Size: " << bufferPartSize[i] << " (" << (bufferPartSize[i] >> 10) << " KiB)";
        os << std::endl;
    }
}

} // namespace GenTL


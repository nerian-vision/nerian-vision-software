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

#include "device/deviceportimpl.h"
#include "device/logicaldevice.h"
#include "device/physicaldevice.h"
#include "stream/datastream.h"
#include "misc/infoquery.h"

using namespace visiontransfer;

namespace GenTL {

DevicePortImpl::DevicePortImpl(LogicalDevice* device)
    :device(device) {
}

GC_ERROR DevicePortImpl::readFeature(unsigned int featureId, void* pBuffer, size_t* piSize) {
    INFO_DATATYPE type;
    return device->getInfo(featureId, &type, pBuffer, piSize);
}

GC_ERROR DevicePortImpl::writeSelector(unsigned int selector) {
    if(selector < 0 || selector > 7) {
        return GC_ERR_INVALID_INDEX;
    } else {
        return GC_ERR_SUCCESS;
    }
}

GC_ERROR DevicePortImpl::readChildFeature(unsigned int selector, unsigned int featureId,
        void* pBuffer, size_t* piSize) {

    InfoQuery info(nullptr, pBuffer, piSize);

    std::string id = device->getId();
    const ImageSet& metaData = device->getPhysicalDevice()->getLatestMetaData();
    bool hasIntensityStream = metaData.hasImageType(ImageSet::IMAGE_LEFT);
    bool hasDisparityStream = metaData.hasImageType(ImageSet::IMAGE_DISPARITY);

    // Possible values of the component selector
    enum ComponentSelector {
        Intensity,
        Disparity,
        Range
    };

    DataStream::StreamType streamType = device->getStream()->getStreamType();

    switch(featureId) {
        // Common device info, not related to component selector
        case 0: // width
            info.setUInt(metaData.getWidth());
            break;
        case 1: // height
            info.setUInt(metaData.getHeight());
            break;
        case 2: // Pixelformat
            info.setUInt(static_cast<unsigned int>(device->getStream()->getPixelFormat(
                device->getPhysicalDevice()->getLatestMetaData())));
            break;
        case 3: // Payload size
            info.setUInt(static_cast<unsigned int>(device->getStream()->getPayloadSize()));
            break;

        // Component selector specific info
        case 4:  // Enable
            {
                const unsigned int INTENSITY_BIT = 1;
                const unsigned int DISPARITY_BIT = 2;
                const unsigned int RANGE_BIT     = 4;
                if(streamType == DataStream::MULTIPART_STREAM) { // Multipart
                    info.setUInt((hasIntensityStream?INTENSITY_BIT:0) + RANGE_BIT + (hasDisparityStream?DISPARITY_BIT:0));
                } else if(streamType == DataStream::IMAGE_LEFT_STREAM || streamType == DataStream::IMAGE_RIGHT_STREAM) {
                    info.setUInt(INTENSITY_BIT);
                } else if(streamType == DataStream::DISPARITY_STREAM) {
                    info.setUInt(DISPARITY_BIT);
                } else if(streamType == DataStream::POINTCLOUD_STREAM) {
                    info.setUInt(RANGE_BIT);
                } else {
                    info.setUInt(0);
                }
            }
            break;
        case 5: // Id
            if(streamType != DataStream::MULTIPART_STREAM) {
                info.setUInt(0); // Only one component
            } else {
                switch(selector) {
                    case Intensity:
                        info.setUInt(0);
                        break;
                    case Disparity:
                        info.setUInt(1);
                        break;
                    case Range:
                        info.setUInt(2);
                        break;
                    default:
                        info.setUInt(0);
                }
            }
            break;

        case 6: // 3D: focal length
            {
                const ImageSet& metaData = device->getPhysicalDevice()->getLatestMetaData();
                const float* q = metaData.getQMatrix();
                info.setDouble(q[11]);
            }
            break;

        case 7: // 3D: baseline
            {
                const ImageSet& metaData = device->getPhysicalDevice()->getLatestMetaData();
                const float* q = metaData.getQMatrix();
                info.setDouble(1.0 / q[14]);
            }
            break;
        case 8: // 3D: invalid data value
            {
                float inval = device->getPhysicalDevice()->getInvalidDepthValue();
                info.setDouble(inval);
            }
            break;
        case 9: // 3D: X offset ('PrincipalPointU')
            {
                const ImageSet& metaData = device->getPhysicalDevice()->getLatestMetaData();
                const float* q = metaData.getQMatrix();
                info.setDouble(-q[3]);
            }
            break;
        case 0xa: // 3D: Y offset ('PrincipalPointV')
            {
                const ImageSet& metaData = device->getPhysicalDevice()->getLatestMetaData();
                const float* q = metaData.getQMatrix();
                info.setDouble(-q[7]);
            }
            break;

        default:
            return GC_ERR_INVALID_ADDRESS;
    }

    return info.query();
}

}

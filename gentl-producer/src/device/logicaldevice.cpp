/*******************************************************************************
 * Copyright (c) 2023 Allied Vision Technologies GmbH
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

#include "device/logicaldevice.h"
#include "device/physicaldevice.h"
#include "system/system.h"
#include "stream/datastream.h"
#include "misc/infoquery.h"

namespace GenTL {

LogicalDevice::LogicalDevice(PhysicalDevice* physicalDevice, const std::string& id, DataStream::StreamType streamType)
    :Handle(TYPE_DEVICE), physicalDevice(physicalDevice), id(id), stream(this, streamType), portImpl(this),
        remotePort(id.c_str(), "remote.xml", "Device", "Device" , &portImpl),
        localPort(id.c_str(), "device.xml", "DevicePort", "TLDevice", &portImpl), deviceOpen(false) {

    // Set a default value for the component selector
    // that is also enabled by the XML
    switch(streamType) {
        case DataStream::IMAGE_LEFT_STREAM:
            remotePort.setSelector(0);
            break;
        case DataStream::IMAGE_RIGHT_STREAM:
        case DataStream::IMAGE_THIRD_COLOR_STREAM:
            // Not available in multipart stream
            break;
        case DataStream::DISPARITY_STREAM:
            remotePort.setSelector(1);
            break;
        case DataStream::POINTCLOUD_STREAM:
            remotePort.setSelector(2);
            break;
        case DataStream::MULTIPART_STREAM:
            // Default is first component in multipart streams
            remotePort.setSelector(0);
            break;
    }
}

LogicalDevice::~LogicalDevice() {
    close();
}

GC_ERROR LogicalDevice::open() {
    if (deviceOpen) {
        //std::cout << "\033[33;1mDEBUG\033[m LogicalDevice::open() - already opened this same device!" << std::endl;
        return GC_ERR_RESOURCE_IN_USE;
    }
    bool isMulti = stream.getStreamType() == DataStream::MULTIPART_STREAM;
    int currentDevices = physicalDevice->getCurrentLogicalDeviceState();
    //std::cout << "\033[33;1mDEBUG\033[m LogicalDevice::open() with isMulti " << isMulti << " currentDevices " << currentDevices << std::endl;
    if ((currentDevices == 0) // nothing open yet
            || ((currentDevices==1) && (!isMulti)) // another single-part device is open
        ) {
        deviceOpen = true;
        return GC_ERR_SUCCESS;
    } else {
        // We can not open single and multi-part devices at the same time
        // due to the buffer setup in libvisiontransfer
        return GC_ERR_RESOURCE_IN_USE;
    }
}

GC_ERROR LogicalDevice::close() {
    if (!deviceOpen) return GC_ERR_ERROR;
    deviceOpen = false;
    return GC_ERR_SUCCESS;
}

GC_ERROR LogicalDevice::getParentIF(IF_HANDLE* phIface) {
    if(phIface == nullptr) {
        return GC_ERR_INVALID_PARAMETER;
    } else {
        *phIface = physicalDevice->getInterface();
        return GC_ERR_SUCCESS;
    }
}

GC_ERROR LogicalDevice::getInfo(DEVICE_INFO_CMD iInfoCmd, INFO_DATATYPE* piType,
        void* pBuffer, size_t* piSize) {
    return physicalDevice->getInterface()->getDeviceInfo(id.c_str(), iInfoCmd, piType, pBuffer, piSize);
}

GC_ERROR LogicalDevice::getNumDataStreams( uint32_t* piNumDataStreams) {
    *piNumDataStreams = 1;
    return GC_ERR_SUCCESS;
}

GC_ERROR LogicalDevice::getPort(PORT_HANDLE* phRemoteDev) {
    if(phRemoteDev == nullptr) {
        return GC_ERR_INVALID_PARAMETER;
    } else {
        *phRemoteDev = &remotePort;
        return GC_ERR_SUCCESS;
    }
}

GC_ERROR LogicalDevice::getDataStreamID(uint32_t iIndex, char* sDataStreamID,
        size_t* piSize) {
    InfoQuery info(nullptr, sDataStreamID, piSize);
    info.setString("default");
    return info.query();
}

GC_ERROR LogicalDevice::openDataStream(const char* sDataStreamID, DS_HANDLE* phDataStream) {
    if(sDataStreamID == nullptr || std::string(sDataStreamID) != "default") {
        return GC_ERR_INVALID_ID;
    }

    *phDataStream = &stream;
    return stream.open();
}


}

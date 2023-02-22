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

#ifndef NERIAN_LOGICALDEVICE_H
#define NERIAN_LOGICALDEVICE_H

#include "misc/common.h"
#include "misc/handle.h"
#include "misc/port.h"
#include "stream/datastream.h"
#include "device/deviceportimpl.h"
#include <genicam/gentl.h>

namespace GenTL {

class PhysicalDevice;

/*
 * Logical devices can be queried through the GenTL interface. Several
 * logical devices might correspond to the same physical device. This
 * class represents a DEV_HANDLE and encapsulates all device related
 * GenTL functions.
 */
class LogicalDevice: public Handle {
public:
    LogicalDevice(PhysicalDevice* physicalDevice, const std::string& id, DataStream::StreamType streamType);
    ~LogicalDevice();

    // Methods that match functions form the GenTL interface
    GC_ERROR open();
    GC_ERROR close();

    GC_ERROR getParentIF(IF_HANDLE* phIface);
    GC_ERROR getInfo(DEVICE_INFO_CMD iInfoCmd, INFO_DATATYPE* piType,
            void* pBuffer, size_t* piSize);
    GC_ERROR getNumDataStreams(uint32_t* piNumDataStreams);
    GC_ERROR getPort(PORT_HANDLE* phRemoteDev);
    GC_ERROR getDataStreamID(uint32_t iIndex, char* sDataStreamID,
            size_t* piSize);
    GC_ERROR openDataStream(const char* sDataStreamID, DS_HANDLE* phDataStream);

    // Methods that are used internally
    DataStream* getStream() {return &stream;}
    PhysicalDevice* getPhysicalDevice() {return physicalDevice;}
    std::string getId() {return id;}
    Port* getLocalPort() {return &localPort;}
    bool isOpen() {return deviceOpen;}

private:
    PhysicalDevice* physicalDevice; // Pointer to the associated physical device
    std::string id; // ID string for this device
    DataStream stream; // Data stream of this device

    DevicePortImpl portImpl;
    Port remotePort;
    Port localPort;
    bool deviceOpen;
};

}

#endif

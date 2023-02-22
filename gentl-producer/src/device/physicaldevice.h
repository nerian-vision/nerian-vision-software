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

#ifndef NERIAN_PYSICALDEVICE_H
#define NERIAN_PYSICALDEVICE_H

#include "misc/common.h"

#include <genicam/gentl.h>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <visiontransfer/imagetransfer.h>
#include <visiontransfer/reconstruct3d.h>
#include <visiontransfer/deviceparameters.h>
#include <visiontransfer/exceptions.h>

namespace GenTL {

class Interface;
class LogicalDevice;
class Event;

/*
 * A physical device is an actual connection to a device, which may be
 * represented by several logical devices. This class is not a handle type
 * but only used internally.
 */
class PhysicalDevice {
public:
    enum LogicalDeviceIDs {
        ID_MULTIPART,
        ID_IMAGE_LEFT,
        ID_IMAGE_RIGHT,
        ID_DISPARITY,
        ID_POINTCLOUD
    };

    PhysicalDevice(Interface* interface);
    ~PhysicalDevice();

    GC_ERROR open(bool udp, const char* host);
    void close();

    std::unique_lock<std::mutex> lock(){
        return std::unique_lock<std::mutex>(receiveMutex);
    }

    Interface* getInterface() {return interface;}
    LogicalDevice* getLogicalDevice(int index) {return logicalDevices[index].get();}
    const visiontransfer::ImageSet& getLatestMetaData() const {return latestMetaData;}
    bool inUse();

    Event* allocErrorEvent();
    void freeErrorEvent();

    bool isUdp() const {return udp;}
    std::string getHost() const {return host;}

    static int logicalIdToIndex(const char* id);
    static std::string logicalIndexToId(int index);

    inline float getInvalidDepthValue() const { return 1073741824.0f; /* 2^30, so that EVERY client gets it */ }

    void sendSoftwareTriggerRequest();

private:
    Interface* interface; // Associated system object
    std::unique_ptr<visiontransfer::ImageTransfer> imageTf; // Object for receiving image data
    std::unique_ptr<visiontransfer::DeviceParameters> deviceParameters; // Parameter access for sending software trigger

    bool udp; // Indicates if UDP or TCP protocol is used
    std::string host; // Remote host name or local interface address

    bool threadRunning; // Indicates if the receive thread is running
    std::thread receiveThread; // Thread for image reception
    std::mutex receiveMutex; // Mutex for the image reception

    visiontransfer::ImageSet latestMetaData; // Image set object with meta data for the latest frame
    std::unique_ptr<LogicalDevice> logicalDevices[5]; // The data streams associated with this device

    std::condition_variable_any initializedCondition; // Condition for signaling completed initialization
    Event* errorEvent; // Event object for reporting error events
    visiontransfer::Reconstruct3D reconstruct; // Object for 3D reconstruction

    double disparityOffset;
    unsigned short maxDisparity;

    void deviceReceiveThread();
    void copyRawDataToBuffer(const visiontransfer::ImageSet& receivedSet);
    void copy3dDataToBuffer(const visiontransfer::ImageSet& receivedSet);
    void copyMultipartDataToBuffer(const visiontransfer::ImageSet& receivedSet);
    void copyPointsFallback(float* dst, float* src, int numPoints);
    void copyPointsSSE(float* dst, float* src, int numPoints);
    void setTestData(visiontransfer::ImageSet& receivedSet);
    int copyImageToBufferMemory(const visiontransfer::ImageSet& receivedSet, int id, unsigned char* dst, int dstSize);
    int copy3dDataToBufferMemory(const visiontransfer::ImageSet& receivedSet, unsigned char* dst, int dstSize);
};

}
#endif

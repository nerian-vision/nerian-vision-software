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

    static const int NUM_LOGICAL_DEVICES = 6;

    enum LogicalDeviceIDs {
        ID_MULTIPART,
        ID_IMAGE_LEFT,
        ID_IMAGE_RIGHT,
        ID_IMAGE_THIRD_COLOR,
        ID_DISPARITY,
        ID_POINTCLOUD
    };

    enum IntensitySource {
        INTENSITY_SOURCE_AUTO,
        INTENSITY_SOURCE_LEFT,
        INTENSITY_SOURCE_MIDDLE
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

    visiontransfer::param::Parameter getParameter(const std::string& uid) {
#ifdef DELIVER_TEST_DATA
        return visiontransfer::param::Parameter(uid).setName("Test mode dummy").setType(visiontransfer::param::ParameterValue::TYPE_INT).setCurrent<int>(0);
#else
        if (!deviceParameters) throw std::runtime_error("Attempted device parameter access without an active DeviceParameters connection");
        return deviceParameters->getParameter(uid);
#endif
    }


    bool hasParameter(const std::string& uid) {
#ifdef DELIVER_TEST_DATA
        return true;
#else
        if (!deviceParameters) throw std::runtime_error("Attempted device parameter access without an active DeviceParameters connection");
        return deviceParameters->hasParameter(uid);
#endif
    }

    template<typename T>
    void setParameter(const std::string& uid, T value) {
#ifdef DELIVER_TEST_DATA
        return;
#else
        if (!deviceParameters) throw std::runtime_error("Attempted device parameter access without an active DeviceParameters connection");
        deviceParameters->setParameter(uid, value);
#endif
    }

    // Get / set whether 3D reconstruction is active and Range component is available
    inline bool getComponentEnabledRange() const {
        return componentEnabledRange;
    }
    inline void setComponentEnabledRange(bool enabled) {
        componentEnabledRange = enabled;
    }

    // Get / set the preferred source camera for Intensity channel of new multipart streams (for three-camera devices)
    IntensitySource getIntensitySource() const { return intensitySource; }
    void setIntensitySource(IntensitySource src);

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
    std::unique_ptr<LogicalDevice> logicalDevices[NUM_LOGICAL_DEVICES]; // The data streams associated with this device

    std::condition_variable_any initializedCondition; // Condition for signaling completed initialization
    Event* errorEvent; // Event object for reporting error events
    visiontransfer::Reconstruct3D reconstruct; // Object for 3D reconstruction

    double disparityOffset;
    unsigned short maxDisparity;

    bool componentEnabledRange;
    IntensitySource intensitySource;

    void deviceReceiveThread();
    void copyRawDataToBuffer(const visiontransfer::ImageSet& receivedSet);
    void copy3dDataToBuffer(const visiontransfer::ImageSet& receivedSet);
    void copyMultipartDataToBuffer(const visiontransfer::ImageSet& receivedSet);
    void copyPointsFallback(float* dst, float* src, int numPoints);
    void copyPointsSSE(float* dst, float* src, int numPoints);
    void setTestData(visiontransfer::ImageSet& receivedSet);
    int copyImageToBufferMemory(const visiontransfer::ImageSet& receivedSet, int id, unsigned char* dst, int dstSize);
    int copy3dDataToBufferMemory(const visiontransfer::ImageSet& receivedSet, unsigned char* dst, int dstSize);

    void remoteParameterChangeCallback(const std::string& uid);
    void invalidateFeatureFromAsyncEvent(const std::string& featureName);
};

}
#endif

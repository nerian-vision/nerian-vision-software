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

#include "device/physicaldevice.h"
#include "device/logicaldevice.h"
#include "interface/interface.h"
#include "stream/buffer.h"
#include "stream/buffermapping.h"
#include "misc/testdata.h"
#include "event/event.h"
#include "misc/common.h"

#include <cmath>
#include <cstdlib>
#include <algorithm>

#include <iostream>
#include <fstream>

// SIMD Headers
#ifdef __SSE2__
#include <emmintrin.h>
#endif

using namespace visiontransfer;

namespace GenTL {

using namespace std::placeholders;

#ifdef ENABLE_DEBUGGING
#ifdef _WIN32
    std::fstream debugStreamPhys("C:\\debug\\gentl-debug-phys-" + std::to_string(time(nullptr)) + ".txt", std::ios::out);
#else
    std::ostream& debugStreamPhys = std::cout;
#endif
#define DEBUG_PHYS(x) debugStreamPhys << x << std::endl;
#else
#define DEBUG_PHYS(x) ;
#endif

PhysicalDevice::PhysicalDevice(Interface* interface): interface(interface), udp(true), threadRunning(false),
    errorEvent(nullptr), disparityOffset(0.0), maxDisparity(0xFFF), componentEnabledRange(true),
    intensitySource(INTENSITY_SOURCE_AUTO) {

    const char* offsetEnv = getenv("NERIAN_DISPARITY_OFFSET");
    if(offsetEnv != nullptr) {
        disparityOffset = atof(offsetEnv);
    }
    DEBUG_PHYS("Created a PhysicalDevice");
}

PhysicalDevice::~PhysicalDevice() {
    close();
    DEBUG_PHYS("Deallocating error event");
    freeErrorEvent();
    DEBUG_PHYS("Destroying a PhysicalDevice");
}

Event* PhysicalDevice::allocErrorEvent() {
    if(errorEvent != nullptr) {
        return nullptr;
    } else {
        errorEvent = new Event(EVENT_ERROR, sizeof(GC_ERROR), nullptr);
        return errorEvent;
    }
}

void PhysicalDevice::freeErrorEvent() {
    if(errorEvent != nullptr) {
        delete errorEvent;
        errorEvent = nullptr;
    }
}

GC_ERROR PhysicalDevice::open(bool udp, const char* host) {
    // Open device
    try {
        // Initialize members
        this->udp = udp;
        this->host = host;

#ifndef DELIVER_TEST_DATA
        // Initialize network receiver
        //imageTf.reset(new ImageTransfer(host, "7681",
        //    udp ? ImageProtocol::PROTOCOL_UDP : ImageProtocol::PROTOCOL_TCP));
        asyncTf.reset(new AsyncTransfer(host, "7681",
            udp ? ImageProtocol::PROTOCOL_UDP : ImageProtocol::PROTOCOL_TCP));
        // Initialize parameter server connection
        DEBUG_PHYS("Creating DeviceParameters");
        deviceParameters.reset(new DeviceParameters(host));
        deviceParameters->setParameterUpdateCallback(std::bind(&PhysicalDevice::remoteParameterChangeCallback, this, _1));

        sendSoftwareTriggerRequest(); // force emission of initial trigger to get at least one set of metadata
#endif

        // Initialize data streams
        std::string baseURL = std::string(udp ? "udp://" : "tcp://" ) + host;
        logicalDevices[ID_MULTIPART].reset(new LogicalDevice(this, baseURL + "/", DataStream::MULTIPART_STREAM));
        logicalDevices[ID_IMAGE_LEFT].reset(new LogicalDevice(this, baseURL + "/left", DataStream::IMAGE_LEFT_STREAM));
        logicalDevices[ID_IMAGE_RIGHT].reset(new LogicalDevice(this, baseURL + "/right", DataStream::IMAGE_RIGHT_STREAM));
        logicalDevices[ID_IMAGE_THIRD_COLOR].reset(new LogicalDevice(this, baseURL + "/third_color", DataStream::IMAGE_THIRD_COLOR_STREAM));
        logicalDevices[ID_DISPARITY].reset(new LogicalDevice(this, baseURL + "/disparity", DataStream::DISPARITY_STREAM));
        logicalDevices[ID_POINTCLOUD].reset(new LogicalDevice(this, baseURL + "/pointcloud", DataStream::POINTCLOUD_STREAM));

        bool success = false;
        {
            std::unique_lock<std::mutex> lock(receiveMutex);
            threadRunning = true;
            receiveThread = std::thread(std::bind(&PhysicalDevice::deviceReceiveThread, this));

            // Wait for first frame
            std::chrono::milliseconds duration(1000);
            initializedCondition.wait_for(lock, duration);
            success = (latestMetaData.getHeight() > 0);
        }

        if(!success) {
            threadRunning = false;
            receiveThread.join();
            return GC_ERR_IO;
        } else {
            return GC_ERR_SUCCESS;
        }
    } catch(...) {
        return GC_ERR_IO;
    }
}

void PhysicalDevice::close() {
    DEBUG_PHYS("Closing a PhysicalDevice");
#ifndef DELIVER_TEST_DATA
    // Re-isolate parameter event thread from current physical device instance
    // This may block briefly until a running parameter callback has completed
    if (deviceParameters) {
        DEBUG_PHYS("Disconnecting device parameter callback");
        deviceParameters->setParameterUpdateCallback([](const std::string& uid){});
    }
#endif
    if(threadRunning) {
        DEBUG_PHYS("Terminating physical device receiver thread");
        threadRunning = false;
        if(receiveThread.joinable()) {
            DEBUG_PHYS("Joining thread");
            receiveThread.join();
        }
    }
    DEBUG_PHYS("Closed physical device");
}

void PhysicalDevice::setTestData(ImageSet& receivedSet) {
#ifdef DELIVER_TEST_DATA
    // This data can be set for testing purposes
    receivedSet.setNumberOfImages(2);
    receivedSet.setIndexOf(ImageSet::IMAGE_LEFT, 0);
    receivedSet.setIndexOf(ImageSet::IMAGE_DISPARITY, 1);
    receivedSet.setIndexOf(ImageSet::IMAGE_RIGHT, -1);
    receivedSet.setIndexOf(ImageSet::IMAGE_COLOR, -1);
    receivedSet.setWidth(640);
    receivedSet.setHeight(480);
    receivedSet.setRowStride(0, 640);
    receivedSet.setRowStride(1, 2*640);
    receivedSet.setPixelFormat(0, ImageSet::FORMAT_8_BIT_MONO);
    receivedSet.setPixelFormat(1, ImageSet::FORMAT_12_BIT_MONO);

    static const float q[16] = {
        1.F, 0.F, 0.F, -3.2400825881958008e+02F, 0.F, 1.F, 0.F,
        -2.3012479209899902e+02F, 0.F, 0.F, 0.F, 7.6647387538017108e+02F, 0.F, 0.F,
        6.6548375133606683e+00F, 0.F
    };

    receivedSet.setQMatrix(q);
    receivedSet.setPixelData(0, TestData::leftTestData);
    receivedSet.setPixelData(1, reinterpret_cast<unsigned char*>(TestData::disparityTestData));
#endif
}

void PhysicalDevice::deviceReceiveThread() {
    try {
        bool initialized = false;
        ImageSet receivedSet;
#ifdef DELIVER_TEST_DATA
        setTestData(receivedSet);
#endif

        while(threadRunning) {
#ifdef DELIVER_TEST_DATA
            // Noting to receive in test mode
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
#else
            // Receive new image
            //if(!imageTf->receiveImageSet(receivedSet)) {
            if(!asyncTf->collectReceivedImageSet(receivedSet, 1.0)) { // Wait up to 1.0 sec for full image set, then gracefully return to running check
                // No image available
                continue;
            }
#endif

            {
                std::unique_lock<std::mutex> lock(receiveMutex);
                // Determine whether new image set is compatible to previous one
                bool metadataChanged = false;
                if (latestMetaData.getWidth()!=receivedSet.getWidth() || latestMetaData.getHeight()!=receivedSet.getHeight()) {
                    metadataChanged = true;
                } else if (latestMetaData.getNumberOfImages()!=receivedSet.getNumberOfImages()) {
                    metadataChanged = true;
                } else {
                    for (int i=0; i<latestMetaData.getNumberOfImages(); ++i) {
                        if (latestMetaData.getPixelFormat(i)!=receivedSet.getPixelFormat(i)) {
                            metadataChanged = true;
                            break;
                        }
                    }
                }

                latestMetaData = receivedSet;

                if (metadataChanged) {
                    // Live image ROI / metadata change - we need to invalidate the feature cache
                    // for relevant parameters; they will be recalculated / queried again.
                    invalidateFeatureFromAsyncEvent("Width");
                    invalidateFeatureFromAsyncEvent("Height");
                    invalidateFeatureFromAsyncEvent("PixelFormat");
                    invalidateFeatureFromAsyncEvent("PayloadSize");
                    invalidateFeatureFromAsyncEvent("OffsetX"); // GenTL representation is not center-based
                    invalidateFeatureFromAsyncEvent("OffsetY"); // (and it might also be clipped to the edges)
                    invalidateFeatureFromAsyncEvent("OffsetXReg");
                    invalidateFeatureFromAsyncEvent("OffsetYReg");
                    invalidateFeatureFromAsyncEvent("OffsetXMaxReg");
                    invalidateFeatureFromAsyncEvent("OffsetYMaxReg");

                    // Also signal all DataStreams - they must update their metadata reporting
                    for(int i=0; i<NUM_LOGICAL_DEVICES; i++) {
                        // this fetches latestMetaData internally, which is already up-to-date
                        logicalDevices[i]->getStream()->updateBufferMapping();
                    }
                }

                // Apply disparity offset
                if(disparityOffset != 0.0) {
                    unsigned short* startPtr = reinterpret_cast<unsigned short*>(receivedSet.getPixelData(ImageSet::IMAGE_DISPARITY));
                    unsigned short* endPtr = reinterpret_cast<unsigned short*>(
                        receivedSet.getPixelData(ImageSet::IMAGE_DISPARITY) +
                        receivedSet.getRowStride(ImageSet::IMAGE_DISPARITY)*receivedSet.getHeight());
                    int increment = int(receivedSet.getSubpixelFactor() * disparityOffset);
                    for(unsigned short* ptr = startPtr; ptr < endPtr; ptr++) {
                        *ptr += increment;
                    }
                    maxDisparity = 0xFFF + increment;
                }

                // Copy raw and 3D data to buffer
                copyRawDataToBuffer(receivedSet);
                if (getComponentEnabledRange() && receivedSet.hasImageType(ImageSet::IMAGE_DISPARITY)) {
                    copy3dDataToBuffer(receivedSet);
                }
                copyMultipartDataToBuffer(receivedSet);

                if(!initialized) {
                    // Initialization is done once we received the first frame
                    initialized = true;
                    initializedCondition.notify_one();
                }
            }
        }
    } catch(...) {
        // Error has occurred
        if(errorEvent != nullptr) {
            errorEvent->emitEvent(GC_ERR_IO);
        }
        threadRunning = false;
    }
}

void PhysicalDevice::copyRawDataToBuffer(const ImageSet& receivedSet) {
    for(int i=0; i<receivedSet.getNumberOfImages(); i++) {
        // Determine the correct logical device
        int id;
        if(i == receivedSet.getIndexOf(ImageSet::IMAGE_LEFT)) {
            id = ID_IMAGE_LEFT;
        } else if(i == receivedSet.getIndexOf(ImageSet::IMAGE_DISPARITY)) {
            id = ID_DISPARITY;
        } else if(i == receivedSet.getIndexOf(ImageSet::IMAGE_COLOR)) {
            id = ID_IMAGE_THIRD_COLOR;
        } else {
            id = ID_IMAGE_RIGHT;
        }

        // Copy to device buffer
        Buffer* buffer = logicalDevices[id]->getStream()->requestBuffer();
        if(buffer == nullptr) {
            continue;
        }

        int copiedBytes = copyImageToBufferMemory(receivedSet, i, buffer->getData(), static_cast<int>(buffer->getSize()));
        if(copiedBytes < 0) {
            buffer->setIncomplete(true);
        } else {
            buffer->setIncomplete(false);
        }

        buffer->setMetaData(receivedSet);
        logicalDevices[id]->getStream()->queueOutputBuffer();

        if(buffer->isIncomplete()) {
            logicalDevices[id]->getStream()->emitErrorEvent(GC_ERR_BUFFER_TOO_SMALL);
        }
    }
}

int PhysicalDevice::copyImageToBufferMemory(const ImageSet& receivedSet, int id, unsigned char* dst, int dstSize) {
    int bytesPerPixel = receivedSet.getBytesPerPixel(id);
    int newStride = receivedSet.getWidth() * bytesPerPixel;
    int totalSize = receivedSet.getHeight() * newStride;

    if(totalSize > dstSize) {
        // No more buffer space.
        return -1;
    } else {
        if(newStride == receivedSet.getRowStride(id)) {
            memcpy(dst, receivedSet.getPixelData(id), totalSize);
        } else {
            for(int y = 0; y<receivedSet.getHeight(); y++) {
                memcpy(&dst[y*newStride], &receivedSet.getPixelData(id)[y*receivedSet.getRowStride(id)], newStride);
            }
        }

        return totalSize;
    }
}

void PhysicalDevice::copy3dDataToBuffer(const ImageSet& receivedSet) {
    Buffer* buffer = logicalDevices[ID_POINTCLOUD]->getStream()->requestBuffer();
    if(buffer == nullptr) {
        // No buffer available
        return;
    }

    int copiedBytes = copy3dDataToBufferMemory(receivedSet, buffer->getData(), static_cast<int>(buffer->getSize()));
    if(copiedBytes < 0) {
        buffer->setIncomplete(true);
    } else {
        buffer->setIncomplete(false);
    }

    buffer->setMetaData(receivedSet);
    logicalDevices[ID_POINTCLOUD]->getStream()->queueOutputBuffer();

    if(buffer->isIncomplete()) {
        logicalDevices[ID_POINTCLOUD]->getStream()->emitErrorEvent(GC_ERR_BUFFER_TOO_SMALL);
    }
}

int PhysicalDevice::copy3dDataToBufferMemory(const ImageSet& receivedSet, unsigned char* dst, int dstSize) {
    int pixels = receivedSet.getWidth()*receivedSet.getHeight();
    int totalSize = 3*sizeof(float)*pixels;

    if(dstSize < totalSize
            || (!receivedSet.hasImageType(ImageSet::IMAGE_DISPARITY))
            || receivedSet.getPixelFormat(ImageSet::IMAGE_DISPARITY) != ImageSet::ImageSet::FORMAT_12_BIT_MONO) {
        // Buffer is too small, no disparity image, or pixel format doesn't match
        return -1;
    } else {
        // GenTL does not support padding between pixels. Let's only
        // copy the non-padding bytes

        float* inputPtr = reconstruct.createPointMap((unsigned short*)receivedSet.getPixelData(ImageSet::IMAGE_DISPARITY),
            receivedSet.getWidth(), receivedSet.getHeight(),
            receivedSet.getRowStride(ImageSet::IMAGE_DISPARITY), receivedSet.getQMatrix(), 0,
            receivedSet.getSubpixelFactor(), maxDisparity);

        float* outputPtr = reinterpret_cast<float*>(dst);
#ifdef __SSE2__
        copyPointsSSE(outputPtr, inputPtr, pixels);
#else
        copyPointsFallback(outputPtr, inputPtr, pixels);
#endif
        return totalSize;
    }
}

void PhysicalDevice::copyMultipartDataToBuffer(const ImageSet& receivedSet) {
    auto stream = logicalDevices[ID_MULTIPART]->getStream();
    Buffer* buffer = stream->requestBuffer();
    if(buffer == nullptr) {
        // No buffer available
        return;
    }
    auto& bufferMapping = stream->getBufferMapping();
    if (bufferMapping.getNumBufferParts() == 0) {
        // Metadata was not available, no buffer layout planned yet
        return;
    }

    buffer->setIncomplete(false);
    int copiedBytes = -1;

    for (int i=0; i<bufferMapping.getNumBufferParts(); ++i) {
        auto func = bufferMapping.getBufferPartImageSetFunction(i);
        auto offset = bufferMapping.getBufferPartOffset(i);
        auto sz = bufferMapping.getBufferPartSize(i);
        if (func != ImageSet::IMAGE_UNDEFINED) { // not the point cloud channel
            auto idx = receivedSet.getIndexOf(func);
            if (idx != -1) {
                copiedBytes = copyImageToBufferMemory(receivedSet, idx, &buffer->getData()[offset],
                    static_cast<int>(buffer->getSize()) - offset);
                if (copiedBytes < 0) {
                    DEBUG_PHYS("Buffer incomplete at ImageSet index " << idx);
                    buffer->setIncomplete(true);
                    break;
                }
            } else {
                // Channel disabled since stream was initialized; deliver zeroed-out frame
                std::memset(&buffer->getData()[offset], 0, std::min(sz, static_cast<int>(buffer->getSize()) - offset));
            }
        } else {
            // Special case: point cloud
            if (getComponentEnabledRange() && receivedSet.hasImageType(ImageSet::IMAGE_DISPARITY)) {
                copiedBytes = copy3dDataToBufferMemory(receivedSet, &buffer->getData()[offset],
                    static_cast<int>(buffer->getSize()) - offset);
                if(copiedBytes < 0) {
                    DEBUG_PHYS("Buffer incomplete at point cloud");
                    buffer->setIncomplete(true);
                }
            } else {
                // Disparity or enabled flag disabled since stream was initialized; deliver zeroed-out buffer
                std::memset(&buffer->getData()[offset], 0, std::min(sz, static_cast<int>(buffer->getSize()) - offset));
            }
        }
    }
    buffer->setMetaData(receivedSet);
    logicalDevices[ID_MULTIPART]->getStream()->queueOutputBuffer();

    if(buffer->isIncomplete()) {
        logicalDevices[ID_MULTIPART]->getStream()->emitErrorEvent(GC_ERR_BUFFER_TOO_SMALL);
    }
}

void PhysicalDevice::copyPointsFallback(float* dst, float* src, int numPoints) {
    float* endPtr = src + 4*numPoints;
    while(src < endPtr) {
        // replace with 0.0 for inf and nan Z values (-> marked invalid)
        if (std::isfinite(src[2])) {
            dst[0] = src[0];
            dst[1] = src[1];
            dst[2] = src[2];
        } else {
            dst[0] = getInvalidDepthValue(); // is inlined
            dst[1] = getInvalidDepthValue();
            dst[2] = getInvalidDepthValue();
        }
        dst+=3;
        src+=4;
    }
}

#ifdef __SSE2__
void PhysicalDevice::copyPointsSSE(float* dst, float* src, int numPoints) {
    float* endPtr = src + 4*numPoints;

    __m128 point;
    // replace with special value for inf and nan Z values (-> marked invalid)
    const __m128 invalidPoint = _mm_setr_ps(getInvalidDepthValue(), getInvalidDepthValue(), getInvalidDepthValue(), getInvalidDepthValue());
    while(src < endPtr) {
        // 1x aligned copy
        if (std::isfinite(src[2])) {
            point  = _mm_load_ps(src);
            _mm_store_ps(dst, point);
        } else {
            _mm_store_ps(dst, invalidPoint);
        }
        dst+=3; src+=4;

        // 3x unaligned copy
        if (std::isfinite(src[2])) {
            point  = _mm_load_ps(src);
            _mm_storeu_ps(dst, point);
        } else {
            _mm_storeu_ps(dst, invalidPoint);
        }
        dst+=3; src+=4;

        if (std::isfinite(src[2])) {
            point  = _mm_load_ps(src);
            _mm_storeu_ps(dst, point);
        } else {
            _mm_storeu_ps(dst, invalidPoint);
        }
        dst+=3; src+=4;

        if (std::isfinite(src[2])) {
            point  = _mm_load_ps(src);
            _mm_storeu_ps(dst, point);
        } else {
            _mm_storeu_ps(dst, invalidPoint);
        }
        dst+=3; src+=4;
    }
}
#endif

bool PhysicalDevice::inUse() {
    for(int i=0; i<NUM_LOGICAL_DEVICES; i++) {
        if(logicalDevices[i]->isOpen()) {
            return true;
        }
    }

    return false;
}


int PhysicalDevice::logicalIdToIndex(const char* id) {
    std::string idStr(id);
    if(idStr == "") {
        return ID_MULTIPART;
    } else if(idStr == "left") {
        return ID_IMAGE_LEFT;
    } else if(idStr == "right") {
        return ID_IMAGE_RIGHT;
    } else if(idStr == "third_color") {
        return ID_IMAGE_THIRD_COLOR;
    } else if(idStr == "disparity") {
        return ID_DISPARITY;
    } else if(idStr == "pointcloud") {
        return ID_POINTCLOUD;
    } else {
        return -1;
    }
}

std::string PhysicalDevice::logicalIndexToId(int index) {
    if(index == ID_MULTIPART) {
        return ""; // Default multipart
    } else if(index == ID_IMAGE_LEFT) {
        return "left";
    } else if(index == ID_IMAGE_RIGHT) {
        return "right";
    } else if(index == ID_IMAGE_THIRD_COLOR) {
        return "third_color";
    } else if(index == ID_DISPARITY) {
        return "disparity";
    } else if(index == ID_POINTCLOUD) {
        return "pointcloud";
    } else {
        return "?"; // Should not happen
    }
}

void PhysicalDevice::sendSoftwareTriggerRequest() {
    if (deviceParameters) {
        try {
            deviceParameters->triggerNow();
        } catch (visiontransfer::TransferException& e) {
            DEBUG_PHYS("Failed to send software trigger request (possible connection loss); Exception: " << e.what());
        }
    }
}

void PhysicalDevice::remoteParameterChangeCallback(const std::string& uid) {
    // This is the mapping for invalidating the proper GenICam SFNC features
    //  when the corresponding parameter is modified on the remote side.
    DEBUG_PHYS("Got remote update for " << uid);
    std::string featureName = "";
    if (uid == "manual_exposure_time" || uid == "manual_exposure_time_color") {
        invalidateFeatureFromAsyncEvent("ExposureTimeReg");
        invalidateFeatureFromAsyncEvent("ExposureTimeMinReg");
        invalidateFeatureFromAsyncEvent("ExposureTimeMaxReg");
        invalidateFeatureFromAsyncEvent("ExposureTime");
    } else if (uid == "manual_gain" || uid == "manual_gain_color") {
        invalidateFeatureFromAsyncEvent("GainReg");
        invalidateFeatureFromAsyncEvent("GainMinReg");
        invalidateFeatureFromAsyncEvent("GainMaxReg");
        invalidateFeatureFromAsyncEvent("Gain");
    } else if (uid == "auto_exposure_mode") {
        invalidateFeatureFromAsyncEvent("ExposureAutoReg");
        invalidateFeatureFromAsyncEvent("GainAutoReg");
        invalidateFeatureFromAsyncEvent("ExposureAuto");
        invalidateFeatureFromAsyncEvent("GainAuto");
    } else if (uid == "RT_input_roi_ofs_left_x" || uid == "RT_input_roi_ofs_left_y" || uid == "calib_image_size") {
        invalidateFeatureFromAsyncEvent("WidthReg");
        invalidateFeatureFromAsyncEvent("HeightReg");
        invalidateFeatureFromAsyncEvent("OffsetXReg");
        invalidateFeatureFromAsyncEvent("OffsetYReg");
        invalidateFeatureFromAsyncEvent("Width");
        invalidateFeatureFromAsyncEvent("Height");
        invalidateFeatureFromAsyncEvent("OffsetX");
        invalidateFeatureFromAsyncEvent("OffsetY");
    } else if (uid == "trigger_frequency") {
        invalidateFeatureFromAsyncEvent("AcquisitionFrameRateReg");
        invalidateFeatureFromAsyncEvent("AcquisitionFrameRate");
    } else if (uid == "trigger_input") {
        invalidateFeatureFromAsyncEvent("TriggerModeReg");
        invalidateFeatureFromAsyncEvent("TriggerSourceReg");
        invalidateFeatureFromAsyncEvent("TriggerMode");
        invalidateFeatureFromAsyncEvent("TriggerSource");
    } else if (uid == "projector_brightness") {
        invalidateFeatureFromAsyncEvent("PatternProjectorBrightnessReg");
        invalidateFeatureFromAsyncEvent("PatternProjectorBrightness");
    } else if (uid == "number_of_disparities" || uid == "disparity_offset") {
        invalidateFeatureFromAsyncEvent("NumberOfDisparitiesReg");
        invalidateFeatureFromAsyncEvent("DisparityOffsetMaxReg");
        invalidateFeatureFromAsyncEvent("DisparityOffsetReg");
        invalidateFeatureFromAsyncEvent("NumberOfDisparities");
        invalidateFeatureFromAsyncEvent("DisparityOffsetMax");
        invalidateFeatureFromAsyncEvent("DisparityOffset");
    } else if (uid == "sgm_p1_no_edge") {
        invalidateFeatureFromAsyncEvent("SgmP1NoEdgeReg");
        invalidateFeatureFromAsyncEvent("SgmP1NoEdge");
    } else if (uid == "sgm_p1_edge") {
        invalidateFeatureFromAsyncEvent("SgmP1EdgeReg");
        invalidateFeatureFromAsyncEvent("SgmP1Edge");
    } else if (uid == "sgm_p2_no_edge") {
        invalidateFeatureFromAsyncEvent("SgmP2NoEdgeReg");
        invalidateFeatureFromAsyncEvent("SgmP2NoEdge");
    } else if (uid == "sgm_p2_edge") {
        invalidateFeatureFromAsyncEvent("SgmP2EdgeReg");
        invalidateFeatureFromAsyncEvent("SgmP2Edge");
    } else if (uid == "sgm_edge_sensitivity") {
        invalidateFeatureFromAsyncEvent("SgmEdgeSensitivityReg");
        invalidateFeatureFromAsyncEvent("SgmEdgeSensitivity");
    } else if (uid == "subpixel_optimization_roi_enabled") {
        invalidateFeatureFromAsyncEvent("SubpixelOptimizationROIEnabled");
    } else if (uid == "subpixel_optimization_roi_width") {
        invalidateFeatureFromAsyncEvent("SubpixelOptimizationROIWidth");
    } else if (uid == "subpixel_optimization_roi_height") {
        invalidateFeatureFromAsyncEvent("SubpixelOptimizationROIHeight");
    } else if (uid == "subpixel_optimization_roi_x") {
        invalidateFeatureFromAsyncEvent("SubpixelOptimizationROIOffsetX");
    } else if (uid == "subpixel_optimization_roi_y") {
        invalidateFeatureFromAsyncEvent("SubpixelOptimizationROIOffsetY");
    } else if (uid == "mask_border_pixels_enabled") {
        invalidateFeatureFromAsyncEvent("MaskBorderPixelsEnabled");
    } else if (uid == "consistency_check_enabled") {
        invalidateFeatureFromAsyncEvent("ConsistencyCheckEnabled");
    } else if (uid == "consistency_check_sensitivity") {
        invalidateFeatureFromAsyncEvent("ConsistencyCheckSensitivity");
    } else if (uid == "uniqueness_check_enabled") {
        invalidateFeatureFromAsyncEvent("UniquenessCheckEnabled");
    } else if (uid == "uniqueness_check_sensitivity") {
        invalidateFeatureFromAsyncEvent("UniquenessCheckSensitivity");
    } else if (uid == "texture_filter_enabled") {
        invalidateFeatureFromAsyncEvent("TextureFilterEnabled");
    } else if (uid == "texture_filter_sensitivity") {
        invalidateFeatureFromAsyncEvent("TextureFilterSensitivity");
    } else if (uid == "gap_interpolation_enabled") {
        invalidateFeatureFromAsyncEvent("GapInterpolationEnabled");
    } else if (uid == "noise_reduction_enabled") {
        invalidateFeatureFromAsyncEvent("NoiseReductionEnabled");
    } else if (uid == "speckle_filter_iterations") {
        invalidateFeatureFromAsyncEvent("SpeckleFilterIterations");
    } else {
        return; // Unmapped feature - ignore parameter change
    }
}

void PhysicalDevice::invalidateFeatureFromAsyncEvent(const std::string& featureName) {
    for(int i=0; i<NUM_LOGICAL_DEVICES; i++) {
        if(logicalDevices[i]->isOpen()) {
            DEBUG_PHYS(" Dispatching FEATURE_INVALIDATE (if registered) for " << featureName);
            PORT_HANDLE myPort;
            logicalDevices[i]->getPort(&myPort);
            reinterpret_cast<Port*>(myPort)->emitFeatureInvalidateEvent(featureName);
        }
    }
}

void PhysicalDevice::setIntensitySource(PhysicalDevice::IntensitySource src) {
    DEBUG_PHYS("Phys: set intensity source to " << (int) src);
    intensitySource = src;
}


}

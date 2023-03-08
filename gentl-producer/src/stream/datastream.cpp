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

#include "stream/datastream.h"
#include "stream/buffer.h"
#include "misc/infoquery.h"
#include "system/system.h"
#include "event/event.h"
#include "device/logicaldevice.h"
#include "device/physicaldevice.h"
#include <algorithm>

#include <iostream> // DEBUG

using namespace visiontransfer;

namespace GenTL {

#ifdef ENABLE_DEBUGGING
#ifdef _WIN32
    std::fstream debugStreamDataStream("C:\\debug\\gentl-debug-datastream-" + std::to_string(time(nullptr)) + ".txt", std::ios::out);
#else
    std::ostream& debugStreamDataStream = std::cout;
#endif
#define DEBUG_DSTREAM(x) debugStreamDataStream << x << std::endl;
#else
#define DEBUG_DSTREAM(x) ;
#endif

DataStream::DataStream(LogicalDevice * logicalDevice, StreamType streamType)
    :Handle(TYPE_STREAM), logicalDevice(logicalDevice), streamType(streamType), framesToAquire(0),
    numDelivered(0), numUnderrun(0), numCaptured(0), newBufferEvent(nullptr),
    errorEvent(nullptr), opened(false), portImpl(this),
    port("default", "datastream.xml", "StreamPort", "TLDataStream", &portImpl) {
}

DataStream::~DataStream() {
    close();
    freeBufferEvent();
    freeErrorEvent();
}

Event* DataStream::allocNewBufferEvent() {
    if(newBufferEvent != nullptr) {
        return nullptr;
    } else {
        newBufferEvent = new Event(EVENT_NEW_BUFFER, sizeof(S_EVENT_NEW_BUFFER), this);
        return newBufferEvent;
    }
}

void DataStream::freeBufferEvent() {
    if(newBufferEvent != nullptr) {
        delete newBufferEvent;
        newBufferEvent = nullptr;
    }
}

Event* DataStream::allocErrorEvent() {
    if(errorEvent != nullptr) {
        return nullptr;
    } else {
        errorEvent = new Event(EVENT_ERROR, sizeof(GC_ERROR), nullptr);
        return errorEvent;
    }
}

void DataStream::freeErrorEvent() {
    if(errorEvent != nullptr) {
        delete errorEvent;
        errorEvent = nullptr;
    }
}

void DataStream::emitErrorEvent(GC_ERROR error) {
    if(errorEvent != nullptr) {
        errorEvent->emitEvent(error);
    }
}

Buffer* DataStream::requestBuffer() {
    if(framesToAquire > 0) {
        if(inputPool.size() > 0) {
            return inputPool.front().get();
        } else {
            numUnderrun++;
            return nullptr;
        }
    } else {
        return nullptr;
    }
}

void DataStream::queueOutputBuffer() {
    outputQueue.push_back(inputPool.front());
    inputPool.pop_front();

    if(framesToAquire != GENTL_INFINITE) {
        framesToAquire--;
    }
    numCaptured++;

    // Event notification
    S_EVENT_NEW_BUFFER eventData;
    eventData.BufferHandle = outputQueue.back().get();
    eventData.pUserPointer = outputQueue.back()->getPrivateData();

    if(newBufferEvent != nullptr) {
        DEBUG_DSTREAM("Emitting EVENT_NEW_BUFFER");
        newBufferEvent->emitEvent(eventData);
    }
}

template <class T>
bool DataStream::findBuffer(T queue, Buffer* buffer) {
    for(const std::shared_ptr<Buffer>& buf: queue) {
        if(buf.get() == buffer) {
            return true;
        }
    }
    return false;
}

GC_ERROR DataStream::announceBuffer(void* pBuffer, size_t iSize, void* pPrivate, BUFFER_HANDLE* phBuffer) {
    if(pBuffer == nullptr || phBuffer == nullptr) {
        return GC_ERR_INVALID_PARAMETER;
    }

    std::unique_lock<std::mutex> lock(logicalDevice->getPhysicalDevice()->lock());

    // Make sure consumers who opened the DataStream early get the effects of updated features
    updateBufferMapping();

    std::shared_ptr<Buffer> buffer(new Buffer(this, pPrivate,
        reinterpret_cast<unsigned char*>(pBuffer), iSize));
    buffers.push_back(buffer);
    *phBuffer = buffer.get();

    return GC_ERR_SUCCESS;
}

GC_ERROR DataStream::allocAndAnnounceBuffer(size_t iBufferSize, void* pPrivate, BUFFER_HANDLE* phBuffer) {
    if(phBuffer == nullptr) {
        return GC_ERR_INVALID_PARAMETER;
    }

    std::unique_lock<std::mutex> lock(logicalDevice->getPhysicalDevice()->lock());

    std::shared_ptr<Buffer> buffer(new Buffer(this, pPrivate, iBufferSize));
    buffers.push_back(buffer);
    *phBuffer = buffer.get();

    return GC_ERR_SUCCESS;
}

void DataStream::updateBufferMapping() {
    const ImageSet& metaData = logicalDevice->getPhysicalDevice()->getLatestMetaData();
    if(metaData.getWidth() == 0) {
        DEBUG_DSTREAM("No valid metadata - skipping BufferMapping generation here");
    } else {
        DEBUG_DSTREAM("Generating BufferMapping");
        PhysicalDevice::IntensitySource intensitySource = logicalDevice->getPhysicalDevice()->getIntensitySource();
        bool rangeEnabled = logicalDevice->getPhysicalDevice()->getComponentEnabledRange();
        bufferMapping = BufferMapping(metaData, (int) intensitySource, rangeEnabled);
        //bufferMapping.dumpToStream(std::cout);
    }
}

GC_ERROR DataStream::open() {
    if(opened) {
        return GC_ERR_RESOURCE_IN_USE;
    } else {
        updateBufferMapping();

        opened = true;
        framesToAquire = 0;
        numUnderrun = 0;
        return GC_ERR_SUCCESS;
    }
}

GC_ERROR DataStream::close() {
    if(!opened) {
        return GC_ERR_INVALID_HANDLE;
    } else {
        std::unique_lock<std::mutex> lock(logicalDevice->getPhysicalDevice()->lock());
        inputPool.clear();
        outputQueue.clear();
        opened = false;
        return GC_ERR_SUCCESS;
    }
}

GC_ERROR DataStream::revokeBuffer(BUFFER_HANDLE hBuffer, void ** ppBuffer, void ** ppPrivate) {
    std::unique_lock<std::mutex> lock(logicalDevice->getPhysicalDevice()->lock());
    Buffer* buffer = reinterpret_cast<Buffer*>(hBuffer);

    // Make sure the buffer is not in the input pool or output queue
    if(findBuffer(inputPool, buffer) || findBuffer(outputQueue, buffer)) {
        return GC_ERR_BUSY;
    }

    // Erase buffer from list
    for(auto iter = buffers.begin(); iter != buffers.end(); iter++) {
        if(iter->get() == buffer) {
            if(ppBuffer != nullptr) {
                if(buffer->isConsumerBuffer()) {
                    *ppBuffer = buffer->getData();
                } else {
                    *ppBuffer = nullptr;
                }
            }

            if(ppPrivate != nullptr) {
                *ppPrivate = buffer->getPrivateData();
            }

            buffers.erase(iter);
            return GC_ERR_SUCCESS;
        }
    }

    // Buffer wasn't in the list
    return GC_ERR_INVALID_HANDLE;
}

GC_ERROR DataStream::queueBuffer(BUFFER_HANDLE hBuffer) {
    Buffer* buffer = reinterpret_cast<Buffer*>(hBuffer);
    std::unique_lock<std::mutex> lock(logicalDevice->getPhysicalDevice()->lock());

    // Locate buffer and queue
    for(const std::shared_ptr<Buffer>& buf: buffers) {
        if(buf.get() == buffer) {
            inputPool.push_back(buf);
            return GC_ERR_SUCCESS;
        }
    }

    return GC_ERR_INVALID_HANDLE;
}

GC_ERROR DataStream::getParentDev(DEV_HANDLE* phDevice) {
    if(phDevice == nullptr) {
        return GC_ERR_INVALID_PARAMETER;
    }

    *phDevice = reinterpret_cast<DEV_HANDLE>(logicalDevice);
    return GC_ERR_SUCCESS;
}

GC_ERROR DataStream::startAcquisition(ACQ_START_FLAGS iStartFlags, uint64_t iNumToAcquire) {
    std::unique_lock<std::mutex> deviceLock(logicalDevice->getPhysicalDevice()->lock());
    logicalDevice->getPhysicalDevice()->sendSoftwareTriggerRequest();
    framesToAquire = iNumToAcquire;
    numDelivered = 0;
    return GC_ERR_SUCCESS;
}

GC_ERROR DataStream::stopAcquisition(ACQ_STOP_FLAGS iStopFlags) {
    framesToAquire = 0;
    return GC_ERR_SUCCESS;
}

GC_ERROR DataStream::flushQueue(ACQ_QUEUE_TYPE iOperation) {
    std::unique_lock<std::mutex> deviceLock(logicalDevice->getPhysicalDevice()->lock());
    bool clearEvents = false;

    switch(iOperation) {
        case ACQ_QUEUE_INPUT_TO_OUTPUT:
            for(unsigned int i=0; i<inputPool.size(); i++) {
                outputQueue.push_back(inputPool[i]);

                S_EVENT_NEW_BUFFER eventData;
                eventData.BufferHandle = inputPool[i].get();
                eventData.pUserPointer = inputPool[i]->getPrivateData();
                if(newBufferEvent != nullptr) {
                    newBufferEvent->emitEvent(eventData);
                }
            }
            inputPool.clear();
            break;
        case ACQ_QUEUE_OUTPUT_DISCARD:
            clearEvents = true;
            outputQueue.clear();
            break;
        case ACQ_QUEUE_ALL_TO_INPUT:
            clearEvents = true;
            outputQueue.clear();
            inputPool.clear();
            inputPool.insert(inputPool.end(), buffers.begin(), buffers.end());
            break;
        case ACQ_QUEUE_UNQUEUED_TO_INPUT:
            inputPool.clear();
            for(const std::shared_ptr<Buffer>& buf: buffers) {
                if(!findBuffer(outputQueue, buf.get())) {
                    inputPool.push_back(buf);
                }
            }
            break;
        case ACQ_QUEUE_ALL_DISCARD:
            clearEvents = true;
            outputQueue.clear();
            inputPool.clear();
            break;
        default:
            return GC_ERR_NOT_IMPLEMENTED;
    }

    if(clearEvents && newBufferEvent != nullptr) {
        newBufferEvent->clearEventQueue();
    }

    return GC_ERR_SUCCESS;
}

GC_ERROR DataStream::getBufferChunkData(BUFFER_HANDLE hBuffer, SINGLE_CHUNK_DATA* pChunkData,
        size_t* piNumChunks) {
    if(piNumChunks == nullptr) {
        return GC_ERR_INVALID_PARAMETER;
    }

    *piNumChunks = 0;
    return GC_ERR_SUCCESS;
}

GC_ERROR DataStream::getBufferID(uint32_t iIndex, BUFFER_HANDLE* phBuffer) {
    if(phBuffer == nullptr) {
        return GC_ERR_INVALID_PARAMETER;
    }

    std::unique_lock<std::mutex> lock(logicalDevice->getPhysicalDevice()->lock());

    if(iIndex >= buffers.size()) {
        return GC_ERR_INVALID_INDEX;
    }

    *phBuffer = buffers[iIndex].get();
    return GC_ERR_SUCCESS;
}

// Obtain payload size for specific single visiontransfer image type
// (special case IMAGE_UNDEFINED for point cloud).
// This is used for the single-part streams, the multipart stream uses
// the bufferMapping directly and maps it in the specified order.
size_t DataStream::getPayloadSizeForImageType(ImageSet::ImageType typ) {
    for (int i=0; i<bufferMapping.getNumBufferParts(); ++i) {
        if (typ == bufferMapping.getBufferPartImageSetFunction(i)) {
            return bufferMapping.getBufferPartSize(i);
        }
    }
    // Not present in the stream, cannot serve
    return 0;
}

size_t DataStream::getPayloadSize() {
    int totalSize = 0;
    switch(streamType) {
        case IMAGE_LEFT_STREAM:
            totalSize = getPayloadSizeForImageType(ImageSet::IMAGE_LEFT);
            break;
        case IMAGE_RIGHT_STREAM:
            totalSize = getPayloadSizeForImageType(ImageSet::IMAGE_RIGHT);
            break;
        case IMAGE_THIRD_COLOR_STREAM:
            totalSize = getPayloadSizeForImageType(ImageSet::IMAGE_COLOR);
            break;
        case DISPARITY_STREAM:
            totalSize = getPayloadSizeForImageType(ImageSet::IMAGE_DISPARITY);
            break;
        case POINTCLOUD_STREAM:
            totalSize = getPayloadSizeForImageType(ImageSet::IMAGE_UNDEFINED); // special case: point cloud
            break;
        default:
            // multipart: add all image/disparity sizes and finally the point cloud size (if actually enabled)
            totalSize = bufferMapping.getTotalBufferSize();
    }
    if (totalSize<=0) {
        // Fallback just in case: report size from first image
        totalSize = getPayloadSizeForImageType(bufferMapping.getBufferPartImageSetFunction(0));
    }
    return totalSize;
}

uint64_t DataStream::getPixelFormatForStreamType(const ImageSet& metaData, StreamType streamType) {
    if(streamType == IMAGE_LEFT_STREAM || streamType == IMAGE_RIGHT_STREAM || streamType == IMAGE_THIRD_COLOR_STREAM) {
        ImageSet::ImageType typ;
        switch(streamType) {
            case IMAGE_RIGHT_STREAM:
                typ = ImageSet::IMAGE_RIGHT; break;
            case IMAGE_THIRD_COLOR_STREAM:
                typ = ImageSet::IMAGE_COLOR; break;
            default:
                typ = ImageSet::IMAGE_LEFT;
        };
        int index = metaData.getIndexOf(typ);
        if(index == -1) {
            // Fallback for failed ImageSet channel query
            return 0x01080001; //"Mono8"
        } else if (metaData.getPixelFormat(index) == ImageSet::FORMAT_8_BIT_RGB) {
            return 0x02180014; //"RGB8"
        } else if(metaData.getPixelFormat(index) == ImageSet::FORMAT_12_BIT_MONO) {
            return 0x01100005; //"Mono12"
        } else {
            // Fallback for unhandled pixel format
            return 0x01080001; //"Mono8"
        }
    } else if(streamType == DISPARITY_STREAM) {
        return 0x01100005; //"Mono12"
    } else if(streamType == POINTCLOUD_STREAM) {
        return 0x026000C0; // "Coord3D_ABC32f"
    } else {
        // Multipart stream (shim; actual per-channel info obtained from buffer part info)
        bool hasColorCamImage = metaData.hasImageType(ImageSet::IMAGE_COLOR);
        PhysicalDevice::IntensitySource intensitySource = logicalDevice->getPhysicalDevice()->getIntensitySource();
        bool colorImageSourceWanted = intensitySource != PhysicalDevice::INTENSITY_SOURCE_LEFT;
        if(metaData.getPixelFormat(metaData.getIndexOf((hasColorCamImage && colorImageSourceWanted) ? ImageSet::IMAGE_COLOR : ImageSet::IMAGE_LEFT)) == ImageSet::FORMAT_8_BIT_RGB) {
            return 0x02180014; //"RGB8"
        } else {
            return 0x01080001; //"Mono8"
        }
        // Handle disparity-only ImageSet -> multipart #0 w/ 12bit?
    }
}

uint64_t DataStream::getPixelFormat(const ImageSet& metaData) {
    return getPixelFormatForStreamType(metaData, streamType);
}

GC_ERROR DataStream::getBufferInfo(BUFFER_HANDLE hBuffer, BUFFER_INFO_CMD iInfoCmd, INFO_DATATYPE* piType,
        void* pBuffer, size_t* piSize) {
    Buffer* buffer = reinterpret_cast<Buffer*>(hBuffer);
    std::unique_lock<std::mutex> lock(logicalDevice->getPhysicalDevice()->lock());
    InfoQuery info(piType, pBuffer, piSize);
    switch(iInfoCmd) {
    case BUFFER_INFO_BASE:
        info.setPtr(buffer->getData());
        break;
    case BUFFER_INFO_SIZE:
        info.setSizeT(buffer->getSize());
        break;
    case BUFFER_INFO_USER_PTR:
        info.setPtr(buffer->getPrivateData());
        break;
    case BUFFER_INFO_TIMESTAMP: {
        int secs = 0, microsecs = 0;
        buffer->getMetaData().getTimestamp(secs, microsecs);
        info.setUInt64(static_cast<uint64_t>(secs)*1000000 + microsecs);
        break;
    }
    case BUFFER_INFO_NEW_DATA:
        info.setBool(findBuffer(outputQueue, buffer));
        break;
    case BUFFER_INFO_IS_QUEUED:
        info.setBool(findBuffer(outputQueue, buffer) || findBuffer(inputPool, buffer));
        break;
    case BUFFER_INFO_IS_ACQUIRING:
        info.setBool(false);
        break;
    case BUFFER_INFO_IS_INCOMPLETE:
        info.setBool(buffer->isIncomplete());
        break;
    case BUFFER_INFO_TLTYPE:
        info.setString("Ethernet");
        break;
    case BUFFER_INFO_SIZE_FILLED:
        if(findBuffer(inputPool, buffer) || buffer->isIncomplete()) {
            info.setSizeT(0);
        } else {
            info.setSizeT(std::min(getPayloadSize(), buffer->getSize()));
        }
        break;
    case BUFFER_INFO_WIDTH:
        info.setSizeT(buffer->getMetaData().getWidth());
        break;
    case BUFFER_INFO_HEIGHT:
        info.setSizeT(buffer->getMetaData().getHeight());
        break;
    case BUFFER_INFO_XOFFSET:
        info.setSizeT(0);
        break;
    case BUFFER_INFO_YOFFSET:
        info.setSizeT(0);
        break;
    case BUFFER_INFO_XPADDING:
        info.setSizeT(0);
        break;
    case BUFFER_INFO_YPADDING:
        info.setSizeT(0);
        break;
    case BUFFER_INFO_FRAMEID:
        info.setUInt64(buffer->getMetaData().getSequenceNumber());
        break;
    case BUFFER_INFO_IMAGEPRESENT:
        info.setBool(true);
        break;
    case BUFFER_INFO_IMAGEOFFSET:
        info.setSizeT(0);
        break;
    case BUFFER_INFO_PAYLOADTYPE:
        info.setSizeT(streamType != MULTIPART_STREAM ? PAYLOAD_TYPE_IMAGE : PAYLOAD_TYPE_MULTI_PART);
        break;
    case BUFFER_INFO_PIXELFORMAT:
        if(streamType == MULTIPART_STREAM) {
            return GC_ERR_NOT_AVAILABLE;
        } else {
            info.setUInt64(getPixelFormat(buffer->getMetaData()));
        }
        break;
    case BUFFER_INFO_PIXELFORMAT_NAMESPACE:
        if(streamType == MULTIPART_STREAM) {
            return GC_ERR_NOT_AVAILABLE;
        } else {
            info.setUInt64(PIXELFORMAT_NAMESPACE_PFNC_32BIT);
        }
        break;
    case BUFFER_INFO_DELIVERED_IMAGEHEIGHT:
        info.setSizeT(buffer->getMetaData().getHeight());
        break;
    case BUFFER_INFO_PIXEL_ENDIANNESS:
        info.setInt(PIXELENDIANNESS_LITTLE);
        break;
    case BUFFER_INFO_DATA_SIZE:
        if(findBuffer(inputPool, buffer)) {
            info.setSizeT(0);
        } else {
            info.setSizeT(getPayloadSize());
        }
        break;
    case BUFFER_INFO_TIMESTAMP_NS: {
        int secs = 0, microsecs = 0;
        buffer->getMetaData().getTimestamp(secs, microsecs);
        info.setUInt64(static_cast<uint64_t>(secs)*1000000000 + static_cast<uint64_t>(microsecs) * 1000);
        break;
    }
    case BUFFER_INFO_DATA_LARGER_THAN_BUFFER:
        info.setBool(getPayloadSize() > buffer->getSize() || buffer->isIncomplete());
        break;
    case BUFFER_INFO_CONTAINS_CHUNKDATA:
        info.setBool(false);
        break;
    case BUFFER_INFO_IS_COMPOSITE:
        info.setBool(false);
        break;
    default:
        return GC_ERR_NOT_IMPLEMENTED;
    }

    return info.query();

}

GC_ERROR DataStream::getInfo(STREAM_INFO_CMD iInfoCmd, INFO_DATATYPE* piType,
        void* pBuffer, size_t* piSize) {
    InfoQuery info(piType, pBuffer, piSize);
    switch(iInfoCmd) {
        case STREAM_INFO_ID:
            info.setString("default");
            break;
        case STREAM_INFO_NUM_DELIVERED:
            info.setUInt64(numDelivered);
            break;
        case STREAM_INFO_NUM_UNDERRUN:
            info.setUInt64(numUnderrun);
            break;
        case STREAM_INFO_NUM_ANNOUNCED:
            info.setSizeT(buffers.size());
            break;
        case STREAM_INFO_NUM_QUEUED:
            info.setSizeT(inputPool.size());
            break;
        case STREAM_INFO_NUM_AWAIT_DELIVERY:
            info.setSizeT(outputQueue.size());
            break;
        case STREAM_INFO_NUM_STARTED:
            info.setUInt64(numCaptured);
            break;
        case STREAM_INFO_PAYLOAD_SIZE:
            info.setSizeT(getPayloadSize());
            break;
        case STREAM_INFO_IS_GRABBING:
            info.setBool(framesToAquire > 0);
            break;
        case STREAM_INFO_DEFINES_PAYLOADSIZE:
            info.setBool(true);
            break;
        case STREAM_INFO_TLTYPE:
            info.setString("Ethernet");
            break;
        case STREAM_INFO_NUM_CHUNKS_MAX:
            info.setSizeT(0);
        case STREAM_INFO_BUF_ANNOUNCE_MIN:
            info.setSizeT(1);
            break;
        case STREAM_INFO_BUF_ALIGNMENT:
            info.setSizeT(1);
            break;
        case STREAM_INFO_FLOW_TABLE:
            return GC_ERR_NO_DATA;
        case STREAM_INFO_GENDC_PREFETCH_DESCRIPTOR:
            return GC_ERR_NO_DATA;
        default:
            return GC_ERR_NOT_IMPLEMENTED;
    }

    return info.query();
}

GC_ERROR DataStream::getNumBufferParts(BUFFER_HANDLE hBuffer, uint32_t *piNumParts) {
    if(streamType == MULTIPART_STREAM) {
        *piNumParts = bufferMapping.getNumBufferParts();
    } else {
        *piNumParts = 1;
    }
    return GC_ERR_SUCCESS;
}

GC_ERROR DataStream::getBufferPartInfo(BUFFER_HANDLE hBuffer, uint32_t iPartIndex,
        BUFFER_PART_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize) {

    if(streamType != MULTIPART_STREAM) {
        // Part info is only available for multi part data streams
        return GC_ERR_NO_DATA;
    }

    DEBUG_DSTREAM("getBufferPartInfo idx=" << iPartIndex << " cmd=" << iInfoCmd);

    int partIndex = static_cast<int>(iPartIndex);
    Buffer* buffer = reinterpret_cast<Buffer*>(hBuffer);

    InfoQuery info(piType, pBuffer, piSize);
    switch(iInfoCmd) {
        case BUFFER_PART_INFO_BASE: {
                size_t offset = bufferMapping.getBufferPartOffset(partIndex);
                DEBUG_DSTREAM("Part " << partIndex << " offset = " << offset);
                info.setPtr(&buffer->getData()[offset]);
                break;
            }
        case BUFFER_PART_INFO_DATA_SIZE: {
                size_t sz = bufferMapping.getBufferPartSize(partIndex);
                DEBUG_DSTREAM("Part " << partIndex << " size = " << sz);
                info.setSizeT(sz);
                break;
            }
        case BUFFER_PART_INFO_DATA_TYPE:
            if (bufferMapping.getBufferPartImageSetFunction(partIndex) == ImageSet::IMAGE_UNDEFINED) { // special case point cloud
                info.setSizeT(PART_DATATYPE_3D_IMAGE);
            } else {
                info.setSizeT(PART_DATATYPE_2D_IMAGE);
            }
            break;
        case BUFFER_PART_INFO_DATA_FORMAT:
            info.setUInt64(bufferMapping.getBufferPartPixelFormat(partIndex));
            break;
        case BUFFER_PART_INFO_DATA_FORMAT_NAMESPACE:
            info.setUInt64(PIXELFORMAT_NAMESPACE_PFNC_32BIT);
            break;
        case BUFFER_PART_INFO_WIDTH:
            info.setSizeT(buffer->getMetaData().getWidth());
            break;
        case BUFFER_PART_INFO_HEIGHT:
            info.setSizeT(buffer->getMetaData().getHeight());
            break;
        case BUFFER_PART_INFO_XOFFSET:
            info.setSizeT(0);
            break;
        case BUFFER_PART_INFO_YOFFSET:
            info.setSizeT(0);
            break;
        case BUFFER_PART_INFO_XPADDING:
            info.setSizeT(0);
            break;
        case BUFFER_PART_INFO_SOURCE_ID:
            info.setUInt64(0);
            break;
        case BUFFER_PART_INFO_DELIVERED_IMAGEHEIGHT:
            info.setSizeT(buffer->getMetaData().getHeight());
            break;
        case BUFFER_PART_INFO_REGION_ID:
            return GC_ERR_NOT_AVAILABLE;
        case BUFFER_PART_INFO_DATA_PURPOSE_ID:
            info.setUInt64(bufferMapping.getBufferPartPurposeID(partIndex));
            break;
        default:
            return GC_ERR_NOT_IMPLEMENTED;
    }

    return info.query();
}

GC_ERROR DataStream::announceCompositeBuffer( size_t iNumSegments, void **ppSegments,
        size_t *piSizes, void *pPrivate, BUFFER_HANDLE *phBuffer) {
    if(ppSegments == nullptr || phBuffer == nullptr || piSizes == nullptr) {
        return GC_ERR_INVALID_PARAMETER;
    }

    if(iNumSegments == 1) {
        // Fallback to normal buffers
        return announceBuffer(ppSegments[0], piSizes[0], pPrivate, phBuffer);
    } else {
        return GC_ERR_NOT_IMPLEMENTED;
    }
}

GC_ERROR DataStream::getBufferInfoStacked(BUFFER_HANDLE hBuffer,
        DS_BUFFER_INFO_STACKED *pInfoStacked, size_t iNumInfos) {
    if(pInfoStacked == nullptr) {
        return GC_ERR_INVALID_PARAMETER;
    }

    for(int i = 0; i < (int)iNumInfos; i++) {
        pInfoStacked[i].iResult = getBufferInfo(hBuffer, pInfoStacked[i].iInfoCmd,
            &pInfoStacked[i].iType, pInfoStacked[i].pBuffer, &pInfoStacked[i].iSize);
    }

    return GC_ERR_SUCCESS;
}

GC_ERROR DataStream::getBufferPartInfoStacked( BUFFER_HANDLE hBuffer,
        DS_BUFFER_PART_INFO_STACKED *pInfoStacked, size_t iNumInfos) {
    if(pInfoStacked == nullptr) {
        return GC_ERR_INVALID_PARAMETER;
    }

    for(int i = 0; i < (int)iNumInfos; i++) {
        pInfoStacked[i].iResult = getBufferPartInfo(hBuffer, pInfoStacked[i].iPartIndex,
            pInfoStacked[i].iInfoCmd, &pInfoStacked[i].iType, pInfoStacked[i].pBuffer,
            &pInfoStacked[i].iSize);

    }
    return GC_ERR_SUCCESS;
}

GC_ERROR DataStream::getNumFlows(uint32_t *piNumFlows) {
    *piNumFlows = 1;
    return GC_ERR_SUCCESS;
}

GC_ERROR DataStream::getFlowInfo(uint32_t iFlowIndex, FLOW_INFO_CMD iInfoCmd,
        INFO_DATATYPE *piType, void *pBuffer, size_t *piSize) {
    if(iFlowIndex != 0) {
        return GC_ERR_INVALID_INDEX;
    }

    InfoQuery info(piType, pBuffer, piSize);
    switch(iInfoCmd) {
        case FLOW_INFO_SIZE:
            info.setSizeT(getPayloadSize());
            break;
        default:
            return GC_ERR_NOT_IMPLEMENTED;
    }

    return info.query();
    return GC_ERR_SUCCESS;
}

GC_ERROR DataStream::getNumBufferSegments(BUFFER_HANDLE hBuffer, uint32_t *piNumSegments) {
    *piNumSegments = 1;
    return GC_ERR_SUCCESS;
}

GC_ERROR DataStream::getBufferSegmentInfo(BUFFER_HANDLE hBuffer, uint32_t iSegmentIndex,
        SEGMENT_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize) {
    if(iSegmentIndex != 0) {
        return GC_ERR_INVALID_INDEX;
    }

    // We just forward this call to the normal getBufferInfo command
    BUFFER_INFO_CMD bufCmd = BUFFER_INFO_CUSTOM_ID;
    switch(iInfoCmd) {
        case SEGMENT_INFO_BASE:
            bufCmd = BUFFER_INFO_BASE;
            break;
        case SEGMENT_INFO_SIZE:
            bufCmd = BUFFER_INFO_SIZE;
            break;
        case SEGMENT_INFO_IS_INCOMPLETE:
            bufCmd = BUFFER_INFO_IS_INCOMPLETE;
            break;
        case SEGMENT_INFO_SIZE_FILLED:
            bufCmd = BUFFER_INFO_SIZE_FILLED;
            break;
        case SEGMENT_INFO_DATA_SIZE:
            bufCmd = BUFFER_INFO_DATA_SIZE;
            break;
        default:
            bufCmd = BUFFER_INFO_CUSTOM_ID;
    }

    return getBufferInfo(hBuffer, bufCmd, piType, pBuffer, piSize);
}

}

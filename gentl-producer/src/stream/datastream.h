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

#ifndef NERIAN_DATASTREAM_H
#define NERIAN_DATASTREAM_H

#include "misc/common.h"
#include "misc/handle.h"
#include "misc/port.h"
#include "stream/streamportimpl.h"
#include "stream/buffermapping.h"

#include <genicam/gentl.h>
#include <deque>
#include <vector>
#include <memory>

namespace visiontransfer {
    class ImageSet;
}

namespace GenTL {

// Some neccessary predeclarations
class LogicalDevice;
class Buffer;
class Event;

/*
 * Represents a DS_HANDLE and encapsulates all data stream related
 * GenTL functions
 */
class DataStream: public Handle {
public:
    enum StreamType {
        IMAGE_LEFT_STREAM = 0,
        IMAGE_RIGHT_STREAM = 1,
        IMAGE_THIRD_COLOR_STREAM = 2,
        DISPARITY_STREAM = 3,
        POINTCLOUD_STREAM = 4,
        MULTIPART_STREAM = 5
    };

    DataStream(LogicalDevice* device, StreamType type);
    ~DataStream();

    // Methods that match functions form the GenTL interface
    GC_ERROR open();
    GC_ERROR close();
    GC_ERROR announceBuffer(void* pBuffer, size_t iSize, void* pPrivate, BUFFER_HANDLE* phBuffer);
    GC_ERROR allocAndAnnounceBuffer(size_t iBufferSize, void* pPrivate, BUFFER_HANDLE* phBuffer);
    GC_ERROR revokeBuffer(BUFFER_HANDLE hBuffer, void ** ppBuffer, void ** ppPrivate);
    GC_ERROR queueBuffer(BUFFER_HANDLE hBuffer);
    GC_ERROR getParentDev(DEV_HANDLE* phDevice);
    GC_ERROR startAcquisition(ACQ_START_FLAGS iStartFlags, uint64_t iNumToAcquire);
    GC_ERROR stopAcquisition(ACQ_STOP_FLAGS iStopFlags);
    GC_ERROR flushQueue(ACQ_QUEUE_TYPE iOperation);
    GC_ERROR getBufferChunkData(BUFFER_HANDLE hBuffer, SINGLE_CHUNK_DATA* pChunkData,
            size_t* piNumChunks);
    GC_ERROR getBufferID(uint32_t iIndex, BUFFER_HANDLE* phBuffer);
    GC_ERROR getBufferInfo(BUFFER_HANDLE hBuffer, BUFFER_INFO_CMD iInfoCmd, INFO_DATATYPE* piType,
            void* pBuffer, size_t* piSize);
    GC_ERROR getInfo(STREAM_INFO_CMD iInfoCmd, INFO_DATATYPE* piType,
            void* pBuffer, size_t* piSize);
    GC_ERROR getNumBufferParts(BUFFER_HANDLE hBuffer, uint32_t *piNumParts );
    GC_ERROR getBufferPartInfo(BUFFER_HANDLE hBuffer, uint32_t iPartIndex,
        BUFFER_PART_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize);

    // New in GenTL 1.6
    GC_ERROR announceCompositeBuffer( size_t iNumSegments, void **ppSegments,
            size_t *piSizes, void *pPrivate, BUFFER_HANDLE *phBuffer);
    GC_ERROR getBufferInfoStacked(BUFFER_HANDLE hBuffer,
            DS_BUFFER_INFO_STACKED *pInfoStacked, size_t iNumInfos);
    GC_ERROR getBufferPartInfoStacked( BUFFER_HANDLE hBuffer,
            DS_BUFFER_PART_INFO_STACKED *pInfoStacked, size_t iNumInfos);
    GC_ERROR getNumFlows(uint32_t *piNumFlows);
    GC_ERROR getFlowInfo(uint32_t iFlowIndex, FLOW_INFO_CMD iInfoCmd,
            INFO_DATATYPE *piType, void *pBuffer, size_t *piSize);
    GC_ERROR getNumBufferSegments(BUFFER_HANDLE hBuffer, uint32_t *piNumSegments);
    GC_ERROR getBufferSegmentInfo(BUFFER_HANDLE hBuffer, uint32_t iSegmentIndex,
            SEGMENT_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize);

    // Methods that are used internally
    Buffer* requestBuffer();
    void queueOutputBuffer();
    LogicalDevice* getLogicalDevice() {return logicalDevice;}

    Event* allocNewBufferEvent();
    void freeBufferEvent();
    Event* allocErrorEvent();
    void freeErrorEvent();
    void emitErrorEvent(GC_ERROR error);

    void notifyDelivered() {numDelivered++;}
    bool isOpen() {return opened;}
    Port* getPort() {return &port;}
    size_t getPayloadSize();
    uint64_t getPixelFormat(const visiontransfer::ImageSet& metaData);
    StreamType getStreamType() {return streamType;}

    const BufferMapping& getBufferMapping() const { return bufferMapping; }

private:
    LogicalDevice* logicalDevice; // The physical device this stream is associated with
    StreamType streamType;

    uint64_t framesToAquire; // The number of frames that remain to be capture
    uint64_t numDelivered; // Total number of frames that have been delivered
    uint64_t numUnderrun; // Total number of buffer underruns
    uint64_t numCaptured; // Total number of buffer underruns

    std::vector<std::shared_ptr<Buffer> > buffers; // All allocated buffers
    std::deque<std::shared_ptr<Buffer> > inputPool; // Buffers in the input pool
    std::deque<std::shared_ptr<Buffer> > outputQueue; // Buffers in the output queue

    Event* newBufferEvent; // Event object for new buffer events
    Event* errorEvent; // Event object for relaying errors

    bool opened;

    StreamPortImpl portImpl;
    Port port;

    BufferMapping bufferMapping;

    // Feature state at instantiation time (may be chosen differently for later streams)
    //int intensitySource; // storage for PhysicalDevice::IntensitySource enum values

    template <class T> bool findBuffer(T queue, Buffer* buffer);
    size_t getPayloadSizeForImageType(visiontransfer::ImageSet::ImageType typ);
    uint64_t getPixelFormatForStreamType(const visiontransfer::ImageSet& metaData, StreamType streamType);

    // Update the BufferMapping, i.e. mapping from ImageSet to buffer parts, given metadata and features
    void updateBufferMapping();
};

}
#endif

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

#include "event/event.h"
#include "misc/infoquery.h"
#include "system/library.h"
#include "device/logicaldevice.h"
#include "device/physicaldevice.h"
#include "stream/datastream.h"
#include "stream/buffer.h"

#include <genicam/gentl.h>

namespace GenTL {

Event::Event(EVENT_TYPE eventId, size_t queueObjSize, DataStream* stream): Handle(TYPE_EVENT),
    eventId(eventId), abort(false), eventQueue(queueObjSize), stream(stream) {
}

void Event::clearEventQueue() {
    std::unique_lock<std::mutex> lock(mutex);
    eventQueue.clear();
}

GC_ERROR Event::registerEvent(EVENTSRC_HANDLE hModule, EVENT_TYPE iEventID, EVENT_HANDLE* phEvent) {
    if(hModule == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    } else if(phEvent == nullptr) {
        return GC_ERR_INVALID_PARAMETER;
    }

    // Identify the event source
    DataStream* stream = nullptr;
    PhysicalDevice* device = nullptr;
    Handle* handle = reinterpret_cast<Handle*>(hModule);

    switch(handle->getType()) {
        case TYPE_STREAM:
            stream = static_cast<DataStream*>(handle);
            break;
        case TYPE_DEVICE:
            device = static_cast<LogicalDevice*>(handle)->getPhysicalDevice();
            break;
        case TYPE_BUFFER:
        case TYPE_SYSTEM:
        case TYPE_INTERFACE:
        case TYPE_EVENT:
        case TYPE_PORT:
        default:
            return GC_ERR_NOT_IMPLEMENTED;
    }

    // Get event object and initialize
    Event* event = nullptr;
    GC_ERROR err = allocEventObject(device, stream, iEventID, &event);
    if(err != GC_ERR_SUCCESS) {
        return err;
    }

    std::unique_lock<std::mutex> lock(event->mutex);
    event->eventQueue.clear();
    event->abort = false;
    *phEvent = event;

    return GC_ERR_SUCCESS;
}

GC_ERROR Event::allocEventObject(PhysicalDevice* device, DataStream* stream, EVENT_TYPE iEventID, Event** event) {
    if(device == nullptr && stream == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    // Identify the event object
    switch (iEventID) {
        case EVENT_ERROR:
            if(device != nullptr) {
                *event = device->allocErrorEvent();
            } else if(stream != nullptr) {
                *event = stream->allocErrorEvent();
            }

            if(*event == nullptr) {
                return GC_ERR_RESOURCE_IN_USE;
            } else {
                return GC_ERR_SUCCESS;
            }
        case EVENT_NEW_BUFFER:
            if (stream != nullptr) {
                *event = stream->allocNewBufferEvent();
                if(*event == nullptr) {
                    return GC_ERR_RESOURCE_IN_USE;
                } else {
                    return GC_ERR_SUCCESS;
                }
            } else {
                return GC_ERR_INVALID_HANDLE;
            }
        default:
            return GC_ERR_NOT_IMPLEMENTED;
    }
}

GC_ERROR Event::unregisterEvent(EVENTSRC_HANDLE hModule, EVENT_TYPE iEventID) {
    if(hModule == nullptr) {
        return GC_ERR_INVALID_HANDLE;
    }

    // Identify the event source
    DataStream* stream = nullptr;
    PhysicalDevice* device = nullptr;
    Handle* handle = reinterpret_cast<Handle*>(hModule);

    switch(handle->getType()) {
        case TYPE_STREAM:
            stream = static_cast<DataStream*>(handle);
            if(iEventID == EVENT_ERROR) {
                stream->freeErrorEvent();
            } else if(iEventID == EVENT_NEW_BUFFER) {
                stream->freeBufferEvent();
            }
            break;
        case TYPE_DEVICE:
            device = static_cast<LogicalDevice*>(handle)->getPhysicalDevice();

            if(iEventID == EVENT_ERROR) {
                device->freeErrorEvent();
            }
            break;
        default:
            return GC_ERR_INVALID_HANDLE;
    }

    return GC_ERR_SUCCESS;
}

GC_ERROR Event::flush() {
    std::unique_lock<std::mutex> lock(mutex);

    eventQueue.clear();
    return GC_ERR_SUCCESS;
}

GC_ERROR Event::getData(void* pBuffer, size_t* piSize, uint64_t iTimeout) {
    std::unique_lock<std::mutex> lock(mutex);

    if(abort) {
        abort = false;
        return GC_ERR_ABORT;
    }

    if(eventQueue.getSize() == 0) {
        // Wait for an event to occur
        if(iTimeout != GENTL_INFINITE) {
            std::chrono::milliseconds duration(iTimeout);
            condition.wait_for(lock, duration);
        } else {
            condition.wait(lock);
        }
    }

    if(eventQueue.getSize() == 0) {
        // There is still no event.
        if(abort) {
            abort = false;
            return GC_ERR_ABORT;
        } else {
            return GC_ERR_TIMEOUT;
        }
    }

    // Pop an entry from the queue
    size_t dataSize = 0;
    void* src = nullptr;
    GC_ERROR err = GC_ERR_SUCCESS;
    S_EVENT_NEW_BUFFER buf = {nullptr, nullptr};

    if(eventId == EVENT_ERROR) {
        eventQueue.pop(err);
        src = &err;
        dataSize = sizeof(err);
    } else {
        eventQueue.pop(buf);
        src = &buf;
        dataSize = sizeof(buf);
        if(stream != nullptr) {
            stream->notifyDelivered();
        }
    }

    // Copy data to output buffer
    if(pBuffer != nullptr) {
        if(*piSize < dataSize) {
            return GC_ERR_BUFFER_TOO_SMALL;
        }
        memcpy(pBuffer, src, dataSize);
    }

    if(piSize != nullptr) {
        *piSize = dataSize;
    }

    return GC_ERR_SUCCESS;
}

GC_ERROR Event::getDataInfo(const void* pInBuffer, size_t iInSize,
        EVENT_DATA_INFO_CMD iInfoCmd, INFO_DATATYPE* piType, void* pOutBuffer, size_t* piOutSize) {
    if(pInBuffer == nullptr) {
        return GC_ERR_INVALID_PARAMETER;
    }

    InfoQuery info(piType, pOutBuffer, piOutSize);
    switch(iInfoCmd) {
        case EVENT_DATA_ID:
            if(eventId == EVENT_ERROR) {
                info.setInt(*reinterpret_cast<const GC_ERROR*>(pInBuffer));
            } else {
                info.setPtr(reinterpret_cast<const S_EVENT_NEW_BUFFER*>(pInBuffer)->BufferHandle);
            }
            break;
        case EVENT_DATA_VALUE:
            if(eventId == EVENT_ERROR) {
                info.setString(Library::errorToString(*reinterpret_cast<const GC_ERROR*>(pInBuffer)));
            } else {
                info.setPtr(reinterpret_cast<const S_EVENT_NEW_BUFFER*>(pInBuffer)->pUserPointer);
            }
            break;
        case EVENT_DATA_NUMID:
            if(eventId == EVENT_ERROR) {
                info.setUInt64(*reinterpret_cast<const GC_ERROR*>(pInBuffer));
            } else {
                info.setUInt64(reinterpret_cast<uint64_t>(
                    reinterpret_cast<const S_EVENT_NEW_BUFFER*>(pInBuffer)->BufferHandle));
            }
            break;
        default:
            return GC_ERR_NOT_IMPLEMENTED;
    }

    return info.query();
}

GC_ERROR Event::getInfo(EVENT_INFO_CMD iInfoCmd, INFO_DATATYPE * piType,
        void * pBuffer, size_t * piSize) {
    InfoQuery info(piType, pBuffer, piSize);

    switch(iInfoCmd) {
        case EVENT_EVENT_TYPE:
            info.setInt(eventId);
            break;
        case EVENT_NUM_IN_QUEUE:
            info.setSizeT(eventQueue.getSize());
            break;
        case EVENT_NUM_FIRED:
            info.setUInt64(eventQueue.getTotalPushes());
            break;
        case EVENT_SIZE_MAX:
            info.setSizeT(eventQueue.getObjectSize());
            break;
        case EVENT_INFO_DATA_SIZE_MAX:
            if(eventId == EVENT_ERROR) {
                info.setSizeT(sizeof(GC_ERROR));
            } else {
                info.setSizeT(sizeof(void*));
            }
            break;
        default:
            ; // Nothing to do
    }

    return info.query();
}

GC_ERROR Event::kill() {
    abort = true;
    condition.notify_one();
    return GC_ERR_SUCCESS;
}

}


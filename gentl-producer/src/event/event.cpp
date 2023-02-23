/*********Name********************************************************************
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

#include <iostream>
#include <fstream>

namespace GenTL {

#ifdef ENABLE_DEBUGGING
#ifdef _WIN32
    std::fstream debugStreamEvent("C:\\debug\\gentl-debug-event-" + std::to_string(time(nullptr)) + ".txt", std::ios::out);
#else
    std::ostream& debugStreamEvent = std::cout;
#endif
#define DEBUG_EVENT(x) debugStreamEvent << x << std::endl;
#else
#define DEBUG_EVENT(x) ;
#endif

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
    Port* port = nullptr;
    Handle* handle = reinterpret_cast<Handle*>(hModule);

    // RYT debug start
    DEBUG_EVENT("======= registerEvent: module=");
    switch(handle->getType()) {
        case TYPE_STREAM:
            DEBUG_EVENT("STREAM "); break;
        case TYPE_DEVICE:
            DEBUG_EVENT("DEVICE "); break;
        case TYPE_BUFFER:
            DEBUG_EVENT("BUFFER "); break;
        case TYPE_SYSTEM:
            DEBUG_EVENT("SYSTEM "); break;
        case TYPE_INTERFACE:
            DEBUG_EVENT("INTERFACE "); break;
        case TYPE_EVENT:
            DEBUG_EVENT("EVENT "); break;
        case TYPE_PORT:
            DEBUG_EVENT("PORT "); break;
        default:
            DEBUG_EVENT("(MODULE UNHANDLED!)"); break;
    }
    DEBUG_EVENT(" eventID=");
    switch (iEventID) {
        case EVENT_ERROR:
            DEBUG_EVENT("ERROR "); break;
        case EVENT_NEW_BUFFER:
            DEBUG_EVENT("NEW_BUFFER "); break;
        case EVENT_FEATURE_INVALIDATE:
            DEBUG_EVENT("FEATURE_INVALIDATE "); break;
        case EVENT_FEATURE_CHANGE:
            DEBUG_EVENT("FEATURE_CHANGE [IMPLEMENT_ME] "); break;
        case EVENT_MODULE:
            DEBUG_EVENT("MODULE [IMPLEMENT_ME] "); break;
        case EVENT_REMOTE_DEVICE:
            DEBUG_EVENT("REMOTE_DEVICE [IMPLEMENT_ME] "); break;
        default:
            DEBUG_EVENT("(Unhandled event ID " << (int) iEventID << ")"); break;
    }
    DEBUG_EVENT(std::endl);
    // RYT debug end

    switch(handle->getType()) {
        case TYPE_STREAM:
            stream = static_cast<DataStream*>(handle);
            break;
        case TYPE_DEVICE:
            device = static_cast<LogicalDevice*>(handle)->getPhysicalDevice();
            break;
        case TYPE_PORT:
            {
                //DEBUG_EVENT("Port event (not implemented)");
                port = static_cast<Port*>(handle);
                //DEBUG_EVENT(port->getPortDebugInfo());
                break;
            }
        case TYPE_BUFFER:
        case TYPE_SYSTEM:
        case TYPE_INTERFACE:
        case TYPE_EVENT:
        default:
            return GC_ERR_NOT_IMPLEMENTED;
    }

    // Get event object and initialize
    Event* event = nullptr;
    GC_ERROR err = allocEventObject(device, stream, port, iEventID, &event);
    if(err != GC_ERR_SUCCESS) {
        return err;
    }

    std::unique_lock<std::mutex> lock(event->mutex);
    event->eventQueue.clear();
    event->abort = false;
    *phEvent = event;

    return GC_ERR_SUCCESS;
}

GC_ERROR Event::allocEventObject(PhysicalDevice* device, DataStream* stream, Port* port, EVENT_TYPE iEventID, Event** event) {
    if(device == nullptr && stream == nullptr && port == nullptr) {
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
        case EVENT_FEATURE_INVALIDATE:
            if (port != nullptr) {
                *event = port->allocFeatureInvalidateEvent();
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
    Port* port = nullptr;
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
        case TYPE_PORT:
            port = static_cast<Port*>(handle);
            if(iEventID == EVENT_FEATURE_INVALIDATE) {
                port->freeFeatureInvalidateEvent();
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
    EventQueue::FeatureStringType featureName; // MAX_LEN_FEATURE_NAME

    if(eventId == EVENT_ERROR) {
        eventQueue.pop(err);
        src = &err;
        dataSize = sizeof(err);
    } else if (eventId == EVENT_NEW_BUFFER) {
        eventQueue.pop(buf);
        src = &buf;
        dataSize = sizeof(buf);
        if(stream != nullptr) {
            stream->notifyDelivered();
        }
    } else if (eventId == EVENT_FEATURE_INVALIDATE) {
        eventQueue.pop(featureName);
        src = &featureName;
        dataSize = sizeof(featureName);
        DEBUG_EVENT("getData() for FEATURE_INVALIDATE of " << featureName.str);
    } else {
        // Missing implementation, should not be reached
        return GC_ERR_ABORT;
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
            } else if (eventId == EVENT_NEW_BUFFER) {
                info.setPtr(reinterpret_cast<const S_EVENT_NEW_BUFFER*>(pInBuffer)->BufferHandle);
            } else if (eventId == EVENT_FEATURE_INVALIDATE) {
                const char* featureName = reinterpret_cast<const char*>(pInBuffer);
                info.setString(featureName); // Feature name as cstr  // TODO RYT double-check
                DEBUG_EVENT("getDataInfo() FEATURE_INVALIDATE, DATA_ID: reporting as \"" << featureName << "\"");
            } else {
                return GC_ERR_NOT_IMPLEMENTED;
            }
            break;
        case EVENT_DATA_VALUE:
            if(eventId == EVENT_ERROR) {
                info.setString(Library::errorToString(*reinterpret_cast<const GC_ERROR*>(pInBuffer)));
            } else if (eventId == EVENT_NEW_BUFFER) {
                info.setPtr(reinterpret_cast<const S_EVENT_NEW_BUFFER*>(pInBuffer)->pUserPointer);
            } else if (eventId == EVENT_FEATURE_INVALIDATE) {
                // Data value not used for this key-only event
                DEBUG_EVENT("getDataInfo() FEATURE_INVALIDATE, DATA_VALUE: set as not available");
                return GC_ERR_NOT_AVAILABLE;
            } else {
                return GC_ERR_NOT_IMPLEMENTED;
            }
            break;
        case EVENT_DATA_NUMID:
            if(eventId == EVENT_ERROR) {
                info.setUInt64(*reinterpret_cast<const GC_ERROR*>(pInBuffer));
            } else if (eventId == EVENT_NEW_BUFFER) {
                info.setUInt64(reinterpret_cast<uint64_t>(
                    reinterpret_cast<const S_EVENT_NEW_BUFFER*>(pInBuffer)->BufferHandle));
            } else if (eventId == EVENT_FEATURE_INVALIDATE) {
                DEBUG_EVENT("getDataInfo() FEATURE_INVALIDATE, DATA_NUMID: set as not available");
                return GC_ERR_NOT_AVAILABLE; // Some numerical mapping could be defined, but it is optional
            } else {
                return GC_ERR_NOT_IMPLEMENTED;
            }
            break;
        default:
            return GC_ERR_NOT_IMPLEMENTED;
    }

    if (info.query() != GC_ERR_SUCCESS) {
        DEBUG_EVENT("  getDataInfo() - setting result was attempted but not successful!");
    }

    return info.query();
}

GC_ERROR Event::getInfo(EVENT_INFO_CMD iInfoCmd, INFO_DATATYPE * piType,
        void * pBuffer, size_t * piSize) {
    InfoQuery info(piType, pBuffer, piSize);

    switch(iInfoCmd) {
        case EVENT_EVENT_TYPE:
            DEBUG_EVENT("getInfo() - EVENT_TYPE = " << eventId);
            info.setInt(eventId);
            break;
        case EVENT_NUM_IN_QUEUE:
            if (eventQueue.getSize() > 0) {
                DEBUG_EVENT("getInfo() - NUM_IN_QUEUE = " << eventQueue.getSize());
            }
            info.setSizeT(eventQueue.getSize());
            break;
        case EVENT_NUM_FIRED:
            DEBUG_EVENT("getInfo() - NUM_FIRED = " << eventQueue.getTotalPushes());
            info.setUInt64(eventQueue.getTotalPushes());
            break;
        case EVENT_SIZE_MAX:
            DEBUG_EVENT("getInfo() - SIZE_MAX = " << eventQueue.getObjectSize());
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


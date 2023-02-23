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

#ifndef NERIAN_EVENT_H
#define NERIAN_EVENT_H

#include "misc/common.h"
#include "misc/handle.h"
#include "event/eventqueue.h"

#include <genicam/gentl.h>
#include <mutex>
#include <condition_variable>

namespace GenTL {

class DataStream;
class PhysicalDevice;
class Port;

/*
 * Represents an EVENT_HANDLE and encapsulates all event related
 * GenTL functions.
 */
class Event: public Handle {
public:
    Event(EVENT_TYPE eventId, size_t queueObjSize, DataStream* stream);

    // Methods that match functions form the GenTL interface
    static GC_ERROR registerEvent(EVENTSRC_HANDLE hModule, EVENT_TYPE iEventID, EVENT_HANDLE* phEvent);
    static GC_ERROR unregisterEvent(EVENTSRC_HANDLE hModule, EVENT_TYPE iEventID);

    GC_ERROR flush();
    GC_ERROR getData(void* pBuffer, size_t* piSize, uint64_t iTimeout);
    GC_ERROR getDataInfo(const void* pInBuffer, size_t iInSize,
            EVENT_DATA_INFO_CMD iInfoCmd, INFO_DATATYPE* piType, void* pOutBuffer, size_t* piOutSize);
    GC_ERROR getInfo(EVENT_INFO_CMD iInfoCmd, INFO_DATATYPE * piType,
            void * pBuffer, size_t * piSize);
    GC_ERROR kill();

    // Methods that are used internally
    template <typename T> void emitEvent(T data) {
        std::unique_lock<std::mutex> lock(mutex);
        if(eventQueue.push(data)) {
            condition.notify_one();
        }
    }
    void clearEventQueue();

private:
    EVENT_TYPE eventId; // Event ID for which this handle has been created
    bool abort; // Set to true if kill() has been called
    EventQueue eventQueue; // The queue of pending events
    DataStream* stream; // The associated datastream or null pointer

    // Mutex and conditions allow us to wait for events
    std::mutex mutex;
    std::condition_variable_any condition;

    static GC_ERROR allocEventObject(PhysicalDevice* device, DataStream* stream, Port* port,
        EVENT_TYPE iEventID, Event** event);
};

}
#endif

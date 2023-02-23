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

#ifndef NERIAN_EVENTQUEUE_H
#define NERIAN_EVENTQUEUE_H

#include "misc/common.h"
#include <cstring>

/*
 * Stores a queue of event objects of a type that is not know at
 * compile time
 */
class EventQueue {
public:

    // Storage for a single feature ID string
    struct FeatureStringType {
        char str[128]; // MAX_LEN_FEATURE_NAME
    };

    static constexpr int MAX_EVENTS_QUEUED = 10;

    EventQueue(size_t objectSize): objectSize(objectSize), queueOffset(0),
            queueSize(0), pushTotal(0) {
        for(int i=0; i< MAX_EVENTS_QUEUED; i++) {
            dataPool[i] = new unsigned char[objectSize];
        }
    }

    ~EventQueue() {
        for(int i=0; i< MAX_EVENTS_QUEUED; i++) {
            delete [] dataPool[i];
        }
    }

    template<typename T>
    bool push(const T& obj) {
        if(queueSize == MAX_EVENTS_QUEUED) {
            return false;
        } else {
            int pos = (queueOffset + queueSize) % MAX_EVENTS_QUEUED;
            memcpy(dataPool[pos], &obj, sizeof(obj));
            queueSize++;
            pushTotal++;
            return true;
        }
    }

    template<typename T>
    bool pop(T& obj) {
        if(queueSize == 0) {
            return false;
        } else {
            std::memcpy(&obj, dataPool[queueOffset], sizeof(obj));
            queueOffset = (queueOffset + 1) % MAX_EVENTS_QUEUED;
            queueSize--;
            return true;
        }
    }

    int getSize() const {
        return queueSize;
    }

    void clear() {
        queueOffset = 0;
        queueSize = 0;
    }

    uint64_t getTotalPushes() const {
        return pushTotal;
    }

    size_t getObjectSize() const {
        return objectSize;
    }

private:
    size_t objectSize;
    int queueOffset;
    int queueSize;
    uint64_t pushTotal;
    unsigned char* dataPool[MAX_EVENTS_QUEUED];
};

#endif

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

#ifndef NERIAN_BUFFER_H
#define NERIAN_BUFFER_H

#include <genicam/gentl.h>
#include <visiontransfer/imageset.h>
#include "misc/common.h"
#include "misc/handle.h"

namespace GenTL {

class DataStream;

/*
 * Represents a BUFFER_HANDLE and manages the buffer memory.
 */
class Buffer: public Handle {
public:
    Buffer(DataStream* stream, void* privateData, size_t size);
    Buffer(DataStream* stream, void* privateData, unsigned char* data, size_t size);
    ~Buffer();

    DataStream* getStream() {return stream;}
    bool isConsumerBuffer() const {return consumerBuffer;}
    void* getPrivateData() {return privateData;}
    unsigned char* getData() {return data;}
    size_t getSize() const {return size;}
    const visiontransfer::ImageSet& getMetaData() const {return metaData;}
    const bool isIncomplete() const {return incomplete;}

    void setMetaData(const visiontransfer::ImageSet& data) {metaData = data;}
    void setIncomplete(bool incomp) {incomplete = incomp;}

private:
    DataStream* stream; // Associated stream
    bool consumerBuffer; // True if this buffer was allocated by the consumer
    void* privateData; // A data pointer passed by the consumer
    unsigned char* data; // Actual buffer memory
    size_t size; // Size of the allocated buffer memory
    visiontransfer::ImageSet metaData; // Image set object containing all the meta data
    bool incomplete;
};

}
#endif

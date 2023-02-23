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

#include "stream/buffer.h"

using namespace visiontransfer;

namespace GenTL {

Buffer::Buffer(DataStream* stream, void* privateData, unsigned char* data, size_t size)
    :Handle(TYPE_BUFFER), stream(stream), consumerBuffer(true), privateData(privateData),
        data(data), size(size), incomplete(false) {

    // Set default buffer components
    metaData.setIndexOf(ImageSet::IMAGE_LEFT, 0);
    metaData.setIndexOf(ImageSet::IMAGE_DISPARITY, 1);
    metaData.setIndexOf(ImageSet::IMAGE_RIGHT, -1);
    metaData.setIndexOf(ImageSet::IMAGE_COLOR, -1);
}


Buffer::Buffer(DataStream* stream, void* privateData, size_t size)
    :Handle(TYPE_BUFFER), stream(stream), consumerBuffer(false),
        privateData(privateData), data(new unsigned char[size]), size(size),
        incomplete(false) {
}

Buffer::~Buffer() {
    if(!consumerBuffer) {
        delete []data;
    }
}

}

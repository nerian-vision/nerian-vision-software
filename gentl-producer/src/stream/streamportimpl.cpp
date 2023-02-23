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

#include "stream/streamportimpl.h"
#include "stream/datastream.h"

namespace GenTL {

StreamPortImpl::StreamPortImpl(DataStream* stream)
    :stream(stream) {
}

GC_ERROR StreamPortImpl::readFeature(unsigned int featureId, void* pBuffer, size_t* piSize) {
    INFO_DATATYPE type;
    return stream->getInfo(featureId, &type, pBuffer, piSize);
}

GC_ERROR StreamPortImpl::writeSelector(unsigned int selector) {
    return GC_ERR_INVALID_ADDRESS; // Not supported
}

GC_ERROR StreamPortImpl::readChildFeature(unsigned int selector, unsigned int featureId,
        void* pBuffer, size_t* piSize) {
    return GC_ERR_INVALID_ADDRESS; // Not supported
}

GC_ERROR StreamPortImpl::writeChildFeature(unsigned int selector, unsigned int featureId,
        const void* pBuffer, size_t* piSize) {
    return GC_ERR_INVALID_ADDRESS; // Not supported
}

}

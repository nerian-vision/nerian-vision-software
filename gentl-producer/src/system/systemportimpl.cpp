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

#include "system/systemportimpl.h"
#include "system/system.h"

namespace GenTL {

SystemPortImpl::SystemPortImpl(System* system)
    :system(system) {
}

GC_ERROR SystemPortImpl::readFeature(unsigned int featureId, void* pBuffer, size_t* piSize) {
    INFO_DATATYPE type;
    return system->getInfo(featureId, &type, pBuffer, piSize);
}

GC_ERROR SystemPortImpl::writeSelector(unsigned int selector) {
    if(selector != 0) {
        return GC_ERR_INVALID_INDEX;
    } else {
        return GC_ERR_SUCCESS;
    }
}

GC_ERROR SystemPortImpl::readChildFeature(unsigned int selector, unsigned int featureId,
        void* pBuffer, size_t* piSize) {
    // Get the interface ID
    char id[100];
    size_t idSize = sizeof(id);
    if(system->getInterfaceID(0, id, &idSize) != GC_ERR_SUCCESS) {
        return GC_ERR_ERROR;
    }

    // Forward call to interface
    INFO_DATATYPE type;
    return system->getInterfaceInfo(id, featureId, &type, pBuffer, piSize);
}

GC_ERROR SystemPortImpl::writeChildFeature(unsigned int selector, unsigned int featureId,
        const void* pBuffer, size_t* piSize) {
    return GC_ERR_INVALID_ADDRESS; // Not supported
}


}

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

#include "interface/interfaceportimpl.h"
#include "system/system.h"

namespace GenTL {

InterfacePortImpl::InterfacePortImpl(Interface* interface)
    :interface(interface) {
}

GC_ERROR InterfacePortImpl::readFeature(unsigned int featureId, void* pBuffer, size_t* piSize) {
    INFO_DATATYPE type;
    return interface->getInfo(featureId, &type, pBuffer, piSize);
}

GC_ERROR InterfacePortImpl::writeSelector(unsigned int selector) {
    uint32_t numDevices;
    if(interface->getNumDevices(&numDevices) != GC_ERR_SUCCESS) {
        return GC_ERR_ERROR;
    }

    if(selector >=numDevices) {
        return GC_ERR_INVALID_INDEX;
    } else {
        return GC_ERR_SUCCESS;
    }
}

GC_ERROR InterfacePortImpl::readChildFeature(unsigned int selector, unsigned int featureId,
        void* pBuffer, size_t* piSize) {
    // Get the interface ID
    char id[100];
    size_t idSize = sizeof(id);
    GC_ERROR err = interface->getDeviceID(selector, id, &idSize);
    if(err != GC_ERR_SUCCESS) {
        return err;
    }

    // Forward call to interface
    INFO_DATATYPE type;
    return interface->getDeviceInfo(id, featureId, &type, pBuffer, piSize);
}

GC_ERROR InterfacePortImpl::writeChildFeature(unsigned int selector, unsigned int featureId,
        const void* pBuffer, size_t* piSize) {
    return GC_ERR_INVALID_ADDRESS; // Not supported
}

}

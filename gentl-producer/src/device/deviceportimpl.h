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

#ifndef NERIAN_DEVICEPORTIMPL_H
#define NERIAN_DEVICEPORTIMPL_H

#include "misc/common.h"
#include "misc/portimplementation.h"
#include <genicam/gentl.h>

namespace GenTL {

class LogicalDevice;

/*
 * Implements the port for the local and remote device
 */
class DevicePortImpl: public PortImplementation {
public:
    DevicePortImpl(LogicalDevice* device);

protected:
    virtual GC_ERROR readFeature(unsigned int featureId, void* pBuffer, size_t* piSize) override;
    virtual GC_ERROR writeSelector(unsigned int selector) override;
    virtual GC_ERROR readChildFeature(unsigned int selector, unsigned int featureId,
        void* pBuffer, size_t* piSize) override;
    virtual GC_ERROR writeChildFeature(unsigned int selector, unsigned int featureId,
        const void* pBuffer, size_t* piSize) override;

private:
    LogicalDevice* device;
    int currentSelectorForBalanceRatio;
    int currentSelectorForExposure;
    int currentSelectorForGain;
    int currentIndexForQMatrix;
};

}
#endif

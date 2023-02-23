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

#ifndef NERIAN_PORTIMPLEMENTATION_H
#define NERIAN_PORTIMPLEMENTATION_H

#include <genicam/gentl.h>
#include "misc/common.h"

namespace GenTL {

/*
 * Implements the read and write functions of a port. This abstract class is
 * necessary to keep virtual inheritance out of the handle types
 */
class PortImplementation {
public:
    virtual GC_ERROR readFeature(unsigned int featureId, void* pBuffer, size_t* piSize) = 0;
    virtual GC_ERROR writeSelector(unsigned int selector) = 0;
    virtual GC_ERROR readChildFeature(unsigned int selector, unsigned int featureId,
        void* pBuffer, size_t* piSize) = 0;
    virtual GC_ERROR writeChildFeature(unsigned int selector, unsigned int featureId,
        const void* pBuffer, size_t* piSize) = 0;
};

}

#endif

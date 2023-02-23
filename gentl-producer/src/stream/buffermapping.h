/*******************************************************************************
 * Copyright (c) 2023 Nerian Vision GmbH
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

#ifndef NERIAN_BUFFERMAPPING_H
#define NERIAN_BUFFERMAPPING_H

#include "visiontransfer/imageset.h"
#include "misc/common.h"
#include <iostream>
#include <cstdint>

namespace GenTL {

// Helper class encapsulating everything needed to nicely map an ImageSet
//  onto a GenTL multi-part buffer, based on metaData and selected prefs
//
// Rationale:
//  - The role of extra supplementary camera channels in GenTL is somewhat
//    underspecified, thus we adopt a clear mapping by assigning the Intensity
//    role to one channel only (or zero, if unavailable)
//  - User can select which is to be the 'intensity source' (relevant for devices
//    with a center camera also mapped to the left coordinate system, such as Ruby)
//    by setting the custom IntensitySource feature
//  - Received [meta]data restricts what is actually relayed (i.e. disparity
//    off in device settings -> it, as well as Range, must be disabled / left
//    out in GenTL).
//  - Multipart buffers are always filled in this order, indices starting at 0:
//     [LeftCamOrCenterCam]       purpose=Intensity
//     [Disparity]                purpose=Disparity
//     [Range from Reconstruct3D] purpose=Range
//     [RightCam]                 purpose=Custom+0
//     [CenterOrLeftCam]          purpose=Custom+1 (the one not chosen for Intensity)
//    All channels are optional (contingent on being present in the visiontransfer
//    stream and not being disabled on a GenApi ComponentEnable feature level.).
//    The stream's part index values are therefore not absolutely fixed.

class BufferMapping {
public:
    // Purpose IDs (correspond to ComponentIDValues, GenTL SFNC section 4.13)
    static const int PURPOSE_INTENSITY = 1;
    static const int PURPOSE_DISPARITY = 8;
    static const int PURPOSE_RANGE = 4;
    static const int PURPOSE_CUSTOM_0 = 0xFF00; // right camera, if present in stream
    static const int PURPOSE_CUSTOM_1 = 0xFF01; // left/color cam not selected for intensity, if present

    BufferMapping();
    BufferMapping(const visiontransfer::ImageSet& metaData, int intensitySource, bool componentEnabledRange);
    int getNumBufferParts() const;
    int getTotalBufferSize() const;
    // Buffer data offset and size for copying data
    int getBufferPartOffset(int partIdx) const;
    int getBufferPartSize(int partIdx) const;
    // metaData relevant to getBufferPartInfo
    visiontransfer::ImageSet::ImageType getBufferPartImageSetFunction(int partIdx) const;
    uint64_t getBufferPartPixelFormat(int partIdx) const;
    int getBufferPartPurposeID(int partIdx) const;
    // Debug: dump entire inferred BufferMapping state
    void dumpToStream(std::ostream& os) const;
private:
    // Configuration
    visiontransfer::ImageSet metaData;
    int intensitySource;
    visiontransfer::ImageSet::ImageType activeIntensityChannel; // effective value, for internal reference
    bool componentEnabledRange;
    // Resulting state
    int numBufferParts;
    int totalBufferSize;
    visiontransfer::ImageSet::ImageType imageSetFunctionMapping[visiontransfer::ImageSet::MAX_SUPPORTED_IMAGES + 1]; // one extra for Range
    int purposeIDMapping[visiontransfer::ImageSet::MAX_SUPPORTED_IMAGES + 1];
    int bufferPartSize[visiontransfer::ImageSet::MAX_SUPPORTED_IMAGES + 1];
    int bufferPartOffset[visiontransfer::ImageSet::MAX_SUPPORTED_IMAGES + 1];
};

} // namespace GenTL

#endif


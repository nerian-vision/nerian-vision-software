/*******************************************************************************
 * Copyright (c) 2024 Allied Vision Technologies GmbH
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

#ifndef VISIONTRANSFER_EXTERNALBUFFER_H
#define VISIONTRANSFER_EXTERNALBUFFER_H

#include "visiontransfer/common.h"
#include "visiontransfer/imageset.h"

namespace visiontransfer {

/// A wrapper for a single external buffer, which may be intended to hold several parts (image channels) of incoming transfers. (Functionality not available in Python API.)
class VT_EXPORT ExternalBuffer {
public:
    class Part {
        public:
            Part(ImageSet::ImageType imageType_, int conversionFlags_, int reserveBits_=0)
                : imageType(imageType_), conversionFlags(conversionFlags_), reserveBits(reserveBits_) { }
            /// The ImageType that should occupy this place in the registered parts sequence, if present
            ImageSet::ImageType imageType;
            /// The conversion flags for the incoming data, and rules for missing parts. To always leave a hole, you can use IMAGE_UNDEFINED here (and set reserveBits)
            int conversionFlags;
            /// The assumed bits-per-pixel for leaving a gap in the buffer in case of a missing channel (only used if CONVERSION_RESERVE_IF_NOT_PRESENT is set)
            int reserveBits;
    };

    // Conversion flags
    static const int CONVERSION_NONE = 0;
    static const int CONVERSION_RESERVE_IF_NOT_PRESENT = 1;
    static const int CONVERSION_MONO_12_TO_16 = 2;

    /// Basic buffer setup, data pointer and size (referring to externally allocated memory)
    ExternalBuffer(unsigned char* data, size_t size);
    ExternalBuffer(const ExternalBuffer& orig);
    /// Destroy the wrapper (external allocation is not touched)
    ~ExternalBuffer();
    /// Return a pointer to the registered data buffer
    unsigned char* getBufferPtr() const;
    /// Return the allocation size of the registered data buffer
    size_t getBufferSize() const;
    /// Append a part definition, i.e., the next definition of ImageType and conversion type to be concatenated into this buffer.
    void appendPartDefinition(Part part);
    /// Return the number of registered buffer parts
    int getNumParts() const;
    /// Return registered buffer part with specified index
    Part getPart(int idx) const;
private:
    // We follow the pimpl idiom
    class Pimpl;
    Pimpl* pimpl;
};

/// A lockable ensemble of one or more disjoint ExternalBuffers. Each set will back the data for one ImageSet. (Functionality not available in Python API.)
class VT_EXPORT ExternalBufferSet {
public:
public:
    ExternalBufferSet();
    ExternalBufferSet(const ExternalBufferSet& orig);
    const ExternalBufferSet& operator=(const ExternalBufferSet& from);
    ~ExternalBufferSet();
    /// Return the auto-generated unique buffer set handle
    ImageSet::ExternalBufferHandle getHandle() const;
    /// Add a single ExternalBuffer. At least one is required for an
    void addBuffer(ExternalBuffer buf);
    int getNumBuffers() const;
    ExternalBuffer getBuffer(int idx) const;
    /// Return 'internally busy' state, i.e., whether buffer set is locked and being filled by reception routine
    bool getBusy() const;
    /// Return 'ready for consumer' state, i.e., whether buffer set has been filled and taken out of the reception routine
    bool getReady() const;
    /// Marks the buffer set 'ready' for the API consumer. (Used internally.)
    void setReady(bool ready);
private:
    // We follow the pimpl idiom
    class Pimpl;
    Pimpl* pimpl;
};

} // namespace

#endif


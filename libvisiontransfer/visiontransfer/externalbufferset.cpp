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

#include <cstdio>
#include <iostream>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
#include <set>
#include <mutex>
#include <thread>
#include "visiontransfer/imageset.h"
#include "visiontransfer/externalbufferset.h"

using namespace visiontransfer;

namespace visiontransfer {

class ExternalBufferSet::Pimpl {
private:
    // A unique buffer set handle, either allocated sequentially or administered by the user
    ImageSet::ExternalBufferHandle handle;
    // Limit the role of the buffer set to a single channel; or multipart for IMAGE_UNDEFINED (default)
    ImageSet::ImageType imageType;
    std::vector<ExternalBuffer> buffers;
    bool blockedByUser;
public:
    Pimpl(ImageSet::ExternalBufferHandle userProvidedHandle=0, ImageSet::ImageType imageType=ImageSet::IMAGE_UNDEFINED);
    Pimpl(const Pimpl& orig);
    void addBuffer(ExternalBuffer buf);
    inline ImageSet::ExternalBufferHandle getHandle() const { return handle; }
    inline int getNumBuffers() const { return (int) buffers.size(); }
    inline ExternalBuffer getBuffer(int idx) const { return buffers.at(idx); }
    bool getBlockedByUser() const { return blockedByUser; }
    void setBlockedByUser(bool busy_) { blockedByUser = busy_; }
    ImageSet::ImageType getImageType() const { return imageType; }
};

// Pimpl functions

// ExternalBufferSet

ExternalBufferSet::Pimpl::Pimpl(ImageSet::ExternalBufferHandle userProvidedHandle, ImageSet::ImageType imageType_) {
    static ImageSet::ExternalBufferHandle nextBufferHandle = 1;
    if (userProvidedHandle != 0) {
        // Use the user handle that the ExternalBufferSet was constructed with
        handle = userProvidedHandle;
    } else {
        // Generate a unique handle (note: the two modes should not be mixed)
        handle = nextBufferHandle++;
    }
    imageType = imageType_;
    blockedByUser = false;
}

ExternalBufferSet::Pimpl::Pimpl(const ExternalBufferSet::Pimpl& orig) {
    handle = orig.handle;
    buffers = orig.buffers;
    imageType = orig.imageType;
    blockedByUser = orig.blockedByUser;
}

void ExternalBufferSet::Pimpl::addBuffer(ExternalBuffer buf) {
    buffers.push_back(buf);
}

ImageSet::ImageType ExternalBufferSet::getImageType() const {
    return pimpl->getImageType();
}

//
// Public API implementation
//

// ExternalBufferSet

ExternalBufferSet::ExternalBufferSet(ImageSet::ExternalBufferHandle handle, ImageSet::ImageType imageType)
: pimpl(new ExternalBufferSet::Pimpl(handle, imageType)) {
}

ExternalBufferSet::ExternalBufferSet(const ExternalBufferSet& orig)
: pimpl(new Pimpl(*(orig.pimpl))) {
}
const ExternalBufferSet& ExternalBufferSet::operator=(const ExternalBufferSet& from) {
    auto old = pimpl;
    pimpl = new Pimpl(*(from.pimpl));
    delete old;
    return *this;
}

ExternalBufferSet::~ExternalBufferSet() {
    delete pimpl;
}

ImageSet::ExternalBufferHandle ExternalBufferSet::getHandle() const {
    return pimpl->getHandle();
}

void ExternalBufferSet::addBuffer(ExternalBuffer buf) {
    pimpl->addBuffer(buf);
}

int ExternalBufferSet::getNumBuffers() const {
    return pimpl->getNumBuffers();
}

ExternalBuffer ExternalBufferSet::getBuffer(int idx) const {
    return pimpl->getBuffer(idx);
}

bool ExternalBufferSet::getBlockedByUser() const {
    return pimpl->getBlockedByUser();
}

void ExternalBufferSet::setBlockedByUser(bool blocked) {
    pimpl->setBlockedByUser(blocked);
}

} // namespace



